import unittest

import numpy as np

from dsstream import protocol as P
from dsstream.codec import (Decoder, Encoder, PaletteMapper, c555_to_rgb, fixed_palette, from_tiles,
                            rgb_to_555, to_tiles)

from helpers import scene


class CodecTest(unittest.TestCase):
    def run_stream(self, mode, palette="adaptive", use_lz4=True, n=30):
        w, h = P.MODE_SIZE[mode]
        enc = Encoder(mode, palette=palette, use_lz4=use_lz4)
        dec = Decoder()
        kinds = []
        for t in range(n):
            payload, info = enc.encode(scene(t, w, h), enc.last_id if t else None)
            kinds.append(dec.apply(payload))
            # the decoder must end up with exactly what the encoder remembers
            np.testing.assert_array_equal(dec.idx, enc.history[info.frame_id][0])
            self.assertEqual(dec.cur, info.frame_id)
            # and look like the source (palette error only)
            err = np.abs(dec.rgb().astype(int) - scene(t, w, h).astype(int)).mean()
            self.assertLess(err, 25 if t % 10 == 9 else 12, f"frame {t}")
        self.assertEqual(kinds[0], "key")
        self.assertIn("delta", kinds)
        return enc

    def test_modes(self):
        for mode in (P.MODE_256x192, P.MODE_128x96, P.MODE_256x384):
            with self.subTest(mode=mode):
                self.run_stream(mode)

    def test_fixed_palette_and_raw(self):
        self.run_stream(P.MODE_256x192, palette="fixed", use_lz4=False)

    def test_delta_is_small(self):
        enc = Encoder()
        p0, i0 = enc.encode(scene(0), None)
        p1, i1 = enc.encode(scene(1), enc.last_id)
        self.assertFalse(i1.key)
        self.assertLess(i1.ntiles, 40)
        self.assertLess(len(p1), len(p0))

    def test_rejects_wrong_reference(self):
        enc = Encoder()
        dec = Decoder()
        p0, _ = enc.encode(scene(0), None)
        p1, _ = enc.encode(scene(1), enc.last_id)
        p2, _ = enc.encode(scene(2), enc.last_id)
        self.assertEqual(dec.apply(p0), "key")
        self.assertEqual(dec.apply(p2), "rejected")   # p1 was "lost"
        self.assertEqual(dec.cur, 1)
        # recovery: the host encodes against what the DS has
        p3, i3 = enc.encode(scene(3), dec.cur)
        self.assertFalse(i3.key)
        self.assertEqual(dec.apply(p3), "delta")

    def test_unknown_reference_gives_keyframe(self):
        enc = Encoder()
        enc.encode(scene(0), None)
        _, info = enc.encode(scene(1), 999)
        self.assertTrue(info.key)

    def test_header_layout(self):
        enc = Encoder()
        payload, info = enc.encode(scene(0), None)
        hdr = P.FrameHeader.unpack(payload)
        self.assertEqual(P.FRAME_HDR.size, 16)
        self.assertEqual(hdr.frame_id, info.frame_id)
        self.assertTrue(hdr.flags & P.FLAG_KEY and hdr.flags & P.FLAG_PALETTE)
        self.assertLessEqual(hdr.npal, 256)
        self.assertEqual(hdr.raw_size, ((hdr.npal * 2 + 3) & ~3) + 768 * 64)

    def test_tiles_roundtrip(self):
        a = np.random.default_rng(0).integers(0, 256, (192, 256), dtype=np.uint8)
        np.testing.assert_array_equal(from_tiles(to_tiles(a), 256, 192), a)
        self.assertEqual(to_tiles(a)[1].tolist(), a[0:8, 8:16].ravel().tolist())

    def test_rgb555(self):
        c = rgb_to_555(np.array([[255, 0, 0], [0, 255, 0], [0, 0, 255]], np.uint8))
        self.assertEqual(c.tolist(), [31, 31 << 5, 31 << 10])
        self.assertEqual(c555_to_rgb(np.array([0x7FFF]))[0].tolist(), [255, 255, 255])

    def test_mapper_nearest(self):
        m = PaletteMapper(fixed_palette())
        img = scene(3)
        idx, err = m.map(rgb_to_555(img))
        pal = c555_to_rgb(fixed_palette()).astype(int)
        # brute force check on a few pixels
        for y, x in [(0, 0), (100, 50), (191, 255), (96, 128)]:
            c = c555_to_rgb(rgb_to_555(img[y, x]))
            d = ((pal - c.astype(int)) ** 2).sum(axis=1)
            self.assertEqual(d[idx[y, x]], d.min())


if __name__ == "__main__":
    unittest.main()
