"""Host server + fake DS client over UDP on localhost, with a fake browser (no
Chromium needed). Checks streaming, recovery after packet loss and input injection."""
import asyncio
import threading
import types
import unittest

import numpy as np

from dsstream import fakeclient
from dsstream import protocol as P
from dsstream.mapping import Mapping
from dsstream.server import RateControl, StreamServer

from helpers import scene


class FakeBrowser:
    viewport = (512, 384)

    def __init__(self):
        self.t = 0
        self.frame_seq = 0
        self.events = []

    async def run(self):
        while True:
            await asyncio.sleep(1 / 30)
            self.t += 1
            self.frame_seq += 1

    def latest_rgb(self):
        return scene(self.t)

    async def inject(self, ev):
        self.events.append(ev)


class ServerTest(unittest.TestCase):
    def run_session(self, loss: float, seconds: float = 3.0):
        port = 17770 + int(loss * 100)
        browser = FakeBrowser()
        server = StreamServer(browser, Mapping.load("default.json"), P.MODE_256x192, port=port,
                              max_fps=20, stats_every=60)
        result = {}

        def client():
            args = types.SimpleNamespace(host="127.0.0.1", port=port, seconds=seconds, save=None,
                                         save_every=1000, max_png=0, script="RIGHT@0.5-1,TOUCH:128x96@1.2-1.4",
                                         loss=loss, max_kbs=0.0)
            result.update(fakeclient.run(args))

        async def main():
            loop = asyncio.get_running_loop()
            transport, _ = await loop.create_datagram_endpoint(lambda: server, local_addr=("127.0.0.1", port))
            tasks = [asyncio.create_task(c) for c in
                     (server.frame_loop(), server.input_loop(), browser.run())]
            th = threading.Thread(target=client)
            th.start()
            while th.is_alive():
                await asyncio.sleep(0.05)
            await asyncio.sleep(0.2)
            for t in tasks:
                t.cancel()
            transport.close()

        asyncio.run(main())
        return result, browser

    def test_stream_and_input(self):
        res, browser = self.run_session(0.0)
        self.assertGreater(res["fps"], 12)
        self.assertEqual(res["rejected"], 0)
        kinds = [(e.kind, e.key) for e in browser.events]
        self.assertIn(("keydown", "ArrowRight"), kinds)
        self.assertIn(("keyup", "ArrowRight"), kinds)
        self.assertIn(("mousedown", "left"), kinds)
        self.assertIn(("mouseup", "left"), kinds)
        # the BYE at the end released everything again
        self.assertEqual(kinds.count(("keydown", "ArrowRight")), kinds.count(("keyup", "ArrowRight")))

    def test_recovers_from_loss(self):
        res, _ = self.run_session(0.10, seconds=4.0)
        self.assertGreater(res["rejected"] + res["incomplete"], 0)
        self.assertGreater(res["fps"], 5)


class RateControlTest(unittest.TestCase):
    def feed(self, rc, rx_share, bad_per_step, rate_bytes, steps=10):
        sent = rx = bad = 0
        rc.update(0, 0, 0, 0.0)
        for i in range(1, steps + 1):
            sent += int(rate_bytes * 2)
            rx += int(rate_bytes * 2 * rx_share)
            bad += bad_per_step
            rc.update(rx, bad, sent, i * 2.0)
        return rc.rate

    def test_random_loss_keeps_rate(self):
        rc = RateControl(120000, 40000, 200000)
        self.assertGreaterEqual(self.feed(rc, 0.97, 3, 50000), 120000)

    def test_congestion_lowers_rate(self):
        rc = RateControl(120000, 40000, 200000)
        self.assertLess(self.feed(rc, 0.7, 10, 120000, steps=3), 90000)

    def test_no_loss_raises_rate_when_limiting(self):
        rc = RateControl(100000, 40000, 200000)
        self.assertGreater(self.feed(rc, 1.0, 0, 100000, steps=3), 120000)


if __name__ == "__main__":
    unittest.main()
