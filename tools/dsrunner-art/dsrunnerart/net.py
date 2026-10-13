"""Small HTTP client on urllib with a download cache, a cache for misses (404) and per host failure
tracking, so an unreachable source is skipped instead of timing out for every game."""
from __future__ import annotations

import hashlib
import json
import os
import socket
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Optional

from . import __version__

USER_AGENT = f"DsRunnerArt/{__version__} (+https://github.com/HolziDape/DsRunner)"
MISS_TTL = 14 * 24 * 3600


class NetworkError(Exception):
    """The server could not be reached (as opposed to "file not found")."""


def default_cache_dir() -> Path:
    base = os.environ.get("LOCALAPPDATA")
    if base:
        return Path(base) / "DsRunnerArt" / "cache"
    base = os.environ.get("XDG_CACHE_HOME") or os.path.join(os.path.expanduser("~"), ".cache")
    return Path(base) / "dsrunner-art"


class HttpClient:
    def __init__(self, cache_dir: Path, offline: bool = False, timeout: float = 30.0, retries: int = 2,
                 refresh_misses: bool = False):
        self.cache_dir = Path(cache_dir)
        self.offline = offline
        self.timeout = timeout
        self.retries = retries
        self.refresh_misses = refresh_misses
        self.request_count = 0
        self._lock = threading.Lock()
        self._down_hosts: dict[str, str] = {}
        self._misses_path = self.cache_dir / "misses.json"
        self._misses: dict[str, float] = {}
        self._misses_dirty = False
        try:
            with open(self._misses_path, encoding="utf-8") as file:
                self._misses = {key: float(value) for key, value in json.load(file).items()}
        except (OSError, ValueError, AttributeError):
            self._misses = {}

    # --- low level ---------------------------------------------------------------------------

    def _open(self, url: str, headers: Optional[dict] = None) -> Optional[bytes]:
        """Downloads url. Returns None for 404/410, raises NetworkError when the host is unusable."""
        host = urllib.parse.urlsplit(url).hostname or ""
        with self._lock:
            if host in self._down_hosts:
                raise NetworkError(self._down_hosts[host])
        if self.offline:
            raise NetworkError("offline")
        request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, **(headers or {})})
        last_error = ""
        for attempt in range(self.retries + 1):
            try:
                with self._lock:
                    self.request_count += 1
                with urllib.request.urlopen(request, timeout=self.timeout) as response:
                    return response.read()
            except urllib.error.HTTPError as error:
                if error.code in (404, 410):
                    return None
                last_error = f"HTTP {error.code}"
                if error.code in (401, 403) or (error.code < 500 and error.code != 429):
                    break
            except (urllib.error.URLError, socket.timeout, TimeoutError, ConnectionError, OSError) as error:
                last_error = str(getattr(error, "reason", error))
            if attempt < self.retries:
                time.sleep(1.5 * (attempt + 1))
        with self._lock:
            self._down_hosts[host] = f"{host}: {last_error}"
        raise NetworkError(f"{host}: {last_error}")

    def is_down(self, url: str) -> bool:
        host = urllib.parse.urlsplit(url).hostname or ""
        with self._lock:
            return host in self._down_hosts

    # --- cached access -----------------------------------------------------------------------

    def _cache_file(self, url: str, folder: str) -> Path:
        digest = hashlib.sha1(url.encode("utf-8")).hexdigest()
        return self.cache_dir / folder / digest[:2] / digest

    def fetch(self, url: str, folder: str = "downloads", max_age: Optional[float] = None,
              headers: Optional[dict] = None) -> Optional[bytes]:
        """Cached download. Returns None when the file does not exist on the server.

        max_age: seconds after which a cached copy is downloaded again (None = keep forever).
        If the server is not reachable, an outdated cached copy is still returned.
        """
        path = self._cache_file(url, folder)
        cached: Optional[bytes] = None
        try:
            cached = path.read_bytes()
            if max_age is None or time.time() - path.stat().st_mtime < max_age:
                return cached
        except OSError:
            cached = None
        if cached is None and self._is_known_miss(url):
            return None
        try:
            data = self._open(url, headers)
        except NetworkError:
            if cached is not None:
                return cached
            raise
        if data is None:
            self._remember_miss(url)
            return None
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            temp = path.with_name(path.name + f".{threading.get_ident()}.tmp")
            temp.write_bytes(data)
            os.replace(temp, path)
        except OSError:
            pass
        return data

    def _is_known_miss(self, url: str) -> bool:
        if self.refresh_misses:
            return False
        with self._lock:
            when = self._misses.get(url)
        return when is not None and time.time() - when < MISS_TTL

    def _remember_miss(self, url: str) -> None:
        with self._lock:
            self._misses[url] = time.time()
            self._misses_dirty = True

    def save(self) -> None:
        """Writes the miss cache (call once at the end of a run)."""
        with self._lock:
            if not self._misses_dirty:
                return
            now = time.time()
            misses = {url: when for url, when in self._misses.items() if now - when < MISS_TTL}
            self._misses_dirty = False
        try:
            self.cache_dir.mkdir(parents=True, exist_ok=True)
            temp = self._misses_path.with_suffix(".tmp")
            temp.write_text(json.dumps(misses), encoding="utf-8")
            os.replace(temp, self._misses_path)
        except OSError:
            pass
