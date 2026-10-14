"""Runs the real DS decoder code (stream/ds/src/decode.c, built for the PC) on
frames from the Python encoder, with lost and reordered packets, and compares the
result with the Python reference decoder."""
import os
import random
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

import numpy as np

from dsstream import protocol as P
from dsstream.codec import Decoder, Encoder

from helpers import scene

HOSTTEST = Path(__file__).resolve().parents[2] / "ds" / "hosttest"


def build() -> Path | None:
    if not shutil.which(os.environ.get("CC", "cc")) or not shutil.which("make"):
        return None
    r = subprocess.run(["make", "-s", "-C", str(HOSTTEST)], capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError(r.stdout + r.stderr)
    return HOSTTEST / "decode_test"


class CDecoderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exe = build()
        if cls.exe is None:
            raise unittest.SkipTest("no C compiler")

    def check(self, mode, loss=0.0, n=40):
        w, h = P.MODE_SIZE[mode]
        rng = random.Random(mode * 100 + int(loss * 1000))
        enc = Encoder(mode)
        ref = Decoder()          # Python decoder, fed the same packets
        rea = P.Reassembler()
        expected = []
        with tempfile.TemporaryDirectory() as d:
            pk = Path(d) / "packets.bin"
            with open(pk, "wb") as f:
                need, lagged = False, (False, 0)
                for t in range(n):
                    # like the host: optimistic delta to the last sent frame, against
                    # the frame the DS has once it reported a rejected delta (the
                    # report arrives one frame later, like over a real network)
                    base = lagged[1] if lagged[0] else enc.last_id
                    lagged = (need, ref.cur)
                    payload, info = enc.encode(scene(t, w, h), base if t else None)
                    pkts = P.packetize(info.frame_id, payload)
                    if len(pkts) > 1:
                        pkts[0], pkts[-1] = pkts[-1], pkts[0]   # reorder
                    for p in pkts:
                        if t > 0 and rng.random() < loss:
                            continue
                        f.write(struct.pack("<H", len(p)) + p)
                        r = rea.add(p)
                        if r:
                            res = ref.apply(r[1])
                            need = res == "rejected"
                            expected.append({"key": 1, "delta": 2, "rejected": 0}[res])
            out = Path(d) / "out.bin"
            subprocess.run([str(self.exe), str(pk), str(out)], check=True)
            data = out.read_bytes()
        nres = struct.unpack_from("<I", data, 0)[0]
        results = list(struct.unpack_from(f"<{nres}b", data, 4))
        pos = 4 + nres
        cur, cmode = struct.unpack_from("<HB", data, pos)
        pos += 4
        pal = np.frombuffer(data, "<u2", 256, pos)
        pos += 512
        top = np.frombuffer(data, np.uint8, 65536, pos).reshape(256, 256)
        bot = np.frombuffer(data, np.uint8, 65536, pos + 65536).reshape(256, 256)
        self.assertEqual(results, expected)
        self.assertEqual(cur, ref.cur)
        self.assertEqual(cmode, ref.mode)
        if mode == P.MODE_256x384:
            got = np.vstack([top[:192], bot[:192]])
        else:
            got = top[:h, :w]
        np.testing.assert_array_equal(got, ref.idx)
        np.testing.assert_array_equal(pal[:len(enc.mapper.pal555)], ref.palette[:len(enc.mapper.pal555)])
        return results

    def test_all_modes(self):
        for mode in (P.MODE_256x192, P.MODE_128x96, P.MODE_256x384):
            with self.subTest(mode=mode):
                self.check(mode)

    def test_with_loss(self):
        res = self.check(P.MODE_256x192, loss=0.08, n=80)
        self.assertIn(0, res)    # some deltas were rejected and then recovered


if __name__ == "__main__":
    unittest.main()
