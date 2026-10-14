"""Fake DS client for testing the host without a DS.

Speaks the same protocol as DsRunnerStream.nds: finds the host (broadcast) or uses
--host, sends input 60 times per second, reassembles and decodes the frames with the
same rules as the DS, saves PNGs and prints fps/bandwidth.

    python -m dsstream.fakeclient --seconds 10 --save out/ --script "RIGHT@1-3,A@4-4.2,TOUCH:200x150@5-5.2"

--loss drops that share of the received packets, --max-kbs drops packets above that
rate (roughly like a DS that can't keep up), both to test the recovery.
"""
from __future__ import annotations

import argparse
import json
import random
import socket
import sys
import time
from pathlib import Path

from . import protocol as P
from .codec import Decoder


def parse_script(s: str) -> list[tuple[str, int, int, float, float]]:
    """"KEY@t0-t1,TOUCH:XxY@t0-t1" -> [(key, x, y, t0, t1)]"""
    out = []
    for part in filter(None, (p.strip() for p in s.split(","))):
        what, when = part.split("@")
        t0, _, t1 = when.partition("-")
        t1 = t1 or str(float(t0) + 0.15)
        x = y = 0
        if ":" in what:
            what, pos = what.split(":")
            x, y = (int(v) for v in pos.lower().split("x"))
        what = what.upper()
        if what not in P.KEY_BITS:
            raise ValueError(f"unknown key {what}")
        out.append((what, x, y, float(t0), float(t1)))
    return out


def discover(port: int, timeout: float = 3.0) -> tuple[str, int, str]:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.settimeout(0.5)
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        for target in ("255.255.255.255", "127.0.0.1"):
            try:
                s.sendto(P.pack_hello(), (target, port))
            except OSError:
                pass
        try:
            data, addr = s.recvfrom(256)
        except socket.timeout:
            continue
        if data and data[0] == P.PKT_HERE:
            hport, _, name = P.unpack_here(data)
            return addr[0], hport, name
    raise SystemExit("Kein Host gefunden")


def run(args) -> dict:
    if args.host:
        host, port, name = args.host, args.port, "?"
    else:
        host, port, name = discover(args.port)
        print(f"Host gefunden: {name} {host}:{port}")
    script = parse_script(args.script) if args.script else []
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1 << 20)
    sock.setblocking(False)
    rea = P.Reassembler()
    dec = Decoder()
    st = P.InputState(flags=P.IN_HELLO)
    rng = random.Random(1)
    save = Path(args.save) if args.save else None
    if save:
        save.mkdir(parents=True, exist_ok=True)
    t_start = time.monotonic()
    next_input = t_start
    need = True
    last_apply = t_start
    stats = dict(frames=0, keyframes=0, deltas=0, rejected=0, incomplete=0, bytes=0, packets=0,
                 dropped_sim=0, first_frame_s=None)
    bucket, bucket_t = 0.0, t_start
    saved = 0
    while True:
        now = time.monotonic()
        t = now - t_start
        if t >= args.seconds:
            break
        # receive everything that is there
        while True:
            try:
                pkt, _ = sock.recvfrom(2048)
            except BlockingIOError:
                break
            if args.max_kbs:
                bucket = max(0.0, bucket - (now - bucket_t) * args.max_kbs * 1000)
                bucket_t = now
                if bucket > args.max_kbs * 1000 * 0.05:   # 50 ms of buffer
                    stats["dropped_sim"] += 1
                    continue
                bucket += len(pkt) + 28
            if args.loss and rng.random() < args.loss:
                stats["dropped_sim"] += 1
                continue
            stats["packets"] += 1
            stats["bytes"] += len(pkt)
            st.bytes_rx += len(pkt)
            r = rea.add(pkt)
            if r is None:
                continue
            fid, payload = r
            res = dec.apply(payload)
            if res == "rejected":
                stats["rejected"] += 1
                need = True
                continue
            need = False
            last_apply = now
            stats["frames"] += 1
            stats["keyframes" if res == "key" else "deltas"] += 1
            st.frames_ok += 1
            if stats["first_frame_s"] is None:
                stats["first_frame_s"] = round(t, 3)
            if save and stats["frames"] % args.save_every == 0 and saved < args.max_png:
                from PIL import Image
                Image.fromarray(dec.rgb()).save(save / f"frame_{stats['frames']:05d}.png")
                saved += 1
        stats["incomplete"] = rea.dropped
        st.frames_bad = rea.dropped + stats["rejected"]
        if now - last_apply > 1.0:
            need = True
        if now >= next_input:
            keys, tx, ty = 0, 0, 0
            for key, x, y, t0, t1 in script:
                if t0 <= t < t1:
                    keys |= 1 << P.KEY_BITS[key]
                    if key == "TOUCH":
                        tx, ty = x, y
            st.keys, st.touch_x, st.touch_y = keys, tx, ty
            st.cur_frame = dec.cur
            st.flags = (P.IN_NEED_REF if need else 0) | (P.IN_HELLO if t < 1 else 0)
            st.seq = (st.seq + 1) & 0xFFFF
            sock.sendto(st.pack(), (host, port))
            next_input += 1 / 60
        time.sleep(0.001)
    sock.sendto(bytes([P.PKT_BYE, P.VERSION]), (host, port))
    dur = args.seconds - (stats["first_frame_s"] or 0)
    stats["fps"] = round(stats["frames"] / max(dur, 0.001), 2)
    stats["kbs"] = round(stats["bytes"] / max(dur, 0.001) / 1000, 1)
    stats["avg_frame_bytes"] = round(stats["bytes"] / max(1, stats["frames"]))
    if save and dec.idx is not None:
        from PIL import Image
        Image.fromarray(dec.rgb()).save(save / "last.png")
    return stats


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="dsstream.fakeclient")
    ap.add_argument("--host", default=None, help="IP des Hosts (sonst Suche per Broadcast)")
    ap.add_argument("--port", type=int, default=P.DEFAULT_PORT)
    ap.add_argument("--seconds", type=float, default=10.0)
    ap.add_argument("--save", default=None, help="Ordner fuer PNGs")
    ap.add_argument("--save-every", type=int, default=30)
    ap.add_argument("--max-png", type=int, default=20)
    ap.add_argument("--script", default="", help='Tasten, z. B. "RIGHT@1-3,A@4,TOUCH:128x96@5-5.2"')
    ap.add_argument("--loss", type=float, default=0.0, help="Anteil verworfener Pakete (0..1)")
    ap.add_argument("--max-kbs", type=float, default=0.0, help="Pakete ueber dieser Rate verwerfen")
    args = ap.parse_args(argv)
    stats = run(args)
    print(json.dumps(stats, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
