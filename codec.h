#pragma once

#include <string.h>
#include <vpx/vpx_codec.h>
#include <vpx/vpx_encoder.h>

#define CONFIG_VP9(x) (!strcmp(x->codec.name, "VP9"))
#define CONFIG_VP8(x) (!strcmp(x->codec.name, "VP8"))

enum /* Four char codes */
{
        VP8_FOURCC = 0x30385056,
        VP9_FOURCC = 0x30395056
};

typedef struct
{
        const char              *name;
        const vpx_codec_iface_t *(*cx_iface)(void);
        const vpx_codec_iface_t *(*dx_iface)(void);
        size_t                  fourcc;
        vpx_codec_enc_cfg_t     vpx_cfg;
} rtvs_codec_t;
