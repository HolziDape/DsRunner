#!/usr/bin/env python3
"""Convert GUI icons between .nbfc and PGM.

.nbfc icons are 4bpp tiled graphics as used by the GBARunner2 GUI: 8x8 tiles
stored row by row, 4 bytes per tile row, low nibble = left pixel. Each 4 bit
value is the coverage (0 = transparent .. 15 = opaque) used with a gradient
palette, so icons are effectively antialiased alpha masks.

PGM (P2 or P5, any maxval) is used as the editable format because it needs no
extra python packages and every image editor can export it.

Usage:
  nbfc.py show   <icon.nbfc> [width]           print as ascii art
  nbfc.py decode <icon.nbfc> <out.pgm> [width] nbfc -> pgm
  nbfc.py encode <in.pgm> <icon.nbfc>          pgm  -> nbfc (size multiple of 8)
"""
import sys

ASCII_RAMP = " .:-=+*#%@ABCDEF"


def decode(data, width=16):
    tiles_x = width // 8
    height = len(data) * 2 // width
    px = [[0] * width for _ in range(height)]
    for t in range(len(data) // 32):
        tx, ty = t % tiles_x, t // tiles_x
        for i in range(32):
            b = data[t * 32 + i]
            y, x = ty * 8 + i // 4, tx * 8 + (i % 4) * 2
            px[y][x] = b & 0xF
            px[y][x + 1] = b >> 4
    return px


def encode(px):
    height, width = len(px), len(px[0])
    if width % 8 or height % 8:
        raise ValueError("icon size must be a multiple of 8")
    out = bytearray()
    for ty in range(height // 8):
        for tx in range(width // 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    lo = px[ty * 8 + y][tx * 8 + x]
                    hi = px[ty * 8 + y][tx * 8 + x + 1]
                    out.append((lo & 0xF) | ((hi & 0xF) << 4))
    return bytes(out)


def read_pgm(path):
    data = open(path, "rb").read()
    tokens, pos = [], 0
    # header: magic, width, height, maxval (comments allowed)
    while len(tokens) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        if data[pos:pos + 1] == b"#":
            pos = data.index(b"\n", pos)
            continue
        start = pos
        while not data[pos:pos + 1].isspace():
            pos += 1
        tokens.append(data[start:pos])
    magic, width, height, maxval = tokens[0], int(tokens[1]), int(tokens[2]), int(tokens[3])
    if magic == b"P5":
        raw = data[pos + 1:pos + 1 + width * height]
        values = list(raw)
    elif magic == b"P2":
        values = [int(v) for v in data[pos:].split()][:width * height]
    else:
        raise ValueError("only P2/P5 pgm files are supported")
    return [[round(values[y * width + x] * 15 / maxval) for x in range(width)] for y in range(height)]


def write_pgm(path, px):
    height, width = len(px), len(px[0])
    with open(path, "w") as f:
        f.write(f"P2\n{width} {height}\n15\n")
        for row in px:
            f.write(" ".join(str(v) for v in row) + "\n")


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 1
    cmd = argv[1]
    if cmd == "show":
        width = int(argv[3]) if len(argv) > 3 else 16
        for row in decode(open(argv[2], "rb").read(), width):
            print("".join(ASCII_RAMP[v] for v in row))
    elif cmd == "decode":
        width = int(argv[4]) if len(argv) > 4 else 16
        write_pgm(argv[3], decode(open(argv[2], "rb").read(), width))
    elif cmd == "encode":
        open(argv[3], "wb").write(encode(read_pgm(argv[2])))
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
