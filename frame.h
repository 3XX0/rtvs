#pragma once

#include <stdint.h>

#define YUV420_RATIO 3 / 2
#define YUV422_RATIO 2

enum /* VPX limits */
{
        MAX_SIMULT_FRAMES = 3,
        MAX_PARTITIONS    = 8
};

enum /* Frame flags */
{
        EMPTY        = 0x00,
        HARD_ENCODED = 0x01,
        SOFT_ENCODED = 0x02
};

typedef struct
{
        uint8_t       flags;
        unsigned char *data;
        size_t        size;
        int64_t       pts;
        unsigned int  partition_num;
        size_t        partition_size[MAX_PARTITIONS];
} rtvs_frame_t;

enum /* Header sizes */
{
        FRAME_HEADER_SIZE    = 3,
        KEYFRAME_HEADER_SIZE = 7
};

/* VP8 frame/payload header, draft-ietf-payload-vp8-09 and RFC 6386 */
typedef struct
{
#ifdef BIG_ENDIAN
        uint8_t size0   : 3; /* 1st partition size (size0+8*size1+2048*size2) */
        uint8_t showf   : 1; /* Show frame bit */
        uint8_t version : 3; /* Version number, codec profile */
        uint8_t ikf     : 1; /* Inverse key frame bit */
#else
        uint8_t ikf     : 1;
        uint8_t version : 3;
        uint8_t showf   : 1;
        uint8_t size0   : 3;
#endif
        uint8_t size1;
        uint8_t size2;
} rtvs_frame_header_t;
