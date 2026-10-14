"""UDP server: answers discovery, receives DS input, sends encoded frames paced."""
from __future__ import annotations

import asyncio
import bisect
import collections
import logging
import socket
import time
from dataclasses import dataclass, field

from . import protocol as P
from .codec import Encoder
from .mapping import InputTranslator, Mapping

log = logging.getLogger("dsstream.server")

SESSION_TIMEOUT = 3.0     # seconds without input -> DS gone
RECOVER_RETRY = 0.3       # resend a recovery frame for the same reference after this
CATCHUP_AFTER = 0.5       # DS still behind the last sent frame (lost, nothing new came)
RX_LAG = 0.1              # bytes sent this long ago should have reached the DS


@dataclass
class RateControl:
    """Send rate in bytes/s (AIMD). Every 2 seconds the bytes the DS says it got
    are compared with the bytes sent (RX_LAG earlier, so bytes in flight don't
    count as lost):
      - more than 15 % missing (and the DS lost frames): the DS or the WiFi can't
        keep up -> down to 90 % of what arrived (at most -20 %),
      - more than 6 % missing while the rate was the limit -> -15 %,
      - under 2 % missing while the rate was the limit -> +step.
    A little random loss (busy channel) doesn't slow the stream down."""
    rate: float
    min_rate: float
    max_rate: float
    step: float = 12000.0
    window: float = 2.0
    last_rx: int | None = None
    last_bad: int = 0
    last_sent: int = 0
    last_check: float = 0.0
    loss: float = 0.0

    def update(self, bytes_rx: int, frames_bad: int, bytes_sent: int, now: float) -> None:
        if self.last_rx is None:
            self.last_rx, self.last_bad, self.last_sent, self.last_check = bytes_rx, frames_bad, bytes_sent, now
            return
        dt = now - self.last_check
        if dt < self.window:
            return
        rx = (bytes_rx - self.last_rx) & 0xFFFFFFFF
        sent = bytes_sent - self.last_sent
        bad = (frames_bad - self.last_bad) & 0xFFFF
        if sent > 4000:
            self.loss = max(0.0, 1.0 - rx / sent)
            limiting = sent / dt > 0.8 * self.rate
            old = self.rate
            if self.loss > 0.15 and bad:
                self.rate = min(self.rate * 0.8, max(0.9 * rx / dt, self.rate * 0.5))
            elif self.loss > 0.06 and bad and limiting:
                self.rate *= 0.85
            elif self.loss < 0.02 and limiting:
                self.rate += self.step
            self.rate = min(self.max_rate, max(self.min_rate, self.rate))
            if self.rate != old:
                log.debug("rate %.0f -> %.0f B/s (sent %d, rx %d, %.1f s, %d frames lost)",
                          old, self.rate, sent, rx, dt, bad)
        self.last_rx, self.last_bad, self.last_sent, self.last_check = bytes_rx, frames_bad, bytes_sent, now


@dataclass
class Session:
    addr: tuple
    translator: InputTranslator
    last_seen: float
    seq: int = -1
    cur: int = 0
    need: bool = True
    started: bool = False
    last_recover_ref: int = -1
    last_recover_time: float = 0.0
    frames_ok: int = 0
    frames_bad: int = 0
    bytes_rx: int = 0


@dataclass
class Stats:
    t0: float = field(default_factory=time.monotonic)
    frames: int = 0
    keys: int = 0
    bytes: int = 0
    packets: int = 0
    enc_ms: float = 0.0
    ds_ok0: int | None = None
    ds_bad0: int | None = None
    captured0: int | None = None   # pictures from the browser at the start of the interval


def subnet_broadcast(ip: str) -> str:
    """Broadcast address of the DS's /24 network (192.168.4.247 -> 192.168.4.255).
    255.255.255.255 would leave a Pi through its default route (the LAN cable), not
    through the hotspot the DS is in."""
    parts = ip.split(".")
    return ".".join(parts[:3] + ["255"]) if len(parts) == 4 else "255.255.255.255"


class StreamServer(asyncio.DatagramProtocol):
    def __init__(self, browser, mapping: Mapping, mode: int, *, port: int = P.DEFAULT_PORT,
                 name: str | None = None, palette: str = "adaptive", use_lz4: bool = True,
                 max_fps: float = 20.0, rate_kbs: float = 120.0, min_rate_kbs: float = 40.0,
                 max_rate_kbs: float = 220.0, stats_every: float = 5.0,
                 broadcast: bool = False, packet_size: int = P.MAX_CHUNK):
        self.browser = browser
        self.mapping = mapping
        self.mode = mode
        self.port = port
        self.name = (name or socket.gethostname())[:31]
        self.encoder = Encoder(mode, palette=palette, use_lz4=use_lz4)
        self.max_fps = max_fps
        self.rc = RateControl(rate_kbs * 1000, min_rate_kbs * 1000, max_rate_kbs * 1000)
        self.stats_every = stats_every
        # send the pictures as broadcasts: WiFi sends those at the lowest rate that every
        # device understands, which helps when the access point picks a rate the DS can't
        # receive (the DS Lite only knows 1 and 2 Mbit/s). No retries, so a bit more loss.
        self.broadcast = broadcast
        self.chunk = max(200, min(P.MAX_CHUNK_LIMIT, packet_size)) - P.PKT_HDR.size
        self.session: Session | None = None
        self.transport = None
        self.events: asyncio.Queue = asyncio.Queue()
        self.stats = Stats()
        self.next_send = 0.0
        self.last_frame_time = 0.0
        self.last_send_time = 0.0
        self.last_img_seq = -1
        self.wake = asyncio.Event()
        self.bytes_sent = 0
        self.sent_hist: collections.deque = collections.deque(maxlen=4096)  # (time, bytes_sent)

    # ---------------------------------------------------------------- network in
    def connection_made(self, transport) -> None:
        self.transport = transport
        sock = transport.get_extra_info("socket")
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        except OSError:
            pass

    def _translator(self) -> InputTranslator:
        vw, vh = self.browser.viewport
        if self.mode == P.MODE_256x384:
            return InputTranslator(self.mapping, vw, vh, touch_y_offset=192, total_h=384)
        return InputTranslator(self.mapping, vw, vh)

    def datagram_received(self, data: bytes, addr) -> None:
        if len(data) < 2 or data[1] != P.VERSION:
            return
        t = data[0]
        now = time.monotonic()
        if t == P.PKT_HELLO:
            here = P.pack_here(self.port, self.mode, self.name)
            self.transport.sendto(here, addr)
            # also as a broadcast: reaches the DS even when unicast frames to it get lost
            try:
                self.transport.sendto(here, (subnet_broadcast(addr[0]), addr[1]))
            except OSError:
                pass
            log.debug("HELLO von %s", addr)
        elif t == P.PKT_INPUT and len(data) >= P.INPUT.size:
            inp = P.InputState.unpack(data)
            s = self.session
            if s is None or s.addr != addr:
                if s is not None:
                    self._end_session("anderer DS uebernimmt")
                log.info("DS verbunden: %s:%d", *addr[:2])
                s = self.session = Session(addr, self._translator(), now)
                self.encoder.reset()
                self.rc.last_rx = None
                self.stats = Stats()
                self.wake.set()
            elif (inp.seq == s.seq or ((inp.seq - s.seq) & 0xFFFF) >= 0x8000) \
                    and not inp.flags & P.IN_HELLO:
                return  # duplicate or reordered old input (HELLO: the DS started anew)
            s.seq = inp.seq
            s.last_seen = now
            s.cur = inp.cur_frame
            was_need = s.need
            s.need = bool(inp.flags & P.IN_NEED_REF) or inp.cur_frame == 0
            s.frames_ok, s.frames_bad, s.bytes_rx = inp.frames_ok, inp.frames_bad, inp.bytes_rx
            self.rc.update(inp.bytes_rx, inp.frames_bad, self._sent_before(now - RX_LAG), now)
            for ev in s.translator.update(inp.keys, inp.touch_x, inp.touch_y):
                self.events.put_nowait(ev)
            if s.need and not was_need:
                self.wake.set()
        elif t == P.PKT_BYE:
            if self.session and self.session.addr == addr:
                self._end_session("DS hat beendet")

    def _sent_before(self, t: float) -> int:
        """Bytes sent up to time t (to compare with what the DS reports)."""
        h = self.sent_hist
        if not h:
            return self.bytes_sent
        if h[0][0] > t:
            return h[0][1]
        i = bisect.bisect_right(h, (t, float("inf"))) - 1
        return h[i][1]

    def _end_session(self, why: str) -> None:
        s = self.session
        if s is None:
            return
        log.info("DS getrennt (%s)", why)
        for ev in s.translator.release_all():
            self.events.put_nowait(ev)
        self.session = None

    # ---------------------------------------------------------------- tasks
    async def input_loop(self) -> None:
        while True:
            ev = await self.events.get()
            await self.browser.inject(ev)

    async def _send_paced(self, packets: list[bytes], addr) -> None:
        for pkt in packets:
            now = time.monotonic()
            if self.next_send > now:
                await asyncio.sleep(self.next_send - now)
            self.transport.sendto(pkt, (subnet_broadcast(addr[0]), addr[1]) if self.broadcast else addr)
            self.bytes_sent += len(pkt)
            self.sent_hist.append((time.monotonic(), self.bytes_sent))
            self.next_send = max(self.next_send, time.monotonic() - 0.02) + (len(pkt) + 28) / self.rc.rate
            self.stats.packets += 1

    async def frame_loop(self) -> None:
        min_interval = 1.0 / self.max_fps
        while True:
            s = self.session
            now = time.monotonic()
            if s is None:
                self.wake.clear()
                try:
                    await asyncio.wait_for(self.wake.wait(), 0.5)
                except asyncio.TimeoutError:
                    pass
                continue
            if now - s.last_seen > SESSION_TIMEOUT:
                self._end_session("keine Eingaben mehr")
                continue
            wait = self.last_frame_time + min_interval - now
            if wait > 0:
                await asyncio.sleep(wait)
                continue

            img = self.browser.latest_rgb()
            if img is None:
                await asyncio.sleep(0.05)
                continue
            new_img = self.browser.frame_seq != self.last_img_seq
            enc = self.encoder
            recover = False
            if s.need or not s.started:
                recover = s.cur != s.last_recover_ref or now - s.last_recover_time > RECOVER_RETRY
            elif s.cur != enc.last_id and now - self.last_send_time > CATCHUP_AFTER:
                recover = s.cur != s.last_recover_ref or now - s.last_recover_time > RECOVER_RETRY
            if not (new_img or recover):
                self.wake.clear()
                try:
                    await asyncio.wait_for(self.wake.wait(), 0.01)
                except asyncio.TimeoutError:
                    pass
                continue

            if recover:
                ref = s.cur if s.cur and enc.has_ref(s.cur) else None
                s.last_recover_ref, s.last_recover_time = s.cur, now
            else:
                ref = enc.last_id if enc.has_ref(enc.last_id) else None
            self.last_img_seq = self.browser.frame_seq
            payload, info = enc.encode(img, ref)
            s.started = True
            self.last_frame_time = now
            st = self.stats
            st.frames += 1
            st.keys += info.key
            st.bytes += info.size
            st.enc_ms += info.ms
            log.debug("frame %d %s ref=%s tiles=%d raw=%d size=%d enc=%.1fms",
                      info.frame_id, "KEY" if info.key else "delta", ref, info.ntiles,
                      info.raw_size, info.size, info.ms)
            await self._send_paced(P.packetize(info.frame_id, payload, self.chunk), s.addr)
            self.last_send_time = time.monotonic()

    async def stats_loop(self) -> None:
        while True:
            await asyncio.sleep(self.stats_every)
            s, st = self.session, self.stats
            if s is None:
                continue
            dt = time.monotonic() - st.t0
            if st.ds_ok0 is None:
                st.ds_ok0, st.ds_bad0 = s.frames_ok, s.frames_bad
                st.captured0 = getattr(self.browser, "captured", 0)
                continue
            ds_ok = (s.frames_ok - st.ds_ok0) & 0xFFFF
            ds_bad = (s.frames_bad - st.ds_bad0) & 0xFFFF
            captured = getattr(self.browser, "captured", 0)
            # the browser only delivers a picture when the page changed: few pictures here
            # mean a still page (or a slow browser), not a slow stream
            browser_fps = (captured - st.captured0) / dt if st.captured0 is not None else 0.0
            log.info("Browser %.1f fps | gesendet %.1f fps, %.0f KB/s (Keyframes %d), Encoder %.1f ms/Bild | "
                     "DS zeigt %.1f fps, verloren %d | Senderate %.0f KB/s, Paketverlust %.0f %%",
                     browser_fps, st.frames / dt, st.bytes / dt / 1000, st.keys,
                     st.enc_ms / max(1, st.frames), ds_ok / dt, ds_bad, self.rc.rate / 1000, self.rc.loss * 100)
            self.stats = Stats(ds_ok0=s.frames_ok, ds_bad0=s.frames_bad, captured0=captured)


async def serve(server: StreamServer, bind: str = "0.0.0.0") -> None:
    loop = asyncio.get_running_loop()
    transport, _ = await loop.create_datagram_endpoint(
        lambda: server, local_addr=(bind, server.port), allow_broadcast=True)
    log.info("Warte auf den DS an UDP-Port %d (Name \"%s\")", server.port, server.name)
    try:
        await asyncio.gather(server.frame_loop(), server.input_loop(), server.stats_loop())
    finally:
        transport.close()
