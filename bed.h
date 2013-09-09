#pragma once

typedef struct
{
        const unsigned char *input;
        size_t              input_sz;
        unsigned int        range;
        unsigned int        value;
        int                 bit_cnt;
} rtvs_bed_t;
