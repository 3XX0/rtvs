#include "rtvs.h"

void Bed_init(rtvs_bed_t *bed, const unsigned char *data, size_t size)
{
        bed->value    = (data[0] << 8) | data[1]; /* First 2 input bytes */
        bed->input    = data + 2;
        bed->input_sz = size - 2;
        bed->range    = 255; /* Initial range is full */
        bed->bit_cnt  = 0; /* Have not yet shifted out any bits */
}

static int get(rtvs_bed_t *bed, int probability)
{
        unsigned int split = 1 + (((bed->range - 1) * probability) >> 8);
        unsigned int SPLIT = split << 8;
        int          r;

        if (bed->value >= SPLIT) { /* True */
                bed->range -= split;
                bed->value -= SPLIT;
                r = 1;
        }
        else { /* False */
                bed->range = split;
                r = 0;
        }

        while (bed->range < 128) { /* Shift out irrelevant value bits */
                bed->value <<= 1;
                bed->range <<= 1;

                if (++bed->bit_cnt == 8) { /* Shift in new bits 8 at a time */
                        bed->bit_cnt = 0;
                        if (bed->input_sz) {
                                bed->value |= *bed->input++;
                                bed->input_sz--;
                        }
                }
        }
        return r;
}

int Bed_get_bit(rtvs_bed_t *bed)
{
        return get(bed, 128);
}

int Bed_get_uint(rtvs_bed_t *bed, int bits)
{
        int r = 0;

        for (int bit = bits - 1; bit >= 0; bit--)
                r |= (Bed_get_bit(bed) << bit);
        return r;
}

int Bed_get_int(rtvs_bed_t *bed, int bits)
{
        int r = 0;

        for (int bit = bits - 1; bit >= 0; bit--)
                r |= (Bed_get_bit(bed) << bit);
        return Bed_get_bit(bed) ? -r : r;
}

int Bed_maybe_get_int(rtvs_bed_t *bed, int bits)
{
        return Bed_get_bit(bed) ? Bed_get_int(bed, bits) : 0;
}
