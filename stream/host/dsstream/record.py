"""Records a page as the DS would receive it, for tests and benchmarks.

    python -m dsstream.record --url test --seconds 4 --out packets.bin [--mode 256x192]

packets.bin: [u16 length][UDP packet] ..., every frame encoded as a delta to the one
before (the first as keyframe), exactly the packets the host would send. Feed it to
stream/ds/hosttest/decode_test or build it into the DS benchmark (stream/ds/data).
"""
from __future__ import annotations

import argparse
import asyncio
import struct
import time

from . import protocol as P
from .browser import Browser
from .codec import Encoder


def write_packets(f, payload: bytes, frame_id: int) -> int:
    n = 0
    for pkt in P.packetize(frame_id, payload):
        f.write(struct.pack("<H", len(pkt)) + pkt)
        n += len(pkt)
    return n


async def record(args) -> None:
    mode = P.MODES[args.mode]
    vw, vh = (512, 768) if mode == P.MODE_256x384 else (512, 384)
    b = Browser(args.url, (vw, vh), P.MODE_SIZE[mode], max_fps=args.fps)
    await b.start()
    enc = Encoder(mode, palette=args.palette)
    seq = -1
    frames = keys = total = 0
    t_end = time.monotonic() + args.seconds
    with open(args.out, "wb") as f:
        while time.monotonic() < t_end:
            await asyncio.sleep(1 / args.fps)
            if b.frame_seq == seq:
                continue
            seq = b.frame_seq
            payload, info = enc.encode(b.latest_rgb(), enc.last_id if frames else None)
            total += write_packets(f, payload, info.frame_id)
            frames += 1
            keys += info.key
    await b.close()
    print(f"{frames} Bilder ({keys} Keyframes), {total} Bytes, {total / max(frames, 1):.0f} Bytes/Bild -> {args.out}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="dsstream.record")
    ap.add_argument("--url", default="test")
    ap.add_argument("--mode", default="256x192", choices=list(P.MODES))
    ap.add_argument("--palette", default="adaptive", choices=["adaptive", "fixed"])
    ap.add_argument("--seconds", type=float, default=4.0)
    ap.add_argument("--fps", type=float, default=20.0)
    ap.add_argument("--out", default="packets.bin")
    asyncio.run(record(ap.parse_args(argv)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
