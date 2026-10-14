"""Frame encoder/decoder for DsRunner Stream.

Format (details in stream/PROTOCOL.md): 8 bits per pixel with an RGB555 palette,
the picture is cut into 8x8 tiles, a delta frame only carries the tiles that differ
from its reference frame, the body is compressed as one LZ4 block. Everything is
chosen so the DS (67 MHz ARM9) decodes with byte copies and aligned 32-bit stores.
"""
from __future__ import annotations

import time
from dataclasses import dataclass

import numpy as np

from . import protocol as P

try:
    import lz4.block as _lz4
except ImportError:  # works without, frames are just bigger
    _lz4 = None

TILE = 8
HISTORY = 64


def rgb_to_555(rgb: np.ndarray) -> np.ndarray:
    """(…,3) uint8 -> (…) uint16 index into the 32768 RGB555 colors, DS bit order
    (red in bits 0-4)."""
    r = rgb[..., 0].astype(np.uint16) >> 3
    g = rgb[..., 1].astype(np.uint16) >> 3
    b = rgb[..., 2].astype(np.uint16) >> 3
    return r | (g << 5) | (b << 10)


def c555_to_rgb(c: np.ndarray) -> np.ndarray:
    c = c.astype(np.uint16)
    r = (c & 31) << 3
    g = ((c >> 5) & 31) << 3
    b = ((c >> 10) & 31) << 3
    out = np.stack([r, g, b], axis=-1)
    return (out | (out >> 5)).astype(np.uint8)


def fixed_palette() -> np.ndarray:
    """6 x 7 x 6 color cube (252 colors) plus 4 extra grays, as RGB555."""
    cols = []
    for r in range(6):
        for g in range(7):
            for b in range(6):
                cols.append((round(r * 255 / 5), round(g * 255 / 6), round(b * 255 / 5)))
    for v in (32, 96, 160, 224):
        cols.append((v, v, v))
    return rgb_to_555(np.array(cols, dtype=np.uint8))


def adaptive_palette(rgb: np.ndarray, colors: int = 256) -> np.ndarray:
    """Palette for one picture (octree, about 1 ms), as unique RGB555 values."""
    from PIL import Image
    img = Image.fromarray(rgb, "RGB")
    q = img.quantize(colors=colors, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    pal = np.array(q.getpalette()[:3 * colors], dtype=np.uint8).reshape(-1, 3)
    used = np.unique(np.asarray(q))
    pal = rgb_to_555(pal[used])
    return np.unique(pal)


class PaletteMapper:
    """Maps RGB555 colors to the nearest palette index. The lookup table is filled
    lazily, only for colors that actually appear, so a new palette costs almost
    nothing even on a Raspberry Pi."""

    def __init__(self, pal555: np.ndarray):
        self.pal555 = pal555.astype(np.uint16)
        self.pal = c555_to_rgb(self.pal555).astype(np.float32)
        self.pal_sq = (self.pal ** 2).sum(axis=1)
        self.lut = np.full(32768, -1, dtype=np.int16)
        self.err = np.zeros(32768, dtype=np.float32)

    def map(self, c555: np.ndarray) -> tuple[np.ndarray, float]:
        flat = c555.ravel()
        missing = np.unique(flat[self.lut[flat] < 0])
        if missing.size:
            rgb = c555_to_rgb(missing).astype(np.float32)
            for s in range(0, missing.size, 8192):
                part = rgb[s:s + 8192]
                # |c - p|^2 = |c|^2 - 2 c.p + |p|^2, the middle term is one matrix product
                d = self.pal_sq[None, :] - 2.0 * (part @ self.pal.T)
                best = d.argmin(axis=1)
                self.lut[missing[s:s + 8192]] = best
                self.err[missing[s:s + 8192]] = np.maximum(
                    0.0, d[np.arange(best.size), best] + (part ** 2).sum(axis=1))
        idx = self.lut[flat].astype(np.uint8).reshape(c555.shape)
        return idx, float(self.err[flat].mean())


def to_tiles(idx: np.ndarray) -> np.ndarray:
    h, w = idx.shape
    return idx.reshape(h // TILE, TILE, w // TILE, TILE).transpose(0, 2, 1, 3).reshape(-1, TILE * TILE)


def from_tiles(tiles: np.ndarray, w: int, h: int) -> np.ndarray:
    return tiles.reshape(h // TILE, w // TILE, TILE, TILE).transpose(0, 2, 1, 3).reshape(h, w)


def _pad4(b: bytes) -> bytes:
    return b + b"\0" * (-len(b) % 4)


@dataclass
class EncodeInfo:
    frame_id: int
    key: bool
    palette: bool
    ntiles: int
    raw_size: int
    size: int
    ms: float


class Encoder:
    def __init__(self, mode: int = P.MODE_256x192, palette: str = "adaptive", use_lz4: bool = True,
                 scene_change: float = 0.6, min_palette_interval: float = 1.0):
        self.mode = mode
        self.w, self.h = P.MODE_SIZE[mode]
        self.ntotal = (self.w // TILE) * (self.h // TILE)
        self.palette_mode = palette
        self.use_lz4 = use_lz4 and _lz4 is not None
        self.scene_change = scene_change
        self.min_palette_interval = min_palette_interval
        self.mapper = PaletteMapper(fixed_palette()) if palette == "fixed" else None
        self.epoch = 0
        self.err0 = 0.0
        self.palette_time = -1e9
        self.last_id = 0
        self.history: dict[int, tuple[np.ndarray, int]] = {}
        self.order: list[int] = []

    def reset(self) -> None:
        self.history.clear()
        self.order.clear()

    def has_ref(self, ref: int) -> bool:
        h = self.history.get(ref)
        return h is not None and h[1] == self.epoch

    def _new_palette(self, rgb: np.ndarray) -> None:
        self.mapper = PaletteMapper(adaptive_palette(rgb))
        self.epoch += 1
        self.palette_time = time.monotonic()
        self.err0 = -1.0

    def encode(self, rgb: np.ndarray, ref: int | None, force_key: bool = False) -> tuple[bytes, EncodeInfo]:
        """rgb: (h, w, 3) uint8 at the mode's size. ref: frame id the DS has (delta
        base) or None for a keyframe."""
        t0 = time.perf_counter()
        assert rgb.shape[:2] == (self.h, self.w), rgb.shape
        c555 = rgb_to_555(rgb)
        now = time.monotonic()
        key = force_key or ref is None or not self.has_ref(ref)
        new_pal = False
        if self.palette_mode != "fixed" and (self.mapper is None or
                                            (key and now - self.palette_time >= self.min_palette_interval)):
            self._new_palette(rgb)
            key = new_pal = True
        idx, err = self.mapper.map(c555)
        if self.err0 < 0:
            self.err0 = err
        tiles = to_tiles(idx)

        if not key:
            ref_tiles = to_tiles(self.history[ref][0])
            changed = (tiles != ref_tiles).any(axis=1)
            ratio = changed.mean()
            drift = self.palette_mode != "fixed" and err > max(2 * self.err0, self.err0 + 40)
            if (ratio >= self.scene_change or drift) and self.palette_mode != "fixed" \
                    and now - self.palette_time >= self.min_palette_interval:
                # scene change or colors drifted away from the palette: a keyframe
                # costs about the same now, so take the chance for a new palette
                self._new_palette(rgb)
                idx, err = self.mapper.map(c555)
                self.err0 = err
                tiles = to_tiles(idx)
                key = new_pal = True
            elif ratio >= 0.97:
                key = True  # nearly everything changed, skip the bitmap

        flags = 0
        parts = []
        if key:
            flags |= P.FLAG_KEY | P.FLAG_PALETTE
            pal = self.mapper.pal555
            parts.append(_pad4(pal.astype("<u2").tobytes()))
            npal = len(pal)
            parts.append(tiles.tobytes())
            ntiles = self.ntotal
            ref_id = 0
        else:
            npal = 0
            parts.append(_pad4(np.packbits(changed, bitorder="little").tobytes()))
            parts.append(tiles[changed].tobytes())
            ntiles = int(changed.sum())
            ref_id = ref
        body = b"".join(parts)
        raw_size = len(body)
        if self.use_lz4:
            comp = _lz4.compress(body, mode="default", store_size=False)
            if len(comp) < raw_size:
                body = comp
                flags |= P.FLAG_LZ4

        fid = P.next_id(self.last_id)
        self.last_id = fid
        hdr = P.FrameHeader(flags, self.mode, fid, ref_id, ntiles, raw_size, npal)
        payload = hdr.pack() + body

        self.history[fid] = (idx, self.epoch)
        self.order.append(fid)
        while len(self.order) > HISTORY:
            self.history.pop(self.order.pop(0), None)
        info = EncodeInfo(fid, key, new_pal or key, ntiles, raw_size, len(payload),
                          (time.perf_counter() - t0) * 1000)
        return payload, info


class Decoder:
    """Reference decoder, same rules as the DS client (src/decode.c)."""

    def __init__(self) -> None:
        self.cur = 0
        self.mode = None
        self.palette = np.zeros(256, dtype=np.uint16)
        self.idx: np.ndarray | None = None

    def apply(self, payload: bytes) -> str:
        """Returns "key", "delta" or "rejected"."""
        hdr = P.FrameHeader.unpack(payload)
        body = payload[P.FRAME_HDR.size:]
        key = bool(hdr.flags & P.FLAG_KEY)
        if not key and (hdr.ref_id != self.cur or hdr.mode != self.mode):
            return "rejected"
        if hdr.flags & P.FLAG_LZ4:
            if _lz4 is None:
                raise RuntimeError("lz4 module missing")
            body = _lz4.decompress(body, uncompressed_size=hdr.raw_size)
        if len(body) != hdr.raw_size:
            raise ValueError("size mismatch")
        w, h = P.MODE_SIZE[hdr.mode]
        ntotal = (w // TILE) * (h // TILE)
        pos = 0
        if hdr.flags & P.FLAG_PALETTE:
            pal = np.frombuffer(body, dtype="<u2", count=hdr.npal, offset=0)
            self.palette[:hdr.npal] = pal
            pos += (hdr.npal * 2 + 3) & ~3
        if key:
            tiles = np.frombuffer(body, dtype=np.uint8, count=ntotal * 64, offset=pos).reshape(-1, 64)
            self.idx = from_tiles(tiles.copy(), w, h)
            self.mode = hdr.mode
        else:
            nb = (ntotal + 7) // 8
            bits = np.unpackbits(np.frombuffer(body, dtype=np.uint8, count=nb, offset=pos),
                                 bitorder="little")[:ntotal].astype(bool)
            pos += (nb + 3) & ~3
            data = np.frombuffer(body, dtype=np.uint8, count=hdr.ntiles * 64, offset=pos).reshape(-1, 64)
            tiles = to_tiles(self.idx).copy()
            tiles[bits] = data
            self.idx = from_tiles(tiles, w, h)
        self.cur = hdr.frame_id
        return "key" if key else "delta"

    def rgb(self) -> np.ndarray:
        return c555_to_rgb(self.palette[self.idx])
