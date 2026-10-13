"""Covers for the cover flow (/_pico/covers/user) and small icons for the 4x3 grid (/_pico/icons/user)."""
import io
import json
import struct
import unittest

from PIL import Image, ImageDraw

from tests.helpers import gba_rom, nds_rom, png_bytes, write
from tests.test_imaging import header_fields
from tests.test_tiles import DS, GBA, MARIO_KART, CardTestCase
from tests.test_updater import thumb

from dsrunnerart import imaging
from dsrunnerart.sdcard import user_file_path
from dsrunnerart.updater import check_card

GOLDEN_SUN = "Golden Sun (Germany)"


def pico_validate(data: bytes, width: int, height: int, bpp: int) -> bool:
    """Mirror of BmpHeader::Validate (arm9/source/romBrowser/FileType/BmpHeader.h)."""
    if data[:2] != b"BM":
        return False
    dib_size, file_width, file_height = struct.unpack_from("<Iii", data, 0x0E)
    file_bpp = struct.unpack_from("<H", data, 0x1C)[0]
    compression = struct.unpack_from("<I", data, 0x1E)[0]
    colors = struct.unpack_from("<I", data, 0x2E)[0]
    return (dib_size == 40 and file_width == width and abs(file_height) == height and file_bpp == bpp
            and compression == 0 and colors in (0, 1 << bpp))


def pico_load_icon(data: bytes):
    """Mirror of BmpFileIconData::Load: (32 rows of 32 palette indices from the top, 16 BGR palette
    entries as RGB), or None when Pico rejects the file."""
    header = data[:118]
    if len(header) < 118 or not pico_validate(header, 32, 32, 4):
        return None
    offset = struct.unpack_from("<I", header, 0x0A)[0]
    if offset < 118:
        return None
    top_down = struct.unpack_from("<i", header, 0x16)[0] < 0
    palette = [(header[0x36 + i * 4 + 2], header[0x36 + i * 4 + 1], header[0x36 + i * 4]) for i in range(16)]
    raw = data[offset:offset + 512]
    if len(raw) != 512:
        return None
    rows = []
    for y in range(32):
        source = raw[(y if top_down else 31 - y) * 16:][:16]
        # BMP: the left pixel is in the high nibble
        rows.append([value for byte in source for value in (byte >> 4, byte & 0x0F)])
    return rows, palette


def cover_pixels(data: bytes) -> list[list[int]]:
    """Palette indices of a 128x96 8 bpp bottom-up cover, rows from the top."""
    offset = struct.unpack_from("<I", data, 0x0A)[0]
    rows = [list(data[offset + y * 128:offset + (y + 1) * 128]) for y in range(96)]
    return list(reversed(rows))


def cover_palette(data: bytes) -> list[tuple[int, int, int]]:
    return [(data[0x36 + i * 4 + 2], data[0x36 + i * 4 + 1], data[0x36 + i * 4]) for i in range(256)]


class CoverFormatTests(unittest.TestCase):
    def test_box_cover(self):
        box = Image.open(io.BytesIO(png_bytes((512, 459), (0, 0, 255))))
        cover = imaging.to_cover(box, "boxart", "nds")
        self.assertEqual(cover.size, (128, 96))
        data = imaging.to_cover_bmp(cover)
        fields = header_fields(data)
        self.assertEqual((fields["width"], fields["height"], fields["bpp"], fields["compression"]), (128, 96, 8, 0))
        self.assertEqual(fields["dib_size"], 40)
        self.assertEqual(fields["colors"], 256)
        self.assertEqual(len(data), fields["pixel_offset"] + 128 * 96)
        self.assertTrue(pico_validate(data, 128, 96, 8))
        self.assertEqual(imaging.validate_cover_bmp(data), imaging.Validation())
        self.assertTrue(imaging.is_tool_bmp(data))
        palette = cover_palette(data)
        rows = cover_pixels(data)
        # the right 22 pixels are black padding, the box (nearly the same aspect) fills the rest
        for row in rows:
            self.assertEqual({palette[index] for index in row[106:]}, {(0, 0, 0)})
        self.assertNotEqual(palette[rows[48][2]], imaging.COVER_BACKGROUND)
        # realistic: many colors, dithered
        self.assertGreater(len({index for row in rows for index in row}), 32)

    def test_wide_screen_gets_bars(self):
        screen = Image.new("RGB", (240, 160), (255, 0, 0))
        cover = imaging.to_cover(screen, "title", "gba")
        self.assertEqual(cover.getpixel((50, 2)), imaging.COVER_BACKGROUND)
        self.assertEqual(cover.getpixel((50, 48)), (255, 0, 0))
        self.assertEqual(cover.getpixel((110, 48)), (0, 0, 0))
        self.assertTrue(imaging.validate_cover_bmp(imaging.to_cover_bmp(cover)).ok)

    def test_all_sizes_and_types(self):
        for size in [(512, 460), (512, 512), (300, 600), (256, 384), (384, 256), (240, 160), (10, 10)]:
            for art_type in ("title", "snap", "boxart", "custom"):
                with self.subTest(size=size, art_type=art_type):
                    cover = imaging.to_cover(Image.new("RGB", size, (9, 9, 9)), art_type, "nds")
                    self.assertEqual(cover.size, (128, 96))

    def test_validator_rejects_what_pico_rejects(self):
        buffer = io.BytesIO()
        Image.new("RGB", (128, 96)).quantize(16).save(buffer, "BMP")
        data = bytearray(buffer.getvalue())
        # Pillow writes 16 palette entries for 16 colors: Pico wants 0 or 256
        self.assertEqual(pico_validate(bytes(data), 128, 96, 8), imaging.validate_cover_bmp(bytes(data)).ok)
        self.assertFalse(imaging.validate_cover_bmp(imaging.to_ds_bmp(Image.new("RGB", (64, 64)))).ok)


class IconFormatTests(unittest.TestCase):
    def make_icon(self) -> Image.Image:
        box = Image.new("RGB", (512, 512), (20, 40, 160))
        draw = ImageDraw.Draw(box)
        draw.rectangle((150, 80, 360, 300), fill=(250, 200, 30))
        draw.ellipse((200, 120, 310, 230), fill=(220, 30, 30))
        return imaging.to_icon(box, "boxart", "gba")

    def test_layout_accepted_by_pico(self):
        icon = self.make_icon()
        self.assertEqual((icon.size, icon.mode), ((32, 32), "RGBA"))
        data = imaging.to_icon_bmp(icon)
        fields = header_fields(data)
        self.assertEqual((fields["width"], fields["height"], fields["bpp"], fields["compression"]), (32, 32, 4, 0))
        self.assertEqual((fields["dib_size"], fields["colors"], fields["pixel_offset"]), (40, 16, 118))
        self.assertEqual(len(data), 118 + 512)
        self.assertEqual(imaging.validate_icon_bmp(data), imaging.Validation())
        self.assertTrue(imaging.is_tool_bmp(data))
        loaded = pico_load_icon(data)
        self.assertIsNotNone(loaded)
        rows, palette = loaded
        self.assertEqual(rows, imaging.decode_icon_bmp(data)[0])
        # rounded corners: transparent (index 0), the middle is opaque
        for x, y in ((0, 0), (31, 0), (0, 31), (31, 31)):
            self.assertEqual(rows[y][x], 0)
        self.assertNotEqual(rows[16][16], 0)
        # minimalist: at most 15 colors besides the transparent one
        self.assertLessEqual(len({index for row in rows for index in row} - {0}), 15)

    def test_rows_and_nibbles(self):
        icon = Image.new("RGBA", (32, 32), (255, 0, 0, 255))
        icon.paste((0, 0, 255, 255), (0, 16, 32, 32))
        icon.paste((0, 255, 0, 255), (0, 0, 1, 16))
        icon.putpixel((31, 31), (0, 0, 0, 0))
        rows, palette = pico_load_icon(imaging.to_icon_bmp(icon))
        self.assertEqual(palette[rows[5][10]], (255, 0, 0))
        self.assertEqual(palette[rows[20][10]], (0, 0, 255))
        # the left column (high nibble of the first byte) stays green
        self.assertEqual(palette[rows[5][0]], (0, 255, 0))
        self.assertEqual(rows[31][31], 0)

    def test_validator_rejects_other_formats(self):
        buffer = io.BytesIO()
        Image.new("RGB", (32, 32)).quantize(256).save(buffer, "BMP")
        self.assertIn("8 bit pro Pixel statt 4", imaging.validate_icon_bmp(buffer.getvalue()).errors)
        self.assertFalse(imaging.validate_icon_bmp(b"BM" + b"\0" * 200).ok)
        data = bytearray(imaging.to_icon_bmp(self.make_icon()))
        self.assertFalse(imaging.validate_icon_bmp(bytes(data[:-10])).ok)
        struct.pack_into("<I", data, 0x2E, 8)
        self.assertIsNone(pico_load_icon(bytes(data)))
        self.assertFalse(imaging.validate_icon_bmp(bytes(data)).ok)

    def test_crop_finds_the_detail_near_the_anchor(self):
        image = Image.new("RGB", (400, 300), (30, 30, 120))
        detail = Image.open(io.BytesIO(png_bytes((90, 90), (200, 200, 0))))
        image.paste(detail, (60, 40))
        crop = imaging.salient_square(image, 0.5, (0.5, 0.3))
        self.assertEqual(crop.size, (150, 150))
        # the detailed patch is inside the crop
        self.assertIn((200, 200, 0), [color for _, color in crop.getcolors(1 << 16)])


class ColorLimitTests(unittest.TestCase):
    """Tiles and icons are reduced without dithering: few colors in flat areas, no noise."""

    @staticmethod
    def gradient(size: int) -> Image.Image:
        image = Image.new("RGB", (size, size))
        image.putdata([(x * 255 // size, 80, 255 - y * 255 // size) for y in range(size) for x in range(size)])
        return image

    def assert_no_dither(self, rows: list[list[int]], palette):
        # a horizontal gradient: without dithering every row goes through its colors once, in order
        for row in rows:
            changes = sum(1 for a, b in zip(row, row[1:]) if a != b)
            self.assertLessEqual(changes + 1, len(set(row)))
            reds = [palette[index][0] for index in row]
            self.assertEqual(reds, sorted(reds))

    def test_tile(self):
        data = imaging.to_tile_bmp(self.gradient(64))
        self.assertTrue(imaging.validate_bmp(data, (64, 64)).ok)
        offset = header_fields(data)["pixel_offset"]
        pixels = data[offset:]
        self.assertNotIn(0, pixels)
        self.assertLessEqual(len(set(pixels)), imaging.TILE_COLORS)
        rows = [list(pixels[y * 64:(y + 1) * 64]) for y in range(64)]
        palette = cover_palette(data)
        self.assert_no_dither(rows, palette)

    def test_icon(self):
        rows, palette = pico_load_icon(imaging.to_icon_bmp(self.gradient(32).convert("RGBA")))
        self.assertLessEqual(len({index for row in rows for index in row}), 15)
        self.assertNotIn(0, {index for row in rows for index in row})
        self.assert_no_dither(rows, palette)

    def test_specks_are_removed(self):
        pixels = bytearray(b"\x01" * 25)
        pixels[12] = 2
        self.assertEqual(imaging.remove_specks(bytes(pixels), 5, 5), b"\x01" * 25)
        # lines stay
        line = bytes([1, 1, 2, 1, 1] * 5)
        self.assertEqual(imaging.remove_specks(line, 5, 5), line)


class PathTests(unittest.TestCase):
    def test_user_paths(self):
        self.assertEqual(user_file_path("/Games/GBA/Kirby.gba", False, "/_pico/covers/user"),
                         "/_pico/covers/user/Kirby.gba.bmp")
        self.assertEqual(user_file_path("fat:/Kirby.gba", False, "/_pico/icons/user"), "/_pico/icons/user/Kirby.gba.bmp")
        self.assertIsNone(user_file_path("/Games/GBA", True, "/_pico/covers/user"))
        self.assertIsNotNone(user_file_path("/" + "a" * 247 + ".gba", False, "/_pico/covers/user"))
        self.assertIsNone(user_file_path("/" + "a" * 248 + ".gba", False, "/_pico/covers/user"))


class CardCoverIconTests(CardTestCase):
    def setUp(self):
        super().setUp()
        self.game("Mario Kart DS.nds", "MARIOKART DS", "AMCP", "Mario Kart DS\nNintendo")
        self.game("Unknown Game.nds", "UNKNOWN", "ZZZZ")
        write(self.sd / "Games" / "GBA" / "Golden Sun.gba", gba_rom("GOLDEN_SUN_A", "AGSD"))
        self.files.update({
            thumb(DS, "Named_Boxarts", MARIO_KART): png_bytes((512, 459), (0, 0, 255)),
            thumb(GBA, "Named_Boxarts", GOLDEN_SUN): png_bytes((512, 512), (0, 200, 200)),
            thumb(GBA, "Named_Titles", GOLDEN_SUN): png_bytes((240, 160), (200, 0, 0)),
        })

    def cover(self, name: str):
        return self.sd / "_pico" / "covers" / "user" / (name + ".bmp")

    def icon(self, name: str):
        return self.sd / "_pico" / "icons" / "user" / (name + ".bmp")

    def test_covers_and_icons(self):
        stats, _ = self.run_updater()
        self.assertEqual(stats.total_errors, 0, self.logs)
        self.assertEqual((stats.covers.downloaded, stats.covers.missing), (2, 1))
        # NDS games show their banner icon: only Golden Sun and the NDS game without an icon need one,
        # and nothing is found for the unknown game
        self.assertEqual((stats.small_icons.downloaded, stats.small_icons.missing), (1, 1))
        for name in ("Mario Kart DS.nds", "Golden Sun.gba"):
            with self.subTest(name=name):
                self.assertTrue(imaging.validate_cover_bmp(self.cover(name).read_bytes()).ok)
        self.assertIsNotNone(pico_load_icon(self.icon("Golden Sun.gba").read_bytes()))
        self.assertFalse(self.icon("Mario Kart DS.nds").exists())
        self.assertFalse(self.icon("Unknown Game.nds").exists())
        self.assertFalse(self.cover("Unknown Game.nds").exists())
        # the box is preferred: the Golden Sun cover is cyan, not red (the title screen)
        image = Image.open(self.cover("Golden Sun.gba")).convert("RGB").crop((0, 0, 106, 96))
        red, green, blue = image.resize((1, 1), Image.Resampling.BOX).getpixel((0, 0))
        self.assertGreater(blue, red)
        manifest = json.loads((self.sd / "_pico" / "covers" / "user" / ".dsrunner-art.json").read_text("utf-8"))
        self.assertTrue(manifest["covers"]["/_pico/covers/user/golden sun.gba.bmp"]["source"].startswith(
            "libretro:boxart:"))
        self.assertIn("Cover:", stats.summary())
        self.assertIn("Icons:", stats.summary())
        # covers and icons that are missing only show in the details
        self.assertNotIn("kein Cover", self.messages("warn"))

        # second run: nothing new
        stats, _ = self.run_updater()
        self.assertEqual((stats.covers.downloaded, stats.covers.unchanged), (0, 2))
        self.assertEqual((stats.small_icons.downloaded, stats.small_icons.unchanged), (0, 1))

    def test_switched_off(self):
        stats, _ = self.run_updater(covers=False, icons=False)
        self.assertIsNone(stats.covers)
        self.assertIsNone(stats.small_icons)
        self.assertFalse((self.sd / "_pico" / "covers").exists())
        self.assertFalse((self.sd / "_pico" / "icons").exists())

    def test_hand_placed_files_stay(self):
        own_cover = write(self.cover("Golden Sun.gba"), b"my own cover")
        own_icon = write(self.icon("Golden Sun.gba"), b"my own icon")
        stats, _ = self.run_updater(force=True)
        self.assertEqual(own_cover.read_bytes(), b"my own cover")
        self.assertEqual(own_icon.read_bytes(), b"my own icon")
        self.assertEqual((stats.covers.foreign, stats.small_icons.foreign), (1, 1))
        self.assertEqual(stats.covers.foreign_bad, 1)
        self.assertIn("von Hand abgelegtes Cover", self.messages("warn"))

        stats, _ = self.run_updater(replace_manual=True)
        self.assertEqual((stats.covers.replaced_manual, stats.small_icons.replaced_manual), (1, 1))
        self.assertTrue(imaging.validate_cover_bmp(own_cover.read_bytes()).ok)
        self.assertIsNotNone(pico_load_icon(own_icon.read_bytes()))
        stats, _ = self.run_updater()
        self.assertEqual((stats.covers.foreign, stats.covers.unchanged), (0, 2))

    def test_lost_manifest_and_clean(self):
        self.run_updater()
        (self.sd / "_pico" / "covers" / "user" / ".dsrunner-art.json").unlink()
        stats, _ = self.run_updater()
        self.assertEqual((stats.covers.foreign, stats.covers.unchanged), (0, 2))
        (self.sd / "Games" / "GBA" / "Golden Sun.gba").unlink()
        stats, _ = self.run_updater(clean=True)
        self.assertEqual((stats.covers.removed, stats.small_icons.removed), (1, 1))
        self.assertFalse(self.cover("Golden Sun.gba").exists())
        self.assertFalse(self.icon("Golden Sun.gba").exists())
        self.assertTrue(self.cover("Mario Kart DS.nds").exists())

    def test_same_file_name_twice(self):
        write(self.sd / "Games" / "GBA" / "Rennen" / "Golden Sun.gba", gba_rom("GOLDEN_SUN_A", "AGSD"))
        stats, _ = self.run_updater()
        self.assertEqual(stats.total_errors, 0, self.logs)
        self.assertEqual(stats.covers.downloaded, 2)
        self.assertEqual(stats.tiles.downloaded, 3)

    def test_check_card(self):
        self.run_updater()
        messages = []
        self.assertEqual(check_card(self.sd, lambda message, level="info": messages.append(message)), 0)
        self.assertIn("2 Cover geprüft, 0 unbrauchbar.", messages)
        self.assertIn("1 Icons geprüft, 0 unbrauchbar.", messages)
        write(self.icon("Wrong.gba"), self.cover("Golden Sun.gba").read_bytes())
        self.assertEqual(check_card(self.sd, lambda message, level="info": messages.append(message)), 1)
        self.assertTrue(any("_pico/icons/user/Wrong.gba.bmp" in message for message in messages))

    def test_old_tile_style_is_redone(self):
        self.run_updater()
        path = self.sd / "_dsrunner" / "tiles" / ".dsrunner-art.json"
        data = json.loads(path.read_text("utf-8"))
        for record in data["tiles"].values():
            record.pop("style", None)
        path.write_text(json.dumps(data), "utf-8")
        stats, _ = self.run_updater()
        self.assertEqual((stats.tiles.downloaded, stats.downloaded), (2, 0))
        stats, _ = self.run_updater()
        self.assertEqual(stats.tiles.downloaded, 0)

    def test_nds_without_icon_gets_one(self):
        self.files[thumb(DS, "Named_Boxarts", "Mario Party DS (Europe) (En,Fr,De,Es,It)")] = png_bytes((512, 459))
        write(self.sd / "Games" / "NDS" / "Party.nds", nds_rom("MARIO PARTY", "AMPP", "Mario Party DS\nNintendo",
                                                                  icon=False))
        stats, _ = self.run_updater()
        self.assertTrue(self.icon("Party.nds").exists())


if __name__ == "__main__":
    unittest.main()
