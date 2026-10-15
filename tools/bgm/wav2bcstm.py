"""Converts a WAV (or, with ffmpeg installed, any audio file) to a looping DSP-ADPCM .bcstm for the
background music of DsRunner / Pico Launcher (/_pico/themes/<theme>/bgm/*.bcstm).

The whole file loops (or from --loop-start on, for songs with an intro). Lower sample rates and mono make the file smaller and cost the DS less
(22050 Hz mono: about 12.6 KB per second).

Usage:
  python wav2bcstm.py song.wav menu.bcstm
  python wav2bcstm.py song.mp3 menu.bcstm --rate 22050 --mono      (mp3, ogg, ...: needs ffmpeg in PATH)
  python wav2bcstm.py menu.bcstm --decode check.wav                 (decodes like DsRunner, for a check)
  python wav2bcstm.py --demo ambient.wav                            (renders DsRunner's calm demo loop)

Needs Python 3.8+ and numpy (pip install numpy). See "Hintergrundmusik" in docs/CONFIG.md.
"""
from __future__ import annotations

import argparse
import shutil
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np

SAMPLES_PER_FRAME = 14
BYTES_PER_FRAME = 8
BLOCK_SIZE = 0x2000                                         # bytes per channel and block
BLOCK_SAMPLES = BLOCK_SIZE // BYTES_PER_FRAME * SAMPLES_PER_FRAME  # 14336
MAX_RATE = 32768


# --- audio input ---------------------------------------------------------------------------------

def read_wav(path: Path) -> tuple[np.ndarray, int]:
    """16 bit PCM WAV -> (samples [channels, n] as int32, sample rate)."""
    with wave.open(str(path), "rb") as wav:
        channels, width, rate, count = wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()
        data = wav.readframes(count)
    if width == 2:
        samples = np.frombuffer(data, dtype="<i2").astype(np.int32)
    elif width == 1:
        samples = (np.frombuffer(data, dtype=np.uint8).astype(np.int32) - 128) << 8
    elif width == 3:
        raw = np.frombuffer(data, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        samples = ((raw[:, 0] | (raw[:, 1] << 8) | (raw[:, 2] << 16)) << 8) >> 16
    elif width == 4:
        samples = np.frombuffer(data, dtype="<i4").astype(np.int64) >> 16
        samples = samples.astype(np.int32)
    else:
        raise SystemExit(f"{path}: unsupported sample width {width * 8} bit")
    return samples.reshape(-1, channels).T.copy(), rate


def read_audio(path: Path, rate: int | None, mono: bool) -> tuple[np.ndarray, int]:
    if path.suffix.lower() == ".wav":
        try:
            samples, source_rate = read_wav(path)
        except wave.Error:
            samples = None  # e.g. float WAV: let ffmpeg convert it
        if samples is not None:
            if mono and samples.shape[0] > 1:
                samples = np.round(samples.mean(axis=0, keepdims=True)).astype(np.int32)
            if rate and rate != source_rate:
                samples = resample(samples, source_rate, rate)
                source_rate = rate
            return samples, source_rate
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise SystemExit(f"{path}: only 16 bit WAV is read directly, install ffmpeg for other formats "
                         "(or convert first: ffmpeg -i song.mp3 -ac 1 -ar 22050 song.wav)")
    with tempfile.TemporaryDirectory() as temp:
        wav_path = Path(temp) / "audio.wav"
        command = [ffmpeg, "-v", "error", "-y", "-i", str(path), "-acodec", "pcm_s16le"]
        if mono:
            command += ["-ac", "1"]
        if rate:
            command += ["-ar", str(rate)]
        subprocess.run(command + [str(wav_path)], check=True)
        return read_wav(wav_path)


def resample(samples: np.ndarray, source_rate: int, rate: int) -> np.ndarray:
    """Windowed sinc resampling (good enough for menu music, no scipy needed)."""
    count = samples.shape[1]
    new_count = int(round(count * rate / source_rate))
    positions = np.arange(new_count) * (source_rate / rate)
    cutoff = min(1.0, rate / source_rate)
    taps = 16
    result = np.zeros((samples.shape[0], new_count))
    base = np.floor(positions).astype(np.int64)
    for k in range(-taps + 1, taps + 1):
        index = base + k
        distance = positions - index
        weight = cutoff * np.sinc(cutoff * distance) * (0.5 + 0.5 * np.cos(np.pi * np.clip(distance / taps, -1, 1)))
        # the music loops: wrap around at the ends
        result += samples[:, index % count] * weight
    return np.clip(np.round(result), -32768, 32767).astype(np.int32)


# --- DSP-ADPCM -----------------------------------------------------------------------------------

def find_coefficients(x: np.ndarray) -> np.ndarray:
    """8 predictor pairs (s16, 1/2048) for one channel: 2nd order linear prediction per frame,
    grouped with k-means."""
    x = x.astype(np.float64)
    frames = len(x) // SAMPLES_PER_FRAME
    candidates, weights = [], []
    window = 2 * SAMPLES_PER_FRAME
    for f in range(frames):
        start = f * SAMPLES_PER_FRAME
        lo = max(2, start - SAMPLES_PER_FRAME // 2)
        hi = min(len(x), lo + window)
        y = x[lo:hi]
        a = np.stack([x[lo - 1:hi - 1], x[lo - 2:hi - 2]], axis=1)
        energy = float(np.dot(y, y))
        if hi - lo < 4 or energy < 1.0:
            continue
        gram = a.T @ a + np.eye(2) * 1e-3 * (energy / len(y) + 1)
        coef = np.linalg.solve(gram, a.T @ y)
        candidates.append(np.clip(coef, [-2.0, -1.0], [1.999, 0.999]))
        weights.append(np.sqrt(energy))
    defaults = np.array([[0, 0], [1, 0], [2, -1], [1.5, -0.6], [1.8, -0.85], [0.5, 0], [1.2, -0.3], [1.9, -0.95]])
    if len(candidates) < 8:
        centers = defaults
    else:
        points = np.array(candidates)
        w = np.array(weights)
        order = np.argsort(points[:, 0])
        centers = points[order[np.linspace(0, len(order) - 1, 8).astype(int)]].copy()
        for _ in range(24):
            distances = ((points[:, None, :] - centers[None, :, :]) ** 2).sum(axis=2)
            labels = distances.argmin(axis=1)
            for k in range(8):
                mask = labels == k
                if mask.any():
                    centers[k] = (points[mask] * w[mask, None]).sum(axis=0) / w[mask].sum()
                else:
                    centers[k] = defaults[k]
    return np.clip(np.round(centers * 2048), -32768, 32767).astype(np.int64)


SCALES = np.arange(13)


def encode_channel(x: np.ndarray, coefs: np.ndarray) -> tuple[bytes, list[tuple[int, int]], np.ndarray]:
    """Closed loop encoding: every frame tries all 8 predictors with all scales and keeps the one
    with the smallest error, simulating DsRunner's decoder (dspAdpcm.s) exactly.
    Returns the frames, the decoder history (hist1, hist2) at the start of every block and the
    decoded samples."""
    frames = (len(x) + SAMPLES_PER_FRAME - 1) // SAMPLES_PER_FRAME
    padded = np.zeros(frames * SAMPLES_PER_FRAME, dtype=np.int64)
    padded[:len(x)] = x
    c1 = np.repeat(coefs[:, 0], len(SCALES))
    c2 = np.repeat(coefs[:, 1], len(SCALES))
    scale = np.tile(SCALES, len(coefs))
    step = np.left_shift(1, scale)
    out = bytearray(frames * BYTES_PER_FRAME)
    decoded_all = np.zeros(frames * SAMPLES_PER_FRAME, dtype=np.int64)
    hist1 = hist2 = 0
    block_history = []
    frames_per_block = BLOCK_SIZE // BYTES_PER_FRAME
    for f in range(frames):
        if f % frames_per_block == 0:
            block_history.append((hist1, hist2))
        target = padded[f * SAMPLES_PER_FRAME:(f + 1) * SAMPLES_PER_FRAME]
        h1 = np.full(len(c1), hist1, dtype=np.int64)
        h2 = np.full(len(c1), hist2, dtype=np.int64)
        error = np.zeros(len(c1), dtype=np.int64)
        nibbles = np.zeros((SAMPLES_PER_FRAME, len(c1)), dtype=np.int64)
        values = np.zeros((SAMPLES_PER_FRAME, len(c1)), dtype=np.int64)
        for i in range(SAMPLES_PER_FRAME):
            prediction = (c1 * h1 + c2 * h2 + 1024) >> 11
            residual = target[i] - prediction
            nibble = np.clip(np.floor_divide(2 * residual + step, 2 * step), -8, 7)
            decoded = np.clip(prediction + (nibble << scale), -32768, 32767)
            error += (decoded - target[i]) ** 2
            nibbles[i] = nibble
            values[i] = decoded
            h2, h1 = h1, decoded
        best = int(error.argmin())
        predictor, best_scale = divmod(best, len(SCALES))
        hist1, hist2 = int(h1[best]), int(h2[best])
        decoded_all[f * SAMPLES_PER_FRAME:(f + 1) * SAMPLES_PER_FRAME] = values[:, best]
        out[f * BYTES_PER_FRAME] = (predictor << 4) | best_scale
        n = nibbles[:, best] & 0xF
        for j in range(7):
            out[f * BYTES_PER_FRAME + 1 + j] = int((n[2 * j] << 4) | n[2 * j + 1])
    return bytes(out), block_history, decoded_all[:len(x)]


def decode_channel(data: bytes, coefs, sample_count: int, hist1: int = 0, hist2: int = 0) -> np.ndarray:
    """Reference decoder (same math as dspAdpcm.s)."""
    out = np.zeros(sample_count, dtype=np.int32)
    n = 0
    for f in range(0, len(data), BYTES_PER_FRAME):
        header = data[f]
        scale = (header & 0xF) + 11
        c1, c2 = coefs[header >> 4]
        for j in range(SAMPLES_PER_FRAME):
            if n >= sample_count:
                return out
            byte = data[f + 1 + j // 2]
            nibble = (byte >> 4) if j % 2 == 0 else (byte & 0xF)
            if nibble >= 8:
                nibble -= 16
            sample = ((nibble << scale) + c1 * hist1 + c2 * hist2 + 1024) >> 11
            sample = max(-32768, min(32767, sample))
            out[n] = sample
            n += 1
            hist2, hist1 = hist1, sample
    return out


# --- BCSTM container -----------------------------------------------------------------------------

def align(value: int, alignment: int = 0x20) -> int:
    return (value + alignment - 1) // alignment * alignment


def ref(type_id: int, offset: int) -> bytes:
    return struct.pack("<HHi", type_id, 0, offset)


def build_bcstm(channel_data: list[bytes], coefs: list[np.ndarray], block_history: list[list[tuple[int, int]]],
                loop_contexts: list[tuple[int, int, int]], sample_count: int, loop_start: int, rate: int) -> bytes:
    channels = len(channel_data)
    blocks = (sample_count + BLOCK_SAMPLES - 1) // BLOCK_SAMPLES
    last_samples = sample_count - (blocks - 1) * BLOCK_SAMPLES
    last_size = (last_samples + SAMPLES_PER_FRAME - 1) // SAMPLES_PER_FRAME * BYTES_PER_FRAME
    last_padded = align(last_size)

    # INFO: stream info, (no) track info, channel info with the DSP-ADPCM coefficients
    stream_info = struct.pack("<BBBBIIIIIIIIIII", 2, 1, channels, 0, rate, loop_start, sample_count, blocks, BLOCK_SIZE,
                              BLOCK_SAMPLES, last_size, last_samples, last_padded, 4, BLOCK_SAMPLES)
    stream_info += ref(0x1F00, 0x18)  # sample data starts 0x18 after the DATA header (offset 0x20)
    stream_info_offset = 0x18          # relative to the INFO header + 8
    channel_table_offset = stream_info_offset + len(stream_info)
    table = struct.pack("<I", channels)
    entries = b""
    codec = b""
    entry_start = 4 + 8 * channels
    for c in range(channels):
        entry_offset = entry_start + 8 * c
        codec_offset = entry_start + 8 * channels + 0x2E * c
        table += ref(0x4102, entry_offset)
        entries += ref(0x0300, codec_offset - entry_offset)
        first = channel_data[c][0]
        codec += struct.pack("<16h", *[int(v) for v in coefs[c].reshape(-1)])
        codec += struct.pack("<BBhh", first, 0, 0, 0)          # start context
        codec += struct.pack("<BBhh", *loop_contexts[c][:1], 0, *loop_contexts[c][1:])  # loop context
        codec += b"\0\0"
    info_body = ref(0x4100, stream_info_offset) + ref(0, -1) + ref(0x0101, channel_table_offset)
    info_body += stream_info + table + entries + codec
    info = b"INFO" + struct.pack("<I", align(8 + len(info_body))) + info_body
    info += b"\0" * (align(len(info)) - len(info))

    seek_body = b"".join(struct.pack("<hh", h1, h2) for b in range(blocks) for c in range(channels)
                         for h1, h2 in [block_history[c][b]])
    seek = b"SEEK" + struct.pack("<I", align(8 + len(seek_body))) + seek_body
    seek += b"\0" * (align(len(seek)) - len(seek))

    data_body = bytearray(b"\0" * 0x18)
    for b in range(blocks):
        for c in range(channels):
            chunk = channel_data[c][b * BLOCK_SIZE:(b + 1) * BLOCK_SIZE]
            size = BLOCK_SIZE if b < blocks - 1 else last_padded
            data_body += chunk + b"\0" * (size - len(chunk))
    data = b"DATA" + struct.pack("<I", align(8 + len(data_body))) + bytes(data_body)
    data += b"\0" * (align(len(data)) - len(data))

    header_size = 0x40
    info_offset = header_size
    seek_offset = info_offset + len(info)
    data_offset = seek_offset + len(seek)
    file_size = data_offset + len(data)
    header = b"CSTM" + struct.pack("<HHIIHH", 0xFEFF, header_size, 0x02000000, file_size, 3, 0)
    header += struct.pack("<HHII", 0x4000, 0, info_offset, len(info))
    header += struct.pack("<HHII", 0x4001, 0, seek_offset, len(seek))
    header += struct.pack("<HHII", 0x4002, 0, data_offset, len(data))
    header += b"\0" * (header_size - len(header))
    return header + info + seek + data


def encode(samples: np.ndarray, rate: int, loop_start: int = 0) -> bytes:
    """The stream plays from the start to the end, then loops from loop_start (a sample number)."""
    channels = min(samples.shape[0], 2)
    samples = samples[:channels]
    count = samples.shape[1]
    channel_data, coefs, history, loop_contexts = [], [], [], []
    for c in range(channels):
        coef = find_coefficients(samples[c])
        data, block_history, decoded = encode_channel(samples[c], coef)
        channel_data.append(data)
        coefs.append(coef)
        history.append(block_history)
        # header of the frame with the loop start and the two samples before it
        header = data[loop_start // SAMPLES_PER_FRAME * BYTES_PER_FRAME]
        hist1 = int(decoded[loop_start - 1]) if loop_start >= 1 else 0
        hist2 = int(decoded[loop_start - 2]) if loop_start >= 2 else 0
        loop_contexts.append((header, hist1, hist2))
    return build_bcstm(channel_data, coefs, history, loop_contexts, count, loop_start, rate)


# --- reading .bcstm (like BcstmAudioStream.cpp) ---------------------------------------------------

def decode_bcstm(blob: bytes) -> tuple[np.ndarray, int, bool]:
    """-> (samples [channels, n], rate, loops), read the way DsRunner reads the file."""
    if blob[:4] != b"CSTM":
        raise SystemExit("no CSTM file")
    info_offset = struct.unpack_from("<I", blob, 0x18)[0]
    data_ref_offset = struct.unpack_from("<I", blob, 0x30)[0]
    stream_ref, _, channel_ref = (struct.unpack_from("<I", blob, info_offset + 8 + 4 + 8 * i)[0] for i in range(3))
    base = info_offset + 8
    (fmt, loop, channels, _, rate, loop_start, loop_end, blocks, block_size, block_samples, _, last_samples,
     last_padded, _, _) = struct.unpack_from("<BBBBIIIIIIIIIII", blob, base + stream_ref)
    if fmt != 2:
        raise SystemExit("not DSP-ADPCM")
    data_rel = struct.unpack_from("<I", blob, base + stream_ref + 0x34)[0]
    data_offset = data_ref_offset + 8 + data_rel
    table = base + channel_ref
    result = []
    for c in range(min(channels, 2)):
        entry = table + struct.unpack_from("<I", blob, table + 4 + 8 * c + 4)[0]
        codec = entry + struct.unpack_from("<I", blob, entry + 4)[0]
        coefs = np.array(struct.unpack_from("<16h", blob, codec), dtype=np.int64).reshape(8, 2)
        data = bytearray()
        for b in range(blocks):
            size = block_size if b < blocks - 1 else last_padded
            start = data_offset + block_size * b * channels + (size if b == blocks - 1 else block_size) * c
            data += blob[start:start + size]
        total = (blocks - 1) * block_samples + last_samples
        result.append(decode_channel(bytes(data), coefs, total))
    return np.array(result), rate, bool(loop)


# --- demo loop -----------------------------------------------------------------------------------

def demo_loop(rate: int = 22050) -> np.ndarray:
    """A calm ambient pad, 4 chords of 6 seconds that loop seamlessly (our own, free to use)."""
    seconds_per_chord = 6.0
    chords = [  # Cmaj9, Am9, Fmaj7(#11), G6sus
        [130.81, 196.00, 246.94, 293.66, 329.63],
        [110.00, 164.81, 196.00, 246.94, 261.63],
        [87.31, 130.81, 164.81, 220.00, 246.94],
        [98.00, 146.83, 196.00, 220.00, 261.63],
    ]
    length = int(rate * seconds_per_chord * len(chords))
    t = np.arange(length) / rate
    out = np.zeros(length)
    chord_length = length // len(chords)
    for index, chord in enumerate(chords):
        # each chord fades in and out with a long overlap; the last one overlaps into the first (loop)
        center = (index + 0.5) * chord_length
        distance = (np.arange(length) - center + length / 2) % length - length / 2
        envelope = np.clip(1.25 - np.abs(distance) / chord_length, 0, 1) ** 2
        envelope = envelope * envelope * (3 - 2 * envelope)  # smoothstep
        for voice, frequency in enumerate(chord):
            # whole cycles per loop, so every oscillator continues seamlessly
            f = round(frequency * length / rate) * rate / length
            detune = round(f * 1.003 * length / rate) * rate / length
            tone = np.sin(2 * np.pi * f * t) + 0.35 * np.sin(2 * np.pi * 2 * f * t) + 0.5 * np.sin(2 * np.pi * detune * t)
            sway = 0.75 + 0.25 * np.sin(2 * np.pi * (voice + 1) * t / (length / rate) + voice)
            out += tone * envelope * sway / (1.85 * len(chord))
    # a soft bell motif on top, every 1.5 s, from the notes of the current chord
    bell = np.zeros(length)
    step = int(rate * 1.5)
    for n, start in enumerate(range(0, length, step)):
        chord = chords[(start // chord_length) % len(chords)]
        frequency = chord[[4, 3, 2, 3][n % 4]] * 2
        f = frequency
        decay = np.exp(-np.arange(int(rate * 3.0)) / (rate * 0.7))
        note = np.sin(2 * np.pi * f * np.arange(len(decay)) / rate) * decay
        index = (start + np.arange(len(note))) % length  # wraps into the beginning: seamless loop
        bell[index] += note * 0.12
    out += bell
    out *= 0.8 / max(1e-9, np.abs(out).max())
    return np.round(out * 32767).astype(np.int32)[None, :]


def write_wav(path: Path, samples: np.ndarray, rate: int) -> None:
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(samples.shape[0])
        wav.setsampwidth(2)
        wav.setframerate(rate)
        wav.writeframes(np.clip(samples, -32768, 32767).astype("<i2").T.tobytes())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("input", nargs="?", type=Path, help="audio file (or .bcstm with --decode)")
    parser.add_argument("output", nargs="?", type=Path, help="the .bcstm to write")
    parser.add_argument("--rate", type=int, default=None, help="sample rate in Hz, e.g. 22050 (default: of the input, max 32768)")
    parser.add_argument("--mono", action="store_true", help="mix down to one channel (half the size)")
    parser.add_argument("--volume", type=float, default=1.0, help="gain, e.g. 0.7")
    parser.add_argument("--loop-start", type=float, default=0.0, metavar="SECONDS",
                        help="the loop goes back to this point (default 0: the whole file loops)")
    parser.add_argument("--decode", type=Path, metavar="WAV", help="decode the input .bcstm to this WAV")
    parser.add_argument("--demo", type=Path, metavar="FILE", help="write the demo loop (.wav or .bcstm)")
    args = parser.parse_args()

    if args.demo:
        rate = args.rate or 22050
        samples = demo_loop(rate)
        if args.demo.suffix.lower() == ".bcstm":
            args.demo.write_bytes(encode(samples, rate))
        else:
            write_wav(args.demo, samples, rate)
        print(f"{args.demo}: demo loop, {samples.shape[1] / rate:.1f} s, {rate} Hz")
        return
    if args.decode:
        samples, rate, loops = decode_bcstm(args.input.read_bytes())
        write_wav(args.decode, samples, rate)
        print(f"{args.decode}: {samples.shape[0]} channel(s), {rate} Hz, {samples.shape[1] / rate:.1f} s, loop: {loops}")
        return
    if not args.input or not args.output:
        parser.error("input and output are needed")

    samples, rate = read_audio(args.input, args.rate, args.mono)
    if rate > MAX_RATE:
        samples = resample(samples, rate, 32000)
        rate = 32000
        print("sample rate lowered to 32000 Hz (the DS plays at most 32768 Hz)")
    if args.volume != 1.0:
        samples = np.clip(np.round(samples * args.volume), -32768, 32767).astype(np.int32)
    if samples.shape[0] > 2:
        print("only the first two channels are used")
    loop_start = int(round(args.loop_start * rate))
    if not 0 <= loop_start < samples.shape[1]:
        parser.error("--loop-start is outside of the audio")
    blob = encode(samples, rate, loop_start)
    args.output.write_bytes(blob)
    decoded, _, _ = decode_bcstm(blob)
    reference = samples[:decoded.shape[0]].astype(np.float64)
    noise = ((decoded - reference) ** 2).mean()
    snr = 10 * np.log10((reference ** 2).mean() / noise) if noise > 0 else float("inf")
    print(f"{args.output}: {decoded.shape[0]} channel(s), {rate} Hz, {samples.shape[1] / rate:.1f} s, "
          f"{len(blob) / 1024:.0f} KB, loops from {loop_start / rate:.2f} s, SNR {snr:.1f} dB")


if __name__ == "__main__":
    main()
