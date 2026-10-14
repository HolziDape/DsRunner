import random
import unittest

from dsstream import protocol as P


class ProtocolTest(unittest.TestCase):
    def test_packetize_reassemble_shuffled(self):
        payload = bytes(random.Random(1).getrandbits(8) for _ in range(10000))
        pkts = P.packetize(7, payload)
        self.assertTrue(all(len(p) <= P.MAX_CHUNK for p in pkts))
        random.Random(2).shuffle(pkts)
        r = P.Reassembler()
        out = None
        for p in pkts + pkts[:2]:          # duplicates after completion are ignored
            res = r.add(p)
            if res:
                self.assertIsNone(out)
                out = res
        self.assertEqual(out, (7, payload))

    def test_newer_frame_drops_incomplete(self):
        r = P.Reassembler()
        a = P.packetize(1, b"a" * 3000)
        b = P.packetize(2, b"b" * 100)
        r.add(a[0])
        self.assertEqual(r.add(b[0]), (2, b"b" * 100))
        self.assertEqual(r.dropped, 1)
        self.assertIsNone(r.add(a[1]))       # late packet of the old frame

    def test_input_roundtrip(self):
        self.assertEqual(P.INPUT.size, 20)
        s = P.InputState(seq=65535, keys=0x1FFF, touch_x=255, touch_y=191, cur_frame=1234,
                         flags=P.IN_NEED_REF, frames_ok=5, frames_bad=6, bytes_rx=1 << 31)
        self.assertEqual(P.InputState.unpack(s.pack()), s)

    def test_here_hello(self):
        port, mode, name = P.unpack_here(P.pack_here(7770, P.MODE_128x96, "pi5"))
        self.assertEqual((port, mode, name), (7770, P.MODE_128x96, "pi5"))
        self.assertEqual(P.pack_hello()[0], P.PKT_HELLO)

    def test_next_id_skips_zero(self):
        self.assertEqual(P.next_id(65535), 1)
        self.assertEqual(P.next_id(0), 1)
        self.assertEqual(P.next_id(5), 6)


if __name__ == "__main__":
    unittest.main()
