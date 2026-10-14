// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: reassembly and decoding of frames.

#include <string.h>

#include "decode.h"

#ifdef DSSTREAM_DS
#include <nds/ndstypes.h>
// hot loops in ARM code in ITCM: no instruction fetches from main RAM
#define FAST_CODE ARM_CODE ITCM_CODE
#else
#define FAST_CODE
#endif

bool frame_header_read(const uint8_t *p, uint32_t len, FrameHeader *h)
{
    if (len < FRAME_HDR_SIZE)
        return false;
    h->flags = p[0];
    h->mode = p[1];
    h->frame_id = rd16(p + 2);
    h->ref_id = rd16(p + 4);
    h->ntiles = rd16(p + 6);
    h->raw_size = rd32(p + 8);
    h->npal = rd16(p + 12);
    return h->mode <= MODE_256x384 && h->raw_size <= MAX_RAW_SIZE && h->npal <= 256;
}

FAST_CODE int lz4_decode(const uint8_t *src, int srclen, uint8_t *dst, int dstlen)
{
    const uint8_t *ip = src;
    const uint8_t *const iend = src + srclen;
    uint8_t *op = dst;
    uint8_t *const oend = dst + dstlen;

    while (ip < iend)
    {
        unsigned token = *ip++;
        unsigned lit = token >> 4;
        if (lit == 15)
        {
            unsigned b;
            do
            {
                if (ip >= iend)
                    return -1;
                b = *ip++;
                lit += b;
            } while (b == 255);
        }
        if (lit > (unsigned)(iend - ip) || lit > (unsigned)(oend - op))
            return -1;
        memcpy(op, ip, lit);
        op += lit;
        ip += lit;
        if (ip >= iend)
            break; // the last sequence has literals only

        if (iend - ip < 2)
            return -1;
        unsigned off = ip[0] | (ip[1] << 8);
        ip += 2;
        if (off == 0 || off > (unsigned)(op - dst))
            return -1;
        unsigned ml = token & 15;
        if (ml == 15)
        {
            unsigned b;
            do
            {
                if (ip >= iend)
                    return -1;
                b = *ip++;
                ml += b;
            } while (b == 255);
        }
        ml += 4;
        if (ml > (unsigned)(oend - op))
            return -1;
        const uint8_t *m = op - off;
        if (off >= ml)
        {
            memcpy(op, m, ml); // no overlap
            op += ml;
        }
        else
        {
            // overlapping copy (runs): byte by byte, 4x unrolled
            while (ml >= 4)
            {
                op[0] = m[0]; op[1] = m[1]; op[2] = m[2]; op[3] = m[3];
                op += 4; m += 4; ml -= 4;
            }
            while (ml--)
                *op++ = *m++;
        }
    }
    return op - dst;
}

// Copies one 8x8 tile (64 bytes, rows of 8) into an 8 bpp bitmap with a row
// stride of 256 bytes, with 32-bit stores only (VRAM can't take 8-bit stores).
static inline void put_tile(uint32_t *bitmap, unsigned tx, unsigned ty, const uint32_t *src)
{
    uint32_t *d = bitmap + ty * (8 * 256 / 4) + tx * 2;
    for (int r = 0; r < 8; r++)
    {
        d[0] = src[0];
        d[1] = src[1];
        d += 256 / 4;
        src += 2;
    }
}

FAST_CODE int frame_decode(const uint8_t *payload, uint32_t len, DecodeState *st,
                           const DecodeTarget *t, uint8_t *scratch)
{
    FrameHeader h;
    if (!frame_header_read(payload, len, &h))
        return DEC_ERROR;

    bool key = h.flags & FLAG_KEY;
    if (!key && (h.ref_id != st->cur_frame || h.mode != st->mode))
        return DEC_REJECTED;

    unsigned col_shift = (h.mode == MODE_128x96) ? 4 : 5; // 16 or 32 tiles per row
    unsigned cols = 1u << col_shift;
    unsigned rows = (h.mode == MODE_128x96) ? 12 : (h.mode == MODE_256x384 ? 48 : 24);
    unsigned ntotal = cols * rows;

    const uint8_t *body = payload + FRAME_HDR_SIZE;
    uint32_t blen = len - FRAME_HDR_SIZE;
    if (h.flags & FLAG_LZ4)
    {
        int n = lz4_decode(body, blen, scratch, h.raw_size);
        if (n != (int)h.raw_size)
            return DEC_ERROR;
        body = scratch;
    }
    else if (blen < h.raw_size)
    {
        return DEC_ERROR;
    }

    // check sizes before touching the screen
    uint32_t pos = 0;
    uint32_t palsize = (h.flags & FLAG_PALETTE) ? ((h.npal * 2 + 3) & ~3u) : 0;
    uint32_t bmsize = key ? 0 : (((ntotal + 7) / 8 + 3) & ~3u);
    uint32_t ntiles = key ? ntotal : h.ntiles;
    if (palsize + bmsize + ntiles * 64 != h.raw_size || ntiles > ntotal)
        return DEC_ERROR;

    if (palsize)
    {
        const uint8_t *p = body;
        for (unsigned i = 0; i < h.npal; i++)
        {
            uint16_t c = rd16(p + i * 2);
            t->palette[0][i] = c;
            if (t->palette[1])
                t->palette[1][i] = c;
        }
        pos += palsize;
    }

    const uint8_t *bits = body + pos;
    pos += bmsize;
    const uint32_t *src = (const uint32_t *)(body + pos);

    if (key)
    {
        for (unsigned ty = 0; ty < rows; ty++)
        {
            uint32_t *bm = t->bitmap[ty >= 24];
            unsigned y = ty >= 24 ? ty - 24 : ty;
            for (unsigned tx = 0; tx < cols; tx++, src += 16)
                put_tile(bm, tx, y, src);
        }
    }
    else
    {
        unsigned left = ntiles;
        for (unsigned i = 0; i < ntotal && left; i += 8)
        {
            unsigned b = bits[i >> 3];
            while (b)
            {
                unsigned k = __builtin_ctz(b);
                b &= b - 1;
                unsigned ti = i + k;
                if (ti >= ntotal)
                    break;
                unsigned ty = ti >> col_shift;
                unsigned tx = ti & (cols - 1);
                put_tile(t->bitmap[ty >= 24], tx, ty >= 24 ? ty - 24 : ty, src);
                src += 16;
                left--;
            }
        }
        if (left)
            return DEC_ERROR; // bitmap and tile count disagree (picture is now partly updated)
    }

    st->cur_frame = h.frame_id;
    st->mode = h.mode;
    return key ? DEC_KEY : DEC_DELTA;
}

void reasm_init(Reassembler *r, uint8_t *buf)
{
    memset(r, 0, sizeof(*r));
    r->buf = buf;
}

uint32_t reasm_add(Reassembler *r, const uint8_t *pkt, int len)
{
    if (len < PKT_HDR_SIZE || pkt[0] != PKT_FRAME || pkt[1] != PROTO_VERSION)
        return 0;
    uint16_t fid = rd16(pkt + 2);
    unsigned idx = pkt[4];
    unsigned count = pkt[5];
    unsigned chunk = rd16(pkt + 6);
    uint32_t total = rd32(pkt + 8);
    if (idx >= count || total > MAX_FRAME_SIZE || chunk == 0)
        return 0;

    if (fid != r->frame_id || (!r->active && !r->done))
    {
        if (r->active || r->done)
        {
            // a packet of an older frame arriving late: ignore
            if ((uint16_t)(fid - r->frame_id) >= 0x8000)
                return 0;
            if (r->active)
                r->dropped++;
        }
        r->frame_id = fid;
        r->active = true;
        r->done = false;
        r->count = count;
        r->got = 0;
        r->total = total;
        memset(r->have, 0, sizeof(r->have));
    }
    if (!r->active || count != r->count || total != r->total)
        return 0;
    if (r->have[idx >> 5] & (1u << (idx & 31)))
        return 0; // duplicate

    uint32_t off = idx * chunk;
    uint32_t n = len - PKT_HDR_SIZE;
    if (off + n > total)
        return 0;
    memcpy(r->buf + off, pkt + PKT_HDR_SIZE, n);
    r->have[idx >> 5] |= 1u << (idx & 31);
    if (++r->got == r->count)
    {
        r->active = false;
        r->done = true;
        return total;
    }
    return 0;
}
