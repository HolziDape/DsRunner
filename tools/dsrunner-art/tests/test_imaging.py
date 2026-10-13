import io
import struct
import unittest

from PIL import Image

from tests.helpers import png_bytes

from dsrunnerart import imaging


def header_fields(data: bytes) -> dict:
    return {
        "magic": data[:2],
        "pixel_offset": struct.unpack_from("<I", data, 0x0A)[0],
        "dib_size": struct.unpack_from("<I", data, 0x0E)[0],
        "width": struct.unpack_from("<i", data, 0x12)[0],
        "height": struct.unpack_from("<i", data, 0x16)[0],
        "bpp": struct.unpack_from("<H", data, 0x1C)[0],
        "compression": struct.unpack_from("<I", data, 0x1E)[0],
        "colors": struct.unpack_from("<I", data, 0x2E)[0],
    }


class BmpOutputTests(unittest.TestCase):
    def setUp(self):
        self.hero = imaging.to_hero(Image.open(io.BytesIO(png_bytes((300, 260)))), "snap")
        self.data = imaging.to_ds_bmp(self.hero)

    def test_header_matches_launcher_rules(self):
        fields = header_fields(self.data)
        self.assertEqual(fields["magic"], b"BM")
        self.assertGreaterEqual(fields["dib_size"], 40)
        self.assertEqual(fields["width"], 256)
        self.assertIn(fields["height"], (192, -192))
        self.assertEqual(fields["bpp"], 8)
        self.assertEqual(fields["compression"], 0)
        self.assertLessEqual(fields["colors"], 256)
        self.assertEqual(len(self.data), fields["pixel_offset"] + 256 * 192)

    def test_validator_accepts_and_index_zero_unused(self):
        validation = imaging.validate_bmp(self.data)
        self.assertEqual(validation.errors, [])
        self.assertEqual(validation.warnings, [])
        offset = header_fields(self.data)["pixel_offset"]
        self.assertNotIn(0, self.data[offset:])

    def test_palette_is_15_bit(self):
        palette = self.data[54:54 + 1024]
        for i in range(256):
            b, g, r, _ = palette[i * 4:i * 4 + 4]
            for value in (r, g, b):
                self.assertEqual(value, ((value >> 3) << 3) | (value >> 5), f"entry {i}")

    def test_pillow_reads_it_back(self):
        image = Image.open(io.BytesIO(self.data))
        self.assertEqual(image.size, (256, 192))
        self.assertEqual(image.mode, "P")
        # top left pixel stays close to the source (bottom-up rows written correctly)
        original = self.hero.getpixel((5, 5))
        decoded = image.convert("RGB").getpixel((5, 5))
        self.assertLess(sum(abs(a - b) for a, b in zip(original, decoded)), 120)

    def test_few_colors(self):
        data = imaging.to_ds_bmp(Image.new("RGB", (256, 192), (10, 200, 30)))
        self.assertTrue(imaging.validate_bmp(data).ok)
        self.assertEqual(imaging.validate_bmp(data).warnings, [])
        offset = header_fields(data)["pixel_offset"]
        self.assertEqual(set(data[offset:]), {1})

    def test_many_colors_stay_within_255(self):
        gradient = Image.new("RGB", (256, 192))
        gradient.putdata([(x, y, (x * y) % 256) for y in range(192) for x in range(256)])
        data = imaging.to_ds_bmp(gradient)
        offset = header_fields(data)["pixel_offset"]
        pixels = data[offset:]
        self.assertNotIn(0, pixels)
        self.assertLessEqual(len(set(pixels)), 255)
        self.assertTrue(imaging.validate_bmp(data).ok)


class ValidatorTests(unittest.TestCase):
    def test_rejects_other_formats(self):
        image = Image.new("RGB", (256, 192), (1, 2, 3))
        buffer = io.BytesIO()
        image.save(buffer, "BMP")
        self.assertIn("24 bit pro Pixel statt 8", imaging.validate_bmp(buffer.getvalue()).errors)

        buffer = io.BytesIO()
        image.resize((128, 96)).quantize(16).save(buffer, "BMP")
        self.assertTrue(any("Größe" in error for error in imaging.validate_bmp(buffer.getvalue()).errors))

        self.assertEqual(imaging.validate_bmp(b"PNG"), imaging.Validation(errors=["keine BMP-Datei"]))

    def test_rejects_compressed_and_truncated(self):
        data = bytearray(imaging.to_ds_bmp(Image.new("RGB", (256, 192), (5, 5, 5))))
        compressed = bytearray(data)
        struct.pack_into("<I", compressed, 0x1E, 1)
        self.assertFalse(imaging.validate_bmp(bytes(compressed)).ok)
        self.assertFalse(imaging.validate_bmp(bytes(data[:-100])).ok)

    def test_top_down_and_index_zero_warning(self):
        palette = [(0, 0, 0)] * 2
        pixels = bytes(256 * 192)
        data = bytearray(imaging.encode_bmp(pixels, palette, skip_index_zero=False))
        struct.pack_into("<i", data, 0x16, -192)
        validation = imaging.validate_bmp(bytes(data))
        self.assertTrue(validation.ok)
        self.assertTrue(validation.warnings)


class FitTests(unittest.TestCase):
    def test_dual_screen_snap_uses_top_screen(self):
        image = Image.new("RGB", (256, 384), (0, 0, 255))
        image.paste((255, 0, 0), (0, 0, 256, 192))
        hero = imaging.to_hero(image, "title")
        self.assertEqual(hero.size, (256, 192))
        self.assertEqual(hero.getpixel((128, 96)), (255, 0, 0))
        self.assertEqual(hero.getpixel((128, 191)), (255, 0, 0))

    def test_cover_fit_sizes(self):
        for size in [(240, 160), (160, 144), (512, 459), (512, 512), (300, 600), (1920, 1080), (10, 10)]:
            for art_type in ("title", "snap", "boxart", "custom"):
                with self.subTest(size=size, art_type=art_type):
                    hero = imaging.to_hero(Image.new("RGB", size, (9, 9, 9)), art_type)
                    self.assertEqual(hero.size, (256, 192))

    def test_boxart_keeps_top(self):
        image = Image.new("RGB", (512, 512), (0, 0, 255))
        image.paste((255, 255, 0), (0, 0, 512, 40))
        hero = imaging.to_hero(image, "boxart")
        self.assertEqual(hero.getpixel((128, 2)), (255, 255, 0))

    def test_transparent_images(self):
        image = Image.new("RGBA", (256, 192), (255, 0, 0, 0))
        hero = imaging.to_hero(image, "custom")
        self.assertEqual(hero.getpixel((10, 10)), imaging.BACKGROUND)


class TileFormatTests(unittest.TestCase):
    """Tile images: 64x64, 8 bpp, uncompressed, palette index 0 unused (docs/CONFIG.md, "Kachel-Bilder")."""

    def check_tile(self, data: bytes):
        fields = header_fields(data)
        self.assertEqual(fields["magic"], b"BM")
        self.assertEqual((fields["width"], abs(fields["height"])), (64, 64))
        self.assertEqual(fields["bpp"], 8)
        self.assertEqual(fields["compression"], 0)
        self.assertEqual(len(data), fields["pixel_offset"] + 64 * 64)
        pixels = data[fields["pixel_offset"]:]
        self.assertNotIn(0, pixels)
        self.assertLessEqual(len(set(pixels)), 255)
        validation = imaging.validate_bmp(data, (64, 64))
        self.assertEqual((validation.errors, validation.warnings), ([], []))
        # an artwork validator rejects it and the other way round
        self.assertFalse(imaging.validate_bmp(data).ok)

    def test_boxart_tile(self):
        box = Image.open(io.BytesIO(png_bytes((512, 460))))
        tile = imaging.to_tile(box, "boxart", "nds")
        self.assertEqual(tile.size, (64, 64))
        self.check_tile(imaging.to_ds_bmp(tile))

    def test_many_colors(self):
        gradient = Image.new("RGB", (64, 64))
        gradient.putdata([(x * 4, y * 4, (x * y) % 256) for y in range(64) for x in range(64)])
        self.check_tile(imaging.to_ds_bmp(gradient))
        self.assertFalse(imaging.validate_bmp(imaging.to_ds_bmp(Image.new("RGB", (256, 192))), (64, 64)).ok)

    def test_all_sizes_and_types(self):
        for size in [(512, 460), (512, 512), (300, 600), (256, 384), (384, 256), (240, 160), (32, 32)]:
            for art_type in ("title", "snap", "boxart", "custom"):
                with self.subTest(size=size, art_type=art_type):
                    tile = imaging.to_tile(Image.new("RGB", size, (9, 9, 9)), art_type, "nds")
                    self.assertEqual(tile.size, (64, 64))

    def test_icon_tile_is_a_clean_2x_upscale(self):
        icon = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        icon.putpixel((0, 0), (255, 0, 0, 255))
        icon.putpixel((31, 31), (0, 255, 0, 255))
        tile = imaging.icon_tile(icon)
        self.assertEqual(tile.size, (64, 64))
        for x, y in ((0, 0), (1, 0), (0, 1), (1, 1)):
            self.assertEqual(tile.getpixel((x, y)), (255, 0, 0))
        self.assertEqual(tile.getpixel((63, 63)), (0, 255, 0))
        # transparent icon pixels show the dark background
        self.assertEqual(tile.getpixel((2, 0)), imaging.BACKGROUND)
        self.check_tile(imaging.to_ds_bmp(tile))

    def test_signature(self):
        data = imaging.to_ds_bmp(Image.new("RGB", (64, 64), (1, 2, 3)))
        self.assertTrue(imaging.is_tool_bmp(data))
        buffer = io.BytesIO()
        Image.new("RGB", (64, 64)).quantize(16).save(buffer, "BMP")
        self.assertFalse(imaging.is_tool_bmp(buffer.getvalue()))


class ScreenChoiceTests(unittest.TestCase):
    def test_empty_top_screen_uses_bottom(self):
        image = Image.new("RGB", (256, 384), (0, 0, 0))
        detail = Image.open(io.BytesIO(png_bytes((256, 192), (200, 40, 40))))
        image.paste(detail, (0, 192))
        screen, portrait = imaging.pick_screen(image, "nds")
        self.assertFalse(portrait)
        self.assertEqual(screen.tobytes(), detail.convert("RGB").tobytes())

    def test_top_screen_wins_otherwise(self):
        image = Image.new("RGB", (256, 384), (0, 0, 0))
        image.paste(Image.open(io.BytesIO(png_bytes((256, 192), (0, 0, 200)))), (0, 0))
        image.paste(Image.open(io.BytesIO(png_bytes((256, 192), (200, 0, 0)))), (0, 192))
        screen, _ = imaging.pick_screen(image)
        self.assertEqual(screen.getpixel((2, 2)), image.getpixel((2, 2)))

    def test_side_by_side_takes_left_screen(self):
        image = Image.new("RGB", (384, 256), (0, 0, 255))
        image.paste((255, 0, 0), (0, 0, 192, 256))
        screen, portrait = imaging.pick_screen(image, "nds")
        self.assertTrue(portrait)
        self.assertEqual(screen.size, (192, 256))
        self.assertEqual(imaging.to_hero(image, "title", "nds").getpixel((128, 96)), (255, 0, 0))
        # a GBA screen has the same aspect and stays whole
        self.assertEqual(imaging.pick_screen(Image.new("RGB", (240, 160)), "gba")[0].size, (240, 160))

    def test_spine_is_cut(self):
        box = Image.new("RGB", (512, 460), (255, 0, 0))
        box.paste((255, 255, 255), (0, 0, 69, 460))
        self.assertEqual(imaging.trim_spine(box, "nds").getpixel((0, 200)), (255, 0, 0))
        self.assertEqual(imaging.to_tile(box, "boxart", "nds").getpixel((1, 32)), (255, 0, 0))
        self.assertEqual(imaging.trim_spine(box, "snes").size, (512, 460))

    def test_16_bit_grayscale(self):
        image = Image.new("I;16", (256, 192), 65535)
        self.assertEqual(imaging.to_hero(image, "snap").getpixel((5, 5)), (255, 255, 255))


class GeneratedArtTests(unittest.TestCase):
    def test_console_art(self):
        for label in ("NDS", "GBA", "Rennspiele", "Ein sehr langer Ordnername mit vielen Wörtern"):
            image = imaging.console_art(label, (44, 74, 168))
            self.assertEqual(image.size, (256, 192))
            self.assertTrue(imaging.validate_bmp(imaging.to_ds_bmp(image)).ok)

    def test_icon_art(self):
        icon = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        icon.paste((255, 0, 0, 255), (8, 8, 24, 24))
        image = imaging.icon_art(icon, (44, 74, 168))
        self.assertEqual(image.size, (256, 192))
        # the enlarged icon sits centered in the upper part
        self.assertEqual(image.getpixel((128, 50)), (255, 0, 0))


if __name__ == "__main__":
    unittest.main()
