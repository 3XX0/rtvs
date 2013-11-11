#pragma once

#include "codec.h"
#include "capture.h"

enum
{
    VP8,
    VP9
};

typedef struct
{
        const char     *device;
        unsigned int   framerate;
        unsigned int   bitrate;
        unsigned int   thread_num;
        unsigned int   height;
        unsigned int   width;
        rtvs_codec_t   codec;
        rtvs_capture_t capture;
} rtvs_config_t;
