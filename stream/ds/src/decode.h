// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: reassembly and decoding of frames (see stream/PROTOCOL.md).
// Plain C without libnds so it can also be built and tested on a PC
// (stream/ds/hosttest).

#ifndef DSSTREAM_DECODE_H
#define DSSTREAM_DECODE_H

#include <stdbool.h>
#include <stdint.h>

#define PROTO_VERSION   1

#define PKT_FRAME       'F'
#define PKT_HELLO       'H'
#define PKT_HERE        'W'
#define PKT_INPUT       'I'
#define PKT_BYE         'Q'

#define FLAG_KEY        0x01
#define FLAG_PALETTE    0x02
#define FLAG_LZ4        0x04

#define MODE_256x192    0
#define MODE_128x96     1
#define MODE_256x384    2
#define MODE_NONE       0xFF

#define IN_NEED_REF     0x01
#define IN_HELLO        0x02

#define FRAME_HDR_SIZE  16
#define PKT_HDR_SIZE    12
#define INPUT_SIZE      20

// largest body: palette 512 + bitmap 192 + 1536 tiles * 64
#define MAX_RAW_SIZE    (512 + 192 + 1536 * 64)
#define MAX_FRAME_SIZE  (FRAME_HDR_SIZE + MAX_RAW_SIZE + MAX_RAW_SIZE / 255 + 64)

// Results of frame_decode()
#define DEC_KEY         1
#define DEC_DELTA       2
#define DEC_REJECTED    0   // delta frame for another reference: DS must ask for help
#define DEC_ERROR       -1  // broken frame

// Where decoded pixels go. bitmap[i] is an 8 bpp bitmap with a row stride of
// 256 bytes (DS extended rotation BG 256x256), written only with aligned 32-bit
// stores so it may point to VRAM. bitmap[1] is the bottom half in 256x384 mode.
typedef struct {
    uint32_t *bitmap[2];
    volatile uint16_t *palette[2];  // palette[1] may be NULL
} DecodeTarget;

typedef struct {
    uint16_t cur_frame;     // last applied frame, 0 = none
    uint8_t mode;           // mode of the shown picture, MODE_NONE before the first keyframe
} DecodeState;

// Frame header fields (little-endian in the payload)
typedef struct {
    uint8_t flags;
    uint8_t mode;
    uint16_t frame_id;
    uint16_t ref_id;
    uint16_t ntiles;
    uint32_t raw_size;
    uint16_t npal;
} FrameHeader;

bool frame_header_read(const uint8_t *p, uint32_t len, FrameHeader *h);

// Decodes one complete frame payload. scratch must be 4-byte aligned and hold
// MAX_RAW_SIZE bytes; payload must be 4-byte aligned.
int frame_decode(const uint8_t *payload, uint32_t len, DecodeState *st, const DecodeTarget *t,
                 uint8_t *scratch);

// LZ4 block decoder. Returns the number of bytes written or -1.
int lz4_decode(const uint8_t *src, int srclen, uint8_t *dst, int dstlen);

// Reassembly of frame packets: one frame at a time, the first packet of a newer
// frame drops an incomplete older one.
typedef struct {
    uint8_t *buf;           // MAX_FRAME_SIZE bytes, 4-byte aligned
    uint16_t frame_id;
    bool active;            // collecting packets of frame_id
    bool done;              // frame_id was completed (ignore duplicates)
    uint8_t count;
    uint8_t got;
    uint32_t total;
    uint32_t have[8];       // bit set per received packet index
    uint32_t dropped;       // incomplete frames given up
} Reassembler;

void reasm_init(Reassembler *r, uint8_t *buf);
// Returns the frame length when pkt completed a frame (data in r->buf), else 0.
uint32_t reasm_add(Reassembler *r, const uint8_t *pkt, int len);

static inline uint16_t rd16(const uint8_t *p) { return p[0] | (p[1] << 8); }
static inline uint32_t rd32(const uint8_t *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void wr16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static inline void wr32(uint8_t *p, uint32_t v)
{
    p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

#endif
