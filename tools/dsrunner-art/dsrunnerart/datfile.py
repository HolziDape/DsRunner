"""Reader for the clrmamepro dat files of libretro-database (metadat/no-intro/<System>.dat)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from .names import NameIndex

_NAME = re.compile(r'^\s*name\s+"(.*)"\s*$')
_SERIAL = re.compile(r'^\s*serial\s+"(.*)"\s*$')
_ROM_CRC = re.compile(r'\bcrc\s+([0-9A-Fa-f]{8})\b')
_ROM_SERIAL = re.compile(r'\bserial\s+"([^"]*)"')


@dataclass
class DatGame:
    name: str
    serials: set[str] = field(default_factory=set)
    crcs: set[int] = field(default_factory=set)


@dataclass
class Dat:
    games: list[DatGame]
    by_serial: dict[str, list[str]]
    by_crc: dict[int, list[str]]
    index: NameIndex

    @classmethod
    def parse(cls, text: str) -> "Dat":
        games: list[DatGame] = []
        current: DatGame | None = None
        for line in text.splitlines():
            stripped = line.strip()
            if stripped.startswith("game (") or stripped == "game (":
                current = DatGame("")
                continue
            if current is None:
                continue
            if stripped == ")":
                if current.name:
                    games.append(current)
                current = None
                continue
            if stripped.startswith("rom ("):
                for crc in _ROM_CRC.findall(stripped):
                    current.crcs.add(int(crc, 16))
                for serial in _ROM_SERIAL.findall(stripped):
                    current.serials.add(serial.strip().upper())
                continue
            match = _NAME.match(line)
            if match and not current.name:
                current.name = match.group(1)
                continue
            match = _SERIAL.match(line)
            if match:
                current.serials.add(match.group(1).strip().upper())
        by_serial: dict[str, list[str]] = {}
        by_crc: dict[int, list[str]] = {}
        for game in games:
            for serial in game.serials:
                by_serial.setdefault(serial, []).append(game.name)
            for crc in game.crcs:
                by_crc.setdefault(crc, []).append(game.name)
        return cls(games, by_serial, by_crc, NameIndex.build(game.name for game in games))

    def names_for_serial(self, game_code: str) -> list[str]:
        """No-Intro names for an NDS/GBA game code. GBA dats sometimes use "AGB-XXXX-..." serials."""
        code = game_code.upper()
        names = list(self.by_serial.get(code, []))
        if names:
            return names
        for prefix in ("AGB-", "NTR-", "TWL-"):
            for serial, serial_names in self.by_serial.items():
                if serial.startswith(prefix + code):
                    names.extend(serial_names)
        return list(dict.fromkeys(names))
