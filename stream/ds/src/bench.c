// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: decoder benchmark.

#include <stdio.h>
#include <string.h>

#include <nds.h>

#include "bench.h"
#include "decode.h"
#include "video.h"

#include "bench_normal_bin.h"
#include "bench_high_bin.h"

static uint8_t bframe[MAX_FRAME_SIZE + 64] ALIGN(32);
static uint8_t bscratch[MAX_RAW_SIZE + 64] ALIGN(32);

typedef struct {
    int frames, keys, errors;
    uint32_t bytes;
    uint32_t us_total, us_max, us_key, us_delta;
} BenchResult;

static void run_stream(const uint8_t *data, size_t size, BenchResult *res)
{
    Reassembler rea;
    DecodeState ds = { 0, MODE_NONE };
    uint8_t layout = MODE_NONE;
    memset(res, 0, sizeof(*res));
    reasm_init(&rea, bframe);
    size_t pos = 0;
    while (pos + 2 <= size)
    {
        int len = data[pos] | (data[pos + 1] << 8);
        pos += 2;
        if (pos + len > size)
            break;
        uint32_t flen = reasm_add(&rea, data + pos, len);
        pos += len;
        if (!flen)
            continue;
        FrameHeader h;
        frame_header_read(bframe, flen, &h);
        if (h.mode != layout)
        {
            video_stream(h.mode, true);
            layout = h.mode;
        }
        cpuStartTiming(0);
        int r = frame_decode(bframe, flen, &ds, video_target(), bscratch);
        uint32_t us = timerTicks2usec(cpuEndTiming());
        if (r != DEC_KEY && r != DEC_DELTA)
        {
            res->errors++;
            continue;
        }
        res->frames++;
        res->bytes += flen;
        res->us_total += us;
        if (us > res->us_max)
            res->us_max = us;
        if (r == DEC_KEY)
        {
            res->keys++;
            res->us_key += us;
        }
        else
        {
            res->us_delta += us;
        }
    }
}

static void print_result(const char *name, const BenchResult *r)
{
    int deltas = r->frames - r->keys;
    printf("%s: %d Bilder (%d Key)\n", name, r->frames, r->keys);
    if (!r->frames)
        return;
    printf(" %lu Bytes/Bild\n", r->bytes / r->frames);
    printf(" Schnitt %lu us, max %lu us\n", r->us_total / r->frames, r->us_max);
    if (r->keys)
        printf(" Keyframe %lu us\n", r->us_key / r->keys);
    if (deltas)
        printf(" Delta    %lu us\n", r->us_delta / deltas);
    if (r->errors)
        printf(" FEHLER: %d\n", r->errors);
}

void bench_run(void)
{
    BenchResult normal, high;
    printf("Geschwindigkeitstest...\n");
    run_stream(bench_high_bin, bench_high_bin_size, &high);
    run_stream(bench_normal_bin, bench_normal_bin_size, &normal);
    consoleSelect(&con_status);
    consoleClear();
    printf("Dekodieren (ohne WLAN):\n\n");
    print_result("Testseite", &normal);
    print_result("Bewegung", &high);
}
