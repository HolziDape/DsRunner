"""Tests for wav2bcstm.py: python3 -m unittest discover -s tools/bgm

The playback test runs the files through a Python model of DsRunner's BcstmAudioStream
(arm9/source/bgm/BcstmAudioStream.cpp, ReadSamples / FetchBlock / DecodeLoopAdpcmFrame), so the
loop handling of the launcher is checked, too.
"""
import struct
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import wav2bcstm as w  # noqa: E402


def clamp16(v):
    return max(-32768, min(32767, v))


class StreamModel:
    """Mirror of BcstmAudioStream (one channel is enough, the channels are independent)."""

    def __init__(self, blob: bytes, channel: int = 0):
        info_offset = struct.unpack_from("<I", blob, 0x18)[0]
        data_block = struct.unpack_from("<I", blob, 0x30)[0]
        base = info_offset + 8
        stream_ref = struct.unpack_from("<I", blob, base + 4)[0]
        channel_ref = struct.unpack_from("<I", blob, base + 20)[0]
        (fmt, self.loop, self.channels, _, self.rate, self.loop_start, self.loop_end, self.blocks,
         self.block_size, self.block_samples, _, self.last_samples, self.last_padded, _, _) = \
            struct.unpack_from("<BBBBIIIIIIIIIII", blob, base + stream_ref)
        assert fmt == 2
        data_rel = struct.unpack_from("<I", blob, base + stream_ref + 0x34)[0]
        self.data_offset = data_block + 8 + data_rel
        table = base + channel_ref
        entry = table + struct.unpack_from("<I", blob, table + 4 + 8 * channel + 4)[0]
        codec = entry + struct.unpack_from("<I", blob, entry + 4)[0]
        self.coefs = struct.unpack_from("<16h", blob, codec)
        self.context = struct.unpack_from("<BBhh", blob, codec + 32)
        self.loop_context = struct.unpack_from("<BBhh", blob, codec + 38)
        self.blob = blob
        self.channel = channel
        self.frame_data = [0] * 14
        self.frame_data[12] = self.context[3]  # last2
        self.frame_data[13] = self.context[2]  # last1
        self.frame_idx = 0
        self.block_number = 0
        self.sample_in_block = 0
        if self.loop:
            self.loop_block = self.loop_start // self.block_samples
            self.loop_block_start = self.loop_start % self.block_samples
            self.loop_end_block = (self.loop_end - 1) // self.block_samples
            self.loop_end_sample = (self.loop_end - 1) % self.block_samples
        else:
            self.loop_block = self.loop_block_start = self.loop_end_block = self.loop_end_sample = 0

    def total_in_block(self):
        total = self.last_samples if self.block_number == self.blocks - 1 else self.block_samples
        if self.loop and self.block_number == self.loop_end_block:
            total = self.loop_end_sample + 1
        return total

    def fetch_block(self):
        looped = False
        if self.block_number == self.blocks or (self.loop and self.block_number == self.loop_end_block + 1):
            looped = True
            self.block_number = self.loop_block if self.loop else 0
        size = self.last_padded if self.block_number == self.blocks - 1 else self.block_size
        start = self.data_offset + self.block_size * (self.block_number * self.channels) + size * self.channel
        self.block = self.blob[start:start + size]
        self.pos = 0
        self.frame_idx = 0
        if looped:
            self.sample_in_block = self.loop_block_start
            if self.block_number == 0 and self.loop_block_start == 0:
                self.frame_data[12] = self.context[3]
                self.frame_data[13] = self.context[2]
            else:
                offset = self.loop_block_start % 14
                frame = self.loop_block_start // 14
                self.pos = frame * 8
                if offset:
                    self.decode_loop_frame(self.loop_context[0], self.loop_context[2], self.loop_context[3], offset)
                else:
                    self.frame_data[12] = self.loop_context[3]
                    self.frame_data[13] = self.loop_context[2]

    def decode_frame(self):
        header = self.block[self.pos]
        scale = (header & 0xF) + 11
        c1, c2 = self.coefs[(header >> 4) * 2], self.coefs[(header >> 4) * 2 + 1]
        last2, last1 = self.frame_data[12], self.frame_data[13]
        for j in range(14):
            byte = self.block[self.pos + 1 + j // 2]
            nibble = (byte >> 4) if j % 2 == 0 else (byte & 0xF)
            nibble = nibble - 16 if nibble >= 8 else nibble
            sample = clamp16(((nibble << scale) + c1 * last1 + c2 * last2 + 1024) >> 11)
            self.frame_data[j] = sample
            last2, last1 = last1, sample
        self.pos += 8
        self.frame_idx = 0

    def decode_loop_frame(self, scale_coef, last1, last2, sample_offset):
        # DecodeLoopAdpcmFrame with DsRunner's fix (1 + n: skip the frame header)
        data = self.pos + 1 + (sample_offset >> 1)
        scale = (scale_coef & 0xF) + 11
        c1, c2 = self.coefs[(scale_coef >> 4) * 2], self.coefs[(scale_coef >> 4) * 2 + 1]
        if sample_offset - 2 >= 0:
            self.frame_data[sample_offset - 2] = last2
        if sample_offset - 1 >= 0:
            self.frame_data[sample_offset - 1] = last1
        j = sample_offset
        while j < 14:
            value = self.block[data]
            data += 1
            if j & 1 == 0:
                high = (value >> 4) - 16 if (value >> 4) >= 8 else (value >> 4)
                sample = clamp16(((high << scale) + c1 * last1 + c2 * last2 + 1024) >> 11)
                self.frame_data[j] = sample
                last2, last1 = last1, sample
            else:
                j -= 1
            low = (value & 0xF) - 16 if (value & 0xF) >= 8 else (value & 0xF)
            sample = clamp16(((low << scale) + c1 * last1 + c2 * last2 + 1024) >> 11)
            self.frame_data[j + 1] = sample
            last2, last1 = last1, sample
            j += 2
        self.frame_idx = sample_offset
        self.pos = data

    def read(self, count):
        out = []
        while count:
            if self.sample_in_block == 0:
                self.fetch_block()
            total = self.total_in_block()
            n = min(total - self.sample_in_block, count)
            for _ in range(n):
                if self.frame_idx == 0:
                    self.decode_frame()
                out.append(self.frame_data[self.frame_idx])
                self.frame_idx = (self.frame_idx + 1) % 14
            count -= n
            self.sample_in_block += n
            if self.sample_in_block == total:
                self.block_number += 1
                self.sample_in_block = 0
        return out


def test_signal(count, rate=16000):
    t = np.arange(count) / rate
    signal = 9000 * np.sin(2 * np.pi * 220 * t) + 4000 * np.sin(2 * np.pi * 1330 * t + 1)
    return np.round(signal).astype(np.int32)[None, :]


class Wav2BcstmTest(unittest.TestCase):
    def test_roundtrip_quality(self):
        samples = test_signal(20000)
        decoded, rate, loops = w.decode_bcstm(w.encode(samples, 16000))
        self.assertEqual((rate, loops), (16000, True))
        self.assertEqual(decoded.shape, samples.shape)
        noise = ((decoded - samples) ** 2).mean()
        snr = 10 * np.log10((samples.astype(np.float64) ** 2).mean() / noise)
        self.assertGreater(snr, 30)

    def test_stereo_layout(self):
        left = test_signal(30000)[0]
        right = -left // 2
        blob = w.encode(np.stack([left, right]), 16000)
        decoded, _, _ = w.decode_bcstm(blob)
        self.assertEqual(decoded.shape, (2, 30000))
        self.assertLess(np.abs(decoded[0] - left).mean(), 200)
        self.assertLess(np.abs(decoded[1] - right).mean(), 200)

    def check_playback(self, count, loop_start, channels=1):
        signal = test_signal(count)
        samples = np.concatenate([signal, signal // 3])[:channels]
        blob = w.encode(samples, 16000, loop_start)
        reference, _, _ = w.decode_bcstm(blob)
        for c in range(channels):
            model = StreamModel(blob, c)
            played = model.read(count + 2 * (count - loop_start) + 77)
            expected = list(reference[c]) + list(reference[c][loop_start:]) * 3
            self.assertEqual(played, expected[:len(played)], f"loop start {loop_start}, channel {c}")

    def test_whole_file_loop(self):
        self.check_playback(40000, 0)

    def test_loop_start_in_first_block(self):
        # inside a frame, inside the first block (14336 samples per block)
        self.check_playback(40000, 1003)
        self.check_playback(40000, 1004)

    def test_loop_start_in_later_block(self):
        self.check_playback(40000, w.BLOCK_SAMPLES + 141, channels=2)
        self.check_playback(40000, w.BLOCK_SAMPLES * 2)

    def test_short_file(self):
        self.check_playback(500, 0)


if __name__ == "__main__":
    unittest.main()
