// SPDX-License-Identifier: Zlib
//
// Runs the DS decoder (src/decode.c) on a PC, for stream/host/tests.
//
//   decode_test <packets.bin> <out.bin> [bench_loops]
//
// packets.bin: sequence of [u16 length][UDP packet] as the DS would receive them.
// out.bin:     [u32 number of results][int8 result per completed frame]
//              [u16 cur_frame][u8 mode][u8 0][512 bytes palette][65536 top][65536 bottom]
// With bench_loops, every completed frame is decoded that many times again and the
// time per frame is printed (only as a relative number, a PC is not a DS).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/decode.h"

static uint32_t bitmap0[65536 / 4], bitmap1[65536 / 4];
static uint16_t palette[256];
static uint32_t framebuf[MAX_FRAME_SIZE / 4 + 1];
static uint32_t scratch[MAX_RAW_SIZE / 4 + 1];

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: decode_test packets.bin out.bin [bench_loops]\n");
        return 2;
    }
    int loops = argc > 3 ? atoi(argv[3]) : 0;
    FILE *f = fopen(argv[1], "rb");
    if (!f)
        return 2;
    DecodeTarget t = { { bitmap0, bitmap1 }, { palette, NULL } };
    DecodeState st = { 0, MODE_NONE };
    Reassembler r;
    reasm_init(&r, (uint8_t *)framebuf);
    static int8_t results[100000];
    uint32_t nres = 0;
    uint8_t pkt[2048];
    double bench_ns = 0;
    int bench_frames = 0;
    for (;;)
    {
        uint8_t l[2];
        if (fread(l, 1, 2, f) != 2)
            break;
        int len = l[0] | (l[1] << 8);
        if (len > (int)sizeof(pkt) || fread(pkt, 1, len, f) != (size_t)len)
            return 3;
        uint32_t flen = reasm_add(&r, pkt, len);
        if (!flen)
            continue;
        if (loops)
        {
            struct timespec a, b;
            DecodeState s2 = st;
            clock_gettime(CLOCK_MONOTONIC, &a);
            for (int i = 0; i < loops; i++)
            {
                s2 = st;
                frame_decode((uint8_t *)framebuf, flen, &s2, &t, (uint8_t *)scratch);
            }
            clock_gettime(CLOCK_MONOTONIC, &b);
            bench_ns += ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / loops;
            bench_frames++;
        }
        int res = frame_decode((uint8_t *)framebuf, flen, &st, &t, (uint8_t *)scratch);
        if (nres < sizeof(results))
            results[nres++] = res;
    }
    fclose(f);
    if (loops && bench_frames)
        printf("decode: %.1f us per frame (PC, %d frames)\n", bench_ns / bench_frames / 1000, bench_frames);

    FILE *o = fopen(argv[2], "wb");
    if (!o)
        return 2;
    uint8_t hdr[8];
    wr32(hdr, nres);
    fwrite(hdr, 1, 4, o);
    fwrite(results, 1, nres, o);
    wr16(hdr, st.cur_frame);
    hdr[2] = st.mode;
    hdr[3] = 0;
    fwrite(hdr, 1, 4, o);
    for (int i = 0; i < 256; i++)
    {
        wr16(hdr, palette[i]);
        fwrite(hdr, 1, 2, o);
    }
    fwrite(bitmap0, 1, sizeof(bitmap0), o);
    fwrite(bitmap1, 1, sizeof(bitmap1), o);
    fclose(o);
    return 0;
}
