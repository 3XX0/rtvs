#include "rtvs.h"

enum /* Macroblocks segment properties */
{
        MB_FEATURE_TREE_PROBS = 3,
        MAX_MB_SEGMENTS = 4
};

enum
{
        BLOCK_CONTEXTS = 4
};

static void skip_segmentation_header(rtvs_bed_t *bed)
{
        if (Bed_get_bit(bed)) {
                int update_map = Bed_get_bit(bed);
                int update_data = Bed_get_bit(bed);

                if (update_data) {
                        Bed_get_bit(bed);
                        for (int i = 0; i < MAX_MB_SEGMENTS; ++i)
                                Bed_maybe_get_int(bed, 7);
                        for (int i = 0; i < MAX_MB_SEGMENTS; ++i)
                                Bed_maybe_get_int(bed, 6);
                }
                if (update_map)
                        for (int i = 0; i < MB_FEATURE_TREE_PROBS; ++i)
                                Bed_get_bit(bed) ? Bed_get_uint(bed, 8) : 255;
        }
}

static void skip_loopfilter_header(rtvs_bed_t *bed)
{
        Bed_get_bit(bed);
        Bed_get_uint(bed, 6);
        Bed_get_uint(bed, 3);

        if (Bed_get_bit(bed) && Bed_get_bit(bed)) {
                for (int i = 0; i < BLOCK_CONTEXTS; ++i)
                        Bed_maybe_get_int(bed, 6);
                for (int i = 0; i < BLOCK_CONTEXTS; ++i)
                        Bed_maybe_get_int(bed, 6);
        }
}

/*
 * VP8 Frame format :
 *
 * +---------------------------------+
 * |         Frame header (3)        |
 * +---------------------------------+
 * |      Key frame header (7)       |
 * +---------------------------------+
 * |         First partition         |
 * +---------------------------------+
 * | Array of N partition size (Nx3) |
 * +---------------------------------+
 * |          N partitions           |
 * +---------------------------------+
 */

void Frame_init_partitions(rtvs_frame_t *frame)
{
        rtvs_bed_t bed;

        const rtvs_frame_header_t* const frame_hdr = (const rtvs_frame_header_t *) frame->data;

        /* Get the 1st partition */
        frame->partition_size[0] = frame_hdr->size0 | (frame_hdr->size1 << 3) | (frame_hdr->size2 << 11);
        frame->partition_num = 1;

        const unsigned char *data = frame->data + FRAME_HEADER_SIZE;
        size_t               size = frame->size - FRAME_HEADER_SIZE;

        if (!frame_hdr->ikf) {
                data += KEYFRAME_HEADER_SIZE;
                size -= KEYFRAME_HEADER_SIZE;
        }

        /* Initialize the boolean entropy decoder */
        Bed_init(&bed, data, frame->partition_size[0]);

        if (!frame_hdr->ikf) /* Skip colorspace and clamping bits */
                Bed_get_uint(&bed, 2);
        skip_segmentation_header(&bed);
        skip_loopfilter_header(&bed);

        data += frame->partition_size[0];
        size -= frame->partition_size[0];

        frame->partition_num += 1 << Bed_get_uint(&bed, 2);
        size -= 3 * (frame->partition_num - 2);

        for (unsigned int i = 1; i < frame->partition_num; ++i) {
                if (i < frame->partition_num - 1) {
                        frame->partition_num = (data[2] << 16) | (data[1] << 8) | data[0];
                        data += 3;
                }
                else
                        frame->partition_size[i] = size;
                size -= frame->partition_size[i];
        }
}
