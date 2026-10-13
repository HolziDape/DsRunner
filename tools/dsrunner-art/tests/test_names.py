import unittest

from tests.helpers import DAT_GB, DAT_NDS

from dsrunnerart.datfile import Dat
from dsrunnerart.names import (REGION_PREFERENCES, NameIndex, base_title, normalize, regions_of, sort_names,
                               thumbnail_file_name)


class NormalizeTests(unittest.TestCase):
    def test_tags_and_punctuation(self):
        self.assertEqual(normalize("Mario Kart DS (Europe) (En,Fr,De,Es,It)"), "mario kart ds")
        self.assertEqual(normalize("Mario Kart DS [!]"), "mario kart ds")
        self.assertEqual(normalize("mario_kart_ds"), "mario kart ds")
        self.assertEqual(normalize("0042 - New Super Mario Bros (E)"), "new super mario bros")
        self.assertEqual(normalize("New Super Mario Bros."), "new super mario bros")
        self.assertEqual(normalize("Cars GER"), "cars")

    def test_articles_and_accents(self):
        self.assertEqual(normalize("Legend of Zelda, The - The Minish Cap (Europe)"),
                         normalize("The Legend of Zelda: The Minish Cap"))
        self.assertEqual(normalize("Pokémon - Platin-Edition"), "pokemon platin edition")
        self.assertEqual(normalize("Mario & Luigi - Partners in Time"), "mario and luigi partners in time")

    def test_base_title(self):
        self.assertEqual(base_title("Golden Sun (Germany) (Rev 1)"), "Golden Sun")
        self.assertEqual(base_title("Cars GER"), "Cars")

    def test_thumbnail_file_name(self):
        self.assertEqual(thumbnail_file_name('Mario & Luigi: "Partners" <1/2>?*|\\`'), "Mario _ Luigi_ _Partners_ _1_2______")
        self.assertEqual(thumbnail_file_name(thumbnail_file_name("A & B")), "A _ B")

    def test_regions(self):
        self.assertEqual(regions_of("Mario Kart DS (USA, Australia)"), ["USA", "Australia"])
        self.assertEqual(regions_of("Golden Sun (G)"), ["Germany"])
        self.assertEqual(regions_of("Cars GER"), ["Germany"])
        self.assertEqual(regions_of("Mario Kart DS (Europe) (En,Fr,De,Es,It)"), ["Europe"])
        self.assertEqual(regions_of("Tetris"), [])


class SortTests(unittest.TestCase):
    def test_region_preference(self):
        names = ["Mario Kart DS (USA, Australia)", "Mario Kart DS (Europe) (En,Fr,De,Es,It)", "Mario Kart DS (Japan)"]
        self.assertEqual(sort_names(names, REGION_PREFERENCES["de"])[0], "Mario Kart DS (Europe) (En,Fr,De,Es,It)")
        self.assertEqual(sort_names(names, REGION_PREFERENCES["us"])[0], "Mario Kart DS (USA, Australia)")
        self.assertEqual(sort_names(names, REGION_PREFERENCES["jp"])[0], "Mario Kart DS (Japan)")

    def test_bad_dumps_last(self):
        names = ["Game (Europe) (Demo)", "Game (USA) (Beta)", "Game (Japan)", "Game (Europe) (Rev 1)"]
        ordered = sort_names(names, REGION_PREFERENCES["de"])
        self.assertEqual(ordered[0], "Game (Europe) (Rev 1)")
        self.assertEqual(ordered[1], "Game (Japan)")


class NameIndexTests(unittest.TestCase):
    def setUp(self):
        self.index = NameIndex.build([
            "Mario Kart DS (Europe) (En,Fr,De,Es,It)", "Mario Kart DS (USA, Australia)",
            "Mario Party DS (Europe)", "FIFA 08 (Europe)", "FIFA 09 (Europe)",
            "Legend of Zelda, The - Phantom Hourglass (Europe)", "Pokemon - Platin-Edition (Germany)",
        ])

    def test_exact(self):
        self.assertEqual(len(self.index.exact("Mario Kart DS (E) [!]")), 2)
        self.assertEqual(self.index.exact("The Legend of Zelda - Phantom Hourglass"),
                         ["Legend of Zelda, The - Phantom Hourglass (Europe)"])
        self.assertEqual(self.index.exact("Pokémon Platin-Edition"), ["Pokemon - Platin-Edition (Germany)"])

    def test_fuzzy(self):
        names, ratio = self.index.fuzzy("Mario")
        self.assertEqual(names, [])
        names, ratio = self.index.fuzzy("Mario Kart")
        self.assertEqual(len(names), 2)
        names, ratio = self.index.fuzzy("Legend of Zelda Phantom Hourglas")
        self.assertEqual(names, ["Legend of Zelda, The - Phantom Hourglass (Europe)"])
        self.assertGreater(ratio, 0.9)

    def test_fuzzy_keeps_similar_games_apart(self):
        self.assertEqual(self.index.fuzzy("Mario Party")[0], ["Mario Party DS (Europe)"])
        self.assertEqual(self.index.fuzzy("Mario Karts DX")[0][0][:13], "Mario Kart DS")
        names, _ = self.index.fuzzy("FIFA 2008")
        self.assertEqual(names, [])
        names, _ = self.index.fuzzy("FIFA O9")
        self.assertEqual(names, [])
        names, _ = self.index.fuzzy("FIFA 09!")
        self.assertEqual(names, ["FIFA 09 (Europe)"])


class DatTests(unittest.TestCase):
    def test_parse(self):
        dat = Dat.parse(DAT_NDS)
        self.assertEqual(len(dat.games), 4)
        self.assertEqual(dat.names_for_serial("amcp"), ["Mario Kart DS (Europe) (En,Fr,De,Es,It)"])
        self.assertEqual(dat.by_crc[0x94E8127E], ["Mario Kart DS (Europe) (En,Fr,De,Es,It)"])
        self.assertEqual(dat.index.exact("mario kart ds"),
                         ["Mario Kart DS (Europe) (En,Fr,De,Es,It)", "Mario Kart DS (USA, Australia)"])

    def test_dat_without_serials(self):
        dat = Dat.parse(DAT_GB)
        self.assertEqual(dat.by_serial, {})
        self.assertEqual(dat.by_crc[0x46DF91AD], ["Tetris (World) (Rev 1)"])

    def test_prefixed_serials(self):
        dat = Dat.parse('game (\n\tname "Game (USA)"\n\tserial "AGB-ABCE-USA"\n)\n')
        self.assertEqual(dat.names_for_serial("ABCE"), ["Game (USA)"])


if __name__ == "__main__":
    unittest.main()
