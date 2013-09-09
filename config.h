#pragma once

#include "codec.h"

typedef struct
{
        const char   *device;
        unsigned int framerate;
        unsigned int bitrate;
        unsigned int thread_num;
        unsigned int height;
        unsigned int width;
        rtvs_codec_t codec;
} rtvs_config_t;
