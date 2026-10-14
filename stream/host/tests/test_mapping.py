import unittest
from pathlib import Path

from dsstream.mapping import InputTranslator, Mapping
from dsstream.protocol import KEY_BITS


def bits(*names):
    v = 0
    for n in names:
        v |= 1 << KEY_BITS[n]
    return v


MAPDIR = Path(__file__).resolve().parent.parent / "mappings"


class MappingTest(unittest.TestCase):
    def setUp(self):
        self.m = Mapping.from_dict({
            "keys": {"UP": "ArrowUp", "A": ["Space", "Enter"], "B": "Space", "Y": "MouseLeft"},
            "touch": "click"})

    def kinds(self, evs):
        return [(e.kind, e.key) for e in evs]

    def test_press_release(self):
        tr = InputTranslator(self.m, 512, 384)
        self.assertEqual(self.kinds(tr.update(bits("UP"), 0, 0)), [("keydown", "ArrowUp")])
        self.assertEqual(tr.update(bits("UP"), 0, 0), [])      # held: nothing new
        self.assertEqual(self.kinds(tr.update(0, 0, 0)), [("keyup", "ArrowUp")])

    def test_shared_target_counts(self):
        tr = InputTranslator(self.m, 512, 384)
        self.assertEqual(self.kinds(tr.update(bits("A"), 0, 0)), [("keydown", "Space"), ("keydown", "Enter")])
        self.assertEqual(self.kinds(tr.update(bits("A", "B"), 0, 0)), [])       # Space already down
        self.assertEqual(self.kinds(tr.update(bits("B"), 0, 0)), [("keyup", "Enter")])
        self.assertEqual(self.kinds(tr.update(0, 0, 0)), [("keyup", "Space")])

    def test_touch_click_scaled(self):
        tr = InputTranslator(self.m, 512, 384)
        evs = tr.update(bits("TOUCH"), 128, 96)
        self.assertEqual(self.kinds(evs), [("mousemove", ""), ("mousedown", "left")])
        self.assertAlmostEqual(evs[0].x, 257.0)
        self.assertAlmostEqual(evs[0].y, 193.0)
        evs = tr.update(bits("TOUCH"), 130, 96)
        self.assertEqual(self.kinds(evs), [("mousemove", "")])
        self.assertEqual(self.kinds(tr.update(0, 0, 0)), [("mouseup", "left")])

    def test_dual_screen_offset(self):
        tr = InputTranslator(self.m, 512, 768, touch_y_offset=192, total_h=384)
        x, y = tr.touch_to_page(0, 0)
        self.assertAlmostEqual(y, (192.5) * 2)

    def test_mouse_key_and_release_all(self):
        tr = InputTranslator(self.m, 512, 384)
        tr.update(bits("TOUCH"), 10, 10)
        evs = tr.update(bits("TOUCH", "Y", "UP"), 10, 10)
        self.assertIn(("mousedown", "left"), self.kinds(evs))
        rel = self.kinds(tr.release_all())
        self.assertIn(("keyup", "ArrowUp"), rel)
        self.assertEqual(rel.count(("mouseup", "left")), 2)   # touch and Y

    def test_bundled_mappings_load(self):
        files = sorted(MAPDIR.glob("*.json"))
        self.assertTrue(files)
        for f in files:
            with self.subTest(f=f.name):
                m = Mapping.load(f)
                self.assertTrue(m.keys)
        self.assertEqual(Mapping.load("default.json").keys["UP"], ["ArrowUp"])

    def test_bad_key(self):
        with self.assertRaises(ValueError):
            Mapping.from_dict({"keys": {"Z": "a"}})


if __name__ == "__main__":
    unittest.main()
