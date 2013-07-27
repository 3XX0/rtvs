#pragma once

#include <string.h>

#include <vpx/vpx_codec.h>
#include <vpx/vpx_encoder.h>

/* Termcap colors */

#define CRED   "\x1b[31;1m"
#define CGREEN "\x1b[32;1m"
#define CBLUE  "\x1b[34;1m"
#define CDFLT  "\x1b[0m"

/* Useful macros definitions */

#define FAIL_ON_ZERO(x) if (!(x)) { return (-1); };
#define FAIL_ON_NULL(x) if ((x) == NULL) { return (-1); };
#define FAIL_ON_NONZERO(x) if ((x)) { return (-1); };
#define FAIL_ON_NEGATIVE(x) if ((x) < 0) { return (-1); };
#define BZERO_STRUCT(x) memset(&(x), 0, sizeof(x));
#define BZERO_ARRAY(x) memset((x), 0, sizeof(x));

/* VPX facilities */

#define VP8_FOURCC (0x30385056)
#define VP9_FOURCC (0x30395056)

#define CONFIG_VP9(x) (!strcmp(x->codec.name, "VP9"))
#define CONFIG_VP8(x) (!strcmp(x->codec.name, "VP8"))

/* Control data structures */

typedef struct rtvs_codec
{
        const char              *name;
        const vpx_codec_iface_t *(*cx_iface)(void);
        const vpx_codec_iface_t *(*dx_iface)(void);
        size_t                  fourcc;
        vpx_codec_enc_cfg_t     vpx_cfg;
} rtvs_codec_t;

typedef struct config
{
        const char   *device;
        int          framerate;
        int          bitrate;
        int          thread_num;
        int          height;
        int          width;
        rtvs_codec_t codec;
} rtvs_config_t;

enum
{
        EMPTY        = 0x00,
        HARD_ENCODED = 0x01,
        SOFT_ENCODED = 0x02
};

#define MAX_SIMULT_FRAMES 3

typedef struct frame
{
        uint8_t     flags;
        const char  *data;
        size_t      size;
        int64_t     pts;
} rtvs_frame_t;

/* Modules definitions */

extern int Capture_start(rtvs_config_t *cfg);
extern int Capture_stop(void);
extern int Capture_get_frame(rtvs_frame_t *frame);

extern int Encoder_start(rtvs_config_t *cfg);
extern int Encoder_stop(void);
extern int Encoder_encode_frame(const rtvs_config_t *cfg, rtvs_frame_t *frames);

extern int Muxing_open_file(const char *outfile);
extern int Muxing_close_file(void);
extern void Muxing_ivf_write_header(const rtvs_config_t *cfg, size_t frame_num);
extern void Muxing_ivf_write_frame(const rtvs_frame_t *frame);
