"""Chromium via Playwright: opens the page, captures it small, injects input."""
from __future__ import annotations

import asyncio
import base64
import io
import logging
import os
import platform
import shutil
import time
from pathlib import Path

import numpy as np
from PIL import Image

from .mapping import Event

log = logging.getLogger("dsstream.browser")

TESTPAGE = Path(__file__).resolve().parent / "testpage" / "index.html"

# keep rendering at full speed although nobody looks at the window
BASE_ARGS = [
    "--disable-background-timer-throttling",
    "--disable-renderer-backgrounding",
    "--disable-backgrounding-occluded-windows",
    "--autoplay-policy=no-user-gesture-required",
    "--no-first-run",
    "--no-default-browser-check",
]
# GPU rendering (WebGL/canvas). On the Raspberry Pi 5 the Chromium of Raspberry Pi OS
# renders with the V3D GPU (OpenGL ES through ANGLE). Check with --url chrome://gpu.
GPU_ARGS = [
    "--ignore-gpu-blocklist",
    "--enable-gpu-rasterization",
    "--enable-zero-copy",
]
GPU_ARGS_LINUX_ARM = ["--use-angle=gles"]

SYSTEM_CHROMIUM = ["/usr/bin/chromium", "/usr/bin/chromium-browser"]


def resolve_url(url: str) -> str:
    if url in ("test", "testpage"):
        return TESTPAGE.as_uri()
    if url.startswith("test?"):
        return TESTPAGE.as_uri() + url[4:]
    if "://" not in url and Path(url).exists():
        return Path(url).resolve().as_uri()
    return url


def pick_executable(choice: str) -> str | None:
    """auto: on a Raspberry Pi (Linux ARM) the system Chromium, it has the Pi GPU
    support; elsewhere the Chromium that comes with Playwright."""
    if choice == "bundled":
        return None
    if choice not in ("auto", ""):
        return choice
    if platform.system() == "Linux" and platform.machine() in ("aarch64", "arm64", "armv7l"):
        for p in SYSTEM_CHROMIUM:
            if os.path.exists(p):
                return p
        found = shutil.which("chromium") or shutil.which("chromium-browser")
        if found:
            return found
    return None


class Browser:
    def __init__(self, url: str, viewport: tuple[int, int], out_size: tuple[int, int], *,
                 headless: bool = True, gpu: bool = False, executable: str = "auto",
                 capture: str = "screencast", max_fps: float = 30.0, mute: bool = False,
                 extra_args: list[str] | None = None):
        self.url = resolve_url(url)
        self.viewport = viewport
        self.out_size = out_size
        self.headless = headless
        self.gpu = gpu
        self.executable = pick_executable(executable)
        self.capture = capture
        self.max_fps = max_fps
        self.mute = mute
        self.extra_args = extra_args or []
        self.frame_seq = 0          # increases with every captured picture
        self._latest: bytes | None = None
        self._latest_rgb: np.ndarray | None = None
        self._rgb_seq = -1
        self.captured = 0
        self._pw = self._browser = self.page = self.cdp = None
        self._shot_task = None

    async def start(self) -> None:
        from playwright.async_api import async_playwright
        args = list(BASE_ARGS)
        if self.gpu:
            args += GPU_ARGS
            if platform.system() == "Linux" and platform.machine() in ("aarch64", "arm64", "armv7l"):
                args += GPU_ARGS_LINUX_ARM
        if self.mute:
            args.append("--mute-audio")
        args += self.extra_args
        self._pw = await async_playwright().start()
        log.info("Chromium: %s, %s, args %s", self.executable or "Playwright",
                 "headless" if self.headless else "mit Fenster", " ".join(args))
        self._browser = await self._pw.chromium.launch(
            headless=self.headless, executable_path=self.executable, args=args)
        ctx = await self._browser.new_context(
            viewport={"width": self.viewport[0], "height": self.viewport[1]}, device_scale_factor=1)
        self.page = await ctx.new_page()
        log.info("Lade %s", self.url)
        await self.page.goto(self.url, wait_until="load", timeout=60000)
        self.cdp = await ctx.new_cdp_session(self.page)
        if self.capture == "screencast":
            self.cdp.on("Page.screencastFrame", self._on_frame)
            await self.cdp.send("Page.startScreencast", {
                "format": "png", "maxWidth": self.out_size[0], "maxHeight": self.out_size[1],
                "everyNthFrame": 1})
        else:
            self._shot_task = asyncio.create_task(self._screenshot_loop())

    def _on_frame(self, params: dict) -> None:
        self._latest = base64.b64decode(params["data"])
        self.frame_seq += 1
        self.captured += 1
        asyncio.ensure_future(self._ack(params["sessionId"]))

    async def _ack(self, session_id) -> None:
        try:
            await self.cdp.send("Page.screencastFrameAck", {"sessionId": session_id})
        except Exception:  # browser closing
            pass

    async def _screenshot_loop(self) -> None:
        vw, vh = self.viewport
        scale = self.out_size[0] / vw
        while True:
            t0 = time.monotonic()
            try:
                r = await self.cdp.send("Page.captureScreenshot", {
                    "format": "png", "clip": {"x": 0, "y": 0, "width": vw, "height": vh, "scale": scale}})
                self._latest = base64.b64decode(r["data"])
                self.frame_seq += 1
                self.captured += 1
            except Exception as e:  # page navigating etc.
                log.debug("screenshot failed: %s", e)
            await asyncio.sleep(max(0.0, 1.0 / self.max_fps - (time.monotonic() - t0)))

    def latest_rgb(self) -> np.ndarray | None:
        """Newest picture as (h, w, 3) uint8 in the output size."""
        if self._latest is None:
            return None
        if self._rgb_seq != self.frame_seq:
            img = Image.open(io.BytesIO(self._latest)).convert("RGB")
            if img.size != self.out_size:
                img = img.resize(self.out_size, Image.BILINEAR)
            self._latest_rgb = np.asarray(img)
            self._rgb_seq = self.frame_seq
        return self._latest_rgb

    async def inject(self, ev: Event) -> None:
        kb, mouse = self.page.keyboard, self.page.mouse
        try:
            if ev.kind == "keydown":
                await kb.down(ev.key)
            elif ev.kind == "keyup":
                await kb.up(ev.key)
            elif ev.kind == "mousemove":
                await mouse.move(ev.x, ev.y)
            elif ev.kind == "mousedown":
                await mouse.move(ev.x, ev.y)
                await mouse.down(button=ev.key)
            elif ev.kind == "mouseup":
                await mouse.up(button=ev.key)
        except Exception as e:
            log.warning("Eingabe %s %s fehlgeschlagen: %s", ev.kind, ev.key, e)

    async def close(self) -> None:
        try:
            if self._shot_task:
                self._shot_task.cancel()
            if self._browser:
                await self._browser.close()
        finally:
            if self._pw:
                await self._pw.stop()
