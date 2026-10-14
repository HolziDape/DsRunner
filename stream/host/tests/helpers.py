"""Synthetic pictures for the tests (no browser needed)."""
import numpy as np


def scene(t: int, w: int = 256, h: int = 192) -> np.ndarray:
    """A small "game": gradient background, a moving square, a counter-like block,
    a strip of noise every 10th frame (to provoke big deltas)."""
    img = np.zeros((h, w, 3), np.uint8)
    img[..., 0] = (np.arange(w)[None, :] * 255 // max(1, w - 1))
    img[..., 1] = (np.arange(h)[:, None] * 255 // max(1, h - 1))
    img[..., 2] = 60
    x = (t * 3) % (w - 20)
    y = h // 2 + int(20 * np.sin(t / 5))
    img[y:y + 16, x:x + 16] = (255, 220, 0)
    img[h - 10:h - 2, 10 + (t % 8) * 4:14 + (t % 8) * 4] = 255
    if t % 10 == 9:
        rng = np.random.default_rng(t)
        img[: h // 4] = rng.integers(0, 256, (h // 4, w, 3), dtype=np.uint8)
    return img
