"""DS buttons/touch -> keyboard/mouse events for the page.

A mapping file (mappings/*.json) looks like:

    {
      "name": "Standard",
      "keys": {"UP": "ArrowUp", "A": ["Space", "Enter"], "Y": "MouseLeft", ...},
      "touch": "click"            // "click", "drag" (= click), "move" or "off"
    }

Key names are Playwright/DOM key names ("ArrowUp", "Space", "KeyW", "Shift",
"Enter", "a", ...). "MouseLeft"/"MouseRight"/"MouseMiddle" press that mouse button
at the last touch position.
"""
from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path

from .protocol import KEY_BITS

MOUSE_KEYS = {"MouseLeft": "left", "MouseRight": "right", "MouseMiddle": "middle"}
TOUCH_MODES = ("click", "drag", "move", "off")


@dataclass
class Event:
    kind: str           # "keydown", "keyup", "mousemove", "mousedown", "mouseup"
    key: str = ""       # key name or mouse button
    x: float = 0.0
    y: float = 0.0


@dataclass
class Mapping:
    name: str = "Standard"
    keys: dict[str, list[str]] = field(default_factory=dict)
    touch: str = "click"

    @classmethod
    def from_dict(cls, d: dict) -> "Mapping":
        keys = {}
        for ds_key, val in d.get("keys", {}).items():
            k = ds_key.upper()
            if k not in KEY_BITS or k == "TOUCH":
                raise ValueError(f"unknown DS key {ds_key!r} (allowed: {', '.join(KEY_BITS)})")
            vals = [val] if isinstance(val, str) else list(val)
            keys[k] = [v for v in vals if v]
        touch = d.get("touch", "click")
        if touch not in TOUCH_MODES:
            raise ValueError(f"touch must be one of {TOUCH_MODES}")
        return cls(d.get("name", "?"), keys, touch)

    @classmethod
    def load(cls, path: str | Path) -> "Mapping":
        p = Path(path)
        if not p.exists():
            alt = Path(__file__).resolve().parent.parent / "mappings" / p.name
            if alt.exists():
                p = alt
        return cls.from_dict(json.loads(p.read_text(encoding="utf-8")))


class InputTranslator:
    """Turns the 60 Hz DS input state into page events (only on changes)."""

    def __init__(self, mapping: Mapping, page_w: int, page_h: int, screen_w: int = 256,
                 screen_h: int = 192, touch_y_offset: int = 0, total_h: int = 192):
        self.m = mapping
        self.page_w, self.page_h = page_w, page_h
        self.screen_w = screen_w
        self.touch_y_offset = touch_y_offset   # dual mode: bottom screen starts at 192
        self.total_h = total_h                 # height of the streamed picture (192/384)
        self.keys = 0
        self.mx = page_w / 2
        self.my = page_h / 2
        self.touching = False
        self.held: dict[str, int] = {}         # target key -> number of DS keys holding it

    def touch_to_page(self, tx: int, ty: int) -> tuple[float, float]:
        x = (tx + 0.5) * self.page_w / self.screen_w
        y = (ty + self.touch_y_offset + 0.5) * self.page_h / self.total_h
        return min(max(x, 0), self.page_w - 1), min(max(y, 0), self.page_h - 1)

    def _press(self, target: str, down: bool, out: list[Event]) -> None:
        n = self.held.get(target, 0)
        if down:
            self.held[target] = n + 1
            if n:
                return
        else:
            if n == 0:
                return
            self.held[target] = n - 1
            if n > 1:
                return
        if target in MOUSE_KEYS:
            out.append(Event("mousedown" if down else "mouseup", MOUSE_KEYS[target], self.mx, self.my))
        else:
            out.append(Event("keydown" if down else "keyup", target))

    def update(self, keys: int, tx: int, ty: int) -> list[Event]:
        out: list[Event] = []
        touch_bit = 1 << KEY_BITS["TOUCH"]
        touching = bool(keys & touch_bit)
        if touching and self.m.touch != "off":
            x, y = self.touch_to_page(tx, ty)
            if (x, y) != (self.mx, self.my) or not self.touching:
                self.mx, self.my = x, y
                out.append(Event("mousemove", "", x, y))
        if self.m.touch in ("click", "drag"):
            if touching and not self.touching:
                out.append(Event("mousedown", "left", self.mx, self.my))
            elif not touching and self.touching:
                out.append(Event("mouseup", "left", self.mx, self.my))
        self.touching = touching

        changed = keys ^ self.keys
        for name, bit in KEY_BITS.items():
            if name == "TOUCH" or not changed & (1 << bit):
                continue
            down = bool(keys & (1 << bit))
            for target in self.m.keys.get(name, []):
                self._press(target, down, out)
        self.keys = keys
        return out

    def release_all(self) -> list[Event]:
        """Events that let go of everything (DS disconnected)."""
        out = self.update(0, 0, 0)
        for target, n in list(self.held.items()):
            if n:
                self.held[target] = 1
                self._press(target, False, out)
        return out
