#include <stdio.h>

#include "rtvs.h"

static FILE *fp;

static void mem_put_le16(char *mem, uint32_t val)
{
        mem[0] = val;
        mem[1] = val>>8;
}

static void mem_put_le32(char *mem, uint32_t val)
{
        mem[0] = val;
        mem[1] = val>>8;
        mem[2] = val>>16;
        mem[3] = val>>24;
}

int Muxing_open_file(const char *outfile)
{
        FAIL_ON_NULL(fp = fopen(outfile, "wb"))
        FAIL_ON_NEGATIVE(fseek(fp, 32, SEEK_SET))
        return (0);
}

int Muxing_close_file(void)
{
        if (fclose(fp) == EOF)
                return (-1);
        return (0);
}

void Muxing_ivf_write_header(const rtvs_config_t *cfg, size_t frame_num)
{
        const vpx_codec_enc_cfg_t* const vpx_cfg = &cfg->codec.vpx_cfg;

        char header[32];

        header[0] = 'D';
        header[1] = 'K';
        header[2] = 'I';
        header[3] = 'F';
        mem_put_le16(header+4,  0);                 /* version */
        mem_put_le16(header+6,  32);                /* headersize */
        mem_put_le32(header+8,  cfg->codec.fourcc); /* fourcc */
        mem_put_le16(header+12, cfg->width);        /* width */
        mem_put_le16(header+14, cfg->height);       /* height */

        /* XXX: According to the IVF spec described here http://wiki.multimedia.cx/index.php?title=IVF
         * the following parameters are the framerate and the timescale.
         * However major implementations use it as the timebase */
        mem_put_le32(header+16, vpx_cfg->g_timebase.den);
        mem_put_le32(header+20, vpx_cfg->g_timebase.num);

        mem_put_le32(header+24, frame_num);         /* frame count */
        mem_put_le32(header+28, 0);                 /* unused */

        rewind(fp);
        fwrite(header, 1, 32, fp);
}

void Muxing_ivf_write_frame(const rtvs_frame_t *frame)
{
        char                    header[12];
        const vpx_codec_pts_t   pts = frame->pts;

        mem_put_le32(header, (uint32_t) frame->size);
        mem_put_le32(header+4, pts & 0xFFFFFFFF);
        mem_put_le32(header+8, pts >> 32);

        fwrite(header, 1, 12, fp);
        fwrite(frame->data, 1, frame->size, fp);
}
