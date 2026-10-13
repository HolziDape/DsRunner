"""Game name normalization and matching against No-Intro names."""
from __future__ import annotations

import difflib
import re
import unicodedata
from dataclasses import dataclass, field
from typing import Iterable, Optional

# characters libretro replaces with "_" in thumbnail file names
_THUMBNAIL_UNSAFE = re.compile(r'[&*/:`<>?\\|"]')
_TAG = re.compile(r"\s*[\(\[][^\)\]]*[\)\]]")
_PAREN_GROUPS = re.compile(r"\(([^)]*)\)")
# scene release numbers in front of the name: "0123 - Mario Kart DS"
_RELEASE_NUMBER = re.compile(r"^\s*\d{3,5}\s*[-_.]\s*")
_ARTICLES = ("the", "a", "an", "der", "die", "das", "le", "la", "les", "el", "los", "il")
_TRAILING_ARTICLE = re.compile(r"^(.*?),\s*(" + "|".join(_ARTICLES) + r")\b(.*)$", re.IGNORECASE)

# No-Intro region names and the short forms found in file names
REGION_ALIASES = {
    "europe": "Europe", "eur": "Europe", "e": "Europe", "eu": "Europe", "pal": "Europe",
    "germany": "Germany", "ger": "Germany", "g": "Germany", "deu": "Germany",
    "usa": "USA", "u": "USA", "us": "USA", "ntsc": "USA",
    "japan": "Japan", "jpn": "Japan", "j": "Japan", "jp": "Japan",
    "world": "World", "w": "World",
    "france": "France", "fra": "France", "f": "France",
    "spain": "Spain", "spa": "Spain", "s": "Spain",
    "italy": "Italy", "ita": "Italy", "i": "Italy",
    "netherlands": "Netherlands", "hol": "Netherlands",
    "united kingdom": "United Kingdom", "uk": "United Kingdom",
    "australia": "Australia", "aus": "Australia", "a": "Australia",
    "korea": "Korea", "kor": "Korea", "k": "Korea",
    "china": "China", "chn": "China", "c": "China",
    "canada": "Canada", "brazil": "Brazil", "sweden": "Sweden", "scandinavia": "Scandinavia",
    "asia": "Asia",
}
# short forms that may stand at the end of a file name without brackets ("Cars GER.nds")
_BARE_REGION_SUFFIXES = {"eur", "ger", "deu", "usa", "jpn", "fra", "spa", "ita", "pal", "ntsc", "uk"}

REGION_PREFERENCES = {
    "de": ["Germany", "Europe", "World", "United Kingdom", "USA", "Australia", "Canada", "Japan"],
    "eu": ["Europe", "World", "United Kingdom", "Germany", "France", "USA", "Australia", "Japan"],
    "us": ["USA", "World", "Canada", "Europe", "Australia", "United Kingdom", "Japan"],
    "jp": ["Japan", "World", "Asia", "USA", "Europe"],
}

# tags that make a dump a bad choice for artwork lookups
_BAD_TAGS = ("beta", "proto", "demo", "sample", "kiosk", "virtual console", "debug", "pirate", "unl",
             "aftermarket", "hack", "program", "test")

# region letter of NDS / GBA game codes
GAME_CODE_REGIONS = {
    "E": "USA", "O": "USA", "P": "Europe", "D": "Germany", "F": "France", "S": "Spain", "I": "Italy",
    "H": "Netherlands", "J": "Japan", "K": "Korea", "C": "China", "U": "Australia", "X": "Europe",
    "Y": "Europe", "Z": "Europe", "V": "Europe", "W": "Europe", "A": "World",
}


def thumbnail_file_name(name: str) -> str:
    """The libretro thumbnail file name (without .png) for a No-Intro name."""
    return _THUMBNAIL_UNSAFE.sub("_", name)


def base_title(name: str) -> str:
    """Name without bracket tags, release numbers and bare region suffixes."""
    text = _TAG.sub("", name)
    text = _RELEASE_NUMBER.sub("", text)
    text = text.replace("_", " ").strip(" .-")
    words = text.split()
    while len(words) > 1 and words[-1].lower() in _BARE_REGION_SUFFIXES:
        words.pop()
    return " ".join(words)


def normalize(name: str) -> str:
    """Comparable form of a game name: lower case ASCII words, no tags, articles moved to the front."""
    text = base_title(name)
    text = unicodedata.normalize("NFKD", text)
    text = "".join(ch for ch in text if not unicodedata.combining(ch))
    # "Legend of Zelda, The - Minish Cap" -> "The Legend of Zelda - Minish Cap"
    match = _TRAILING_ARTICLE.match(text)
    if match:
        text = f"{match.group(2)} {match.group(1)}{match.group(3)}"
    text = text.lower().replace("&", " and ").replace("+", " plus ")
    # "Link's" -> "links"
    text = re.sub(r"['’`]", "", text)
    text = re.sub(r"[^a-z0-9]+", " ", text).strip()
    words = text.split()
    if len(words) > 1 and words[0] in _ARTICLES:
        words = words[1:]
    return " ".join(words)


def regions_of(name: str) -> list[str]:
    """No-Intro regions named in the tags of a name ("(USA, Europe)" -> ["USA", "Europe"])."""
    regions: list[str] = []
    for group in re.findall(r"[\(\[]([^\)\]]*)[\)\]]", name):
        for part in group.split(","):
            region = REGION_ALIASES.get(part.strip().lower())
            if region and region not in regions:
                regions.append(region)
    words = _TAG.sub("", name).replace("_", " ").split()
    if words and words[-1].lower() in _BARE_REGION_SUFFIXES:
        region = REGION_ALIASES.get(words[-1].lower())
        if region and region not in regions:
            regions.append(region)
    return regions


def _numbers(text: str) -> set[str]:
    return set(re.findall(r"\d+", text))


def tag_penalty(name: str) -> int:
    """Higher for dumps whose artwork is a bad choice (betas, demos, ...)."""
    tags = " ".join(_PAREN_GROUPS.findall(name)).lower()
    return sum(1 for tag in _BAD_TAGS if tag in tags)


def _revision(name: str) -> int:
    tags = " ".join(_PAREN_GROUPS.findall(name)).lower()
    return 1 if re.search(r"\b(rev|v\d)", tags) else 0


def region_rank(name: str, preferred: list[str]) -> int:
    regions = regions_of(name)
    ranks = [preferred.index(region) for region in regions if region in preferred]
    if ranks:
        return min(ranks)
    return len(preferred) + (0 if regions else 1)


def sort_names(names: Iterable[str], preferred: list[str]) -> list[str]:
    """Orders No-Intro names: good dumps first, then by region preference, first release, shortest."""
    return sorted(set(names), key=lambda name: (tag_penalty(name), region_rank(name, preferred), _revision(name),
                                                len(name), name))


@dataclass
class NameIndex:
    """No-Intro names of one system, searchable by normalized title."""
    by_title: dict[str, list[str]] = field(default_factory=dict)
    # title without subtitle ("Donkey Kong Country 2" for "... 2 - Diddy's Kong Quest") -> titles
    by_main_title: dict[str, set[str]] = field(default_factory=dict)

    @classmethod
    def build(cls, names: Iterable[str]) -> "NameIndex":
        index = cls()
        for name in names:
            key = normalize(name)
            if not key:
                continue
            index.by_title.setdefault(key, []).append(name)
            base = base_title(name)
            if " - " in base:
                main = normalize(base.split(" - ", 1)[0])
                if main and main != key:
                    index.by_main_title.setdefault(main, set()).add(key)
        return index

    def __len__(self) -> int:
        return len(self.by_title)

    def exact(self, query: str) -> list[str]:
        return list(self.by_title.get(normalize(query), []))

    def fuzzy(self, query: str, cutoff: float = 0.86) -> tuple[list[str], float]:
        """Names of the closest title. Numbers must match exactly (FIFA 08 is not FIFA 09)."""
        key = normalize(query)
        if not key:
            return [], 0.0
        if key in self.by_title:
            return list(self.by_title[key]), 1.0
        numbers = _numbers(key)
        best: tuple[float, Optional[str]] = (0.0, None)
        matcher = difflib.SequenceMatcher(autojunk=False)
        matcher.set_seq2(key)
        for title in self.by_title:
            if _numbers(title) != numbers:
                continue
            matcher.set_seq1(title)
            if matcher.real_quick_ratio() < cutoff or matcher.quick_ratio() < cutoff:
                continue
            ratio = matcher.ratio()
            if ratio > best[0]:
                best = (ratio, title)
        if best[1] is not None and best[0] >= cutoff:
            return list(self.by_title[best[1]]), best[0]
        # the query is the title without its subtitle, and only one game has that title
        titles = self.by_main_title.get(key)
        if titles and len(titles) == 1 and len(key.split()) >= 2:
            title = next(iter(titles))
            return list(self.by_title[title]), len(key) / len(title)
        return self._contained(key, numbers)

    def _contained(self, key: str, numbers: set[str], min_share: float = 0.6) -> tuple[list[str], float]:
        """Titles that contain every word of the query (a word may be the start of a longer one:
        "Pokemon Rot" -> "Pokemon - Rote Edition", "Donkey Kong Country 2" -> "... 2 - Diddy's Kong Quest").
        The query must make up most of the title, and the best title must be unambiguous."""
        words = key.split()
        if len(words) < 2:
            return [], 0.0
        scores: list[tuple[float, str]] = []
        for title in self.by_title:
            if _numbers(title) != numbers:
                continue
            title_words = title.split()
            if len(title_words) * min_share > len(words):
                continue
            remaining = list(title_words)
            for word in words:
                match = next((candidate for candidate in remaining if candidate == word or
                              (len(word) >= 3 and candidate.startswith(word))), None)
                if match is None:
                    break
                remaining.remove(match)
            else:
                scores.append((len(words) / len(title_words), title))
        if not scores:
            return [], 0.0
        scores.sort(reverse=True)
        if len(scores) > 1 and scores[1][0] == scores[0][0]:
            return [], 0.0
        return list(self.by_title[scores[0][1]]), scores[0][0]
