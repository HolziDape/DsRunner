"""DsRunner Stream wire protocol (see stream/PROTOCOL.md).

All multi-byte values are little-endian (the DS is little-endian, so the DS side
can read them without byte swapping).
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

VERSION = 1
DEFAULT_PORT = 7770

# packet types (first byte)
PKT_FRAME = ord("F")   # host -> DS, one chunk of an encoded frame
PKT_HELLO = ord("H")   # DS -> broadcast, "is there a host?"
PKT_HERE = ord("W")    # host -> DS, answer to HELLO
PKT_INPUT = ord("I")   # DS -> host, buttons/touch/ack, 60 times per second
PKT_BYE = ord("Q")     # DS -> host, DS leaves

# frame payload flags
FLAG_KEY = 0x01        # keyframe: all tiles, no tile bitmap
FLAG_PALETTE = 0x02    # body starts with a palette
FLAG_LZ4 = 0x04        # body is one LZ4 block

# video modes
MODE_256x192 = 0       # one screen, full resolution
MODE_128x96 = 1        # one screen, half resolution, the DS scales it up 2x in hardware
MODE_256x384 = 2       # both screens (top half -> top screen)

MODES = {
    "256x192": MODE_256x192,
    "128x96": MODE_128x96,
    "256x384": MODE_256x384,
}
MODE_SIZE = {
    MODE_256x192: (256, 192),
    MODE_128x96: (128, 96),
    MODE_256x384: (256, 384),
}

# input flags (DS -> host)
IN_NEED_REF = 0x01     # the DS rejected a delta frame; it still shows cur_frame
IN_HELLO = 0x02        # first packets of a session: (re)start streaming

# DS keys, same bits as libnds keysCurrent()
KEY_BITS = {
    "A": 0, "B": 1, "SELECT": 2, "START": 3, "RIGHT": 4, "LEFT": 5, "UP": 6, "DOWN": 7,
    "R": 8, "L": 9, "X": 10, "Y": 11, "TOUCH": 12,
}

FRAME_HDR = struct.Struct("<BBHHHIHH")      # flags mode frame_id ref_id ntiles raw_size npal rsv
PKT_HDR = struct.Struct("<BBHBBHI")         # type ver frame_id index count chunk total
INPUT = struct.Struct("<BBHHBBHBBHHI")      # type ver seq keys tx ty cur flags rsv ok bad bytes
HERE = struct.Struct("<BBHB")               # type ver port mode + name
HELLO = struct.Struct("<BBH")               # type ver nonce + name

MAX_CHUNK = 1400       # UDP payload per frame packet (header included stays < 1472)


def next_id(fid: int) -> int:
    """Frame ids run 1..65535, 0 means "no frame"."""
    fid = (fid + 1) & 0xFFFF
    return fid or 1


@dataclass
class FrameHeader:
    flags: int
    mode: int
    frame_id: int
    ref_id: int
    ntiles: int
    raw_size: int
    npal: int

    def pack(self) -> bytes:
        return FRAME_HDR.pack(self.flags, self.mode, self.frame_id, self.ref_id,
                              self.ntiles, self.raw_size, self.npal, 0)

    @classmethod
    def unpack(cls, data: bytes) -> "FrameHeader":
        f, m, fid, ref, nt, raw, npal, _ = FRAME_HDR.unpack_from(data, 0)
        return cls(f, m, fid, ref, nt, raw, npal)


def packetize(frame_id: int, payload: bytes, chunk: int = MAX_CHUNK - PKT_HDR.size) -> list[bytes]:
    """Splits an encoded frame into UDP packets."""
    total = len(payload)
    count = max(1, (total + chunk - 1) // chunk)
    if count > 255:
        raise ValueError(f"frame too large: {total} bytes")
    out = []
    for i in range(count):
        part = payload[i * chunk:(i + 1) * chunk]
        out.append(PKT_HDR.pack(PKT_FRAME, VERSION, frame_id, i, count, chunk, total) + part)
    return out


class Reassembler:
    """Collects frame packets, like the DS does: one frame at a time, a packet of a
    newer frame drops an incomplete older one."""

    def __init__(self) -> None:
        self.frame_id = None
        self.buf = bytearray()
        self.have: set[int] = set()
        self.count = 0
        self.dropped = 0

    def add(self, pkt: bytes) -> tuple[int, bytes] | None:
        if len(pkt) < PKT_HDR.size or pkt[0] != PKT_FRAME:
            return None
        _, ver, fid, idx, count, chunk, total = PKT_HDR.unpack_from(pkt, 0)
        if ver != VERSION or idx >= count:
            return None
        if fid != self.frame_id:
            if self.frame_id is not None and self.have:
                if ((fid - self.frame_id) & 0xFFFF) >= 0x8000:
                    return None  # late packet of an older frame
                self.dropped += 1
            self.frame_id, self.buf, self.have, self.count = fid, bytearray(total), set(), count
        data = pkt[PKT_HDR.size:]
        self.buf[idx * chunk:idx * chunk + len(data)] = data
        self.have.add(idx)
        if len(self.have) == self.count:
            out = bytes(self.buf)
            self.have = set()
            self.frame_id = fid  # keep id so duplicates of this frame are ignored
            self.count = -1
            return fid, out
        return None


@dataclass
class InputState:
    seq: int = 0
    keys: int = 0
    touch_x: int = 0
    touch_y: int = 0
    cur_frame: int = 0
    flags: int = 0
    frames_ok: int = 0
    frames_bad: int = 0
    bytes_rx: int = 0

    def pack(self) -> bytes:
        return INPUT.pack(PKT_INPUT, VERSION, self.seq & 0xFFFF, self.keys & 0xFFFF,
                          self.touch_x & 0xFF, self.touch_y & 0xFF, self.cur_frame & 0xFFFF,
                          self.flags & 0xFF, 0, self.frames_ok & 0xFFFF,
                          self.frames_bad & 0xFFFF, self.bytes_rx & 0xFFFFFFFF)

    @classmethod
    def unpack(cls, data: bytes) -> "InputState":
        (_, _, seq, keys, tx, ty, cur, flags, _, ok, bad, nbytes) = INPUT.unpack_from(data, 0)
        return cls(seq, keys, tx, ty, cur, flags, ok, bad, nbytes)


def pack_hello(nonce: int = 0, name: str = "DsRunnerStream") -> bytes:
    return HELLO.pack(PKT_HELLO, VERSION, nonce & 0xFFFF) + name.encode("ascii", "replace")[:31]


def pack_here(port: int, mode: int, name: str) -> bytes:
    return HERE.pack(PKT_HERE, VERSION, port, mode) + name.encode("ascii", "replace")[:31]


def unpack_here(data: bytes) -> tuple[int, int, str]:
    _, _, port, mode = HERE.unpack_from(data, 0)
    return port, mode, data[HERE.size:].decode("ascii", "replace")
