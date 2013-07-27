#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>
#include <sys/select.h>

#include "rtvs.h"

#include <vpx/vp8cx.h>
#include <vpx/vp8dx.h>

static void rtvs_config_default(rtvs_config_t *cfg)
{
        cfg->device         = "/dev/video0";
        cfg->framerate      = 30;
        cfg->bitrate        = 400;
        cfg->thread_num     = 1;
        cfg->width          = 320;
        cfg->height         = 240;

        cfg->codec.name     = "VP8";
        cfg->codec.cx_iface = &vpx_codec_vp8_cx;
        cfg->codec.dx_iface = &vpx_codec_vp8_dx;
        cfg->codec.fourcc   = VP8_FOURCC;
}

static int _kbhit(void)
{
        fd_set          fds;
        struct timeval  tv;

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(0, &fds);

        if (select(1, &fds, NULL, NULL, &tv) == -1)
                return (0);
        if (FD_ISSET(0, &fds))
                return (1);
        return (0);
}

/* TODO Handle vpx error codes */
int main(int argc, char **argv)
{
        rtvs_frame_t    frames[MAX_SIMULT_FRAMES];
        size_t          frame_num = 0;
        rtvs_config_t   cfg;
        int             opt;

        const char      *mux_parm    = NULL;
        const char      *stream_parm = NULL;

        rtvs_config_default(&cfg);
        while ((opt = getopt(argc, argv, "s:m:d:f:h:w:b:c:t:")) != -1) {
                switch (opt) {
                case 's':
                        stream_parm = optarg;
                        break;
                case 'm':
                        mux_parm = optarg;
                        break;
                case 'd':
                        cfg.device = optarg;
                        break;
                case 'f':
                        cfg.framerate = atoi(optarg);
                        break;
                case 'h':
                        cfg.height = atoi(optarg);
                        break;
                case 'w':
                        cfg.width = atoi(optarg);
                        break;
                case 'b':
                        cfg.bitrate = atoi(optarg);
                        break;
                case 't':
                        cfg.thread_num = atoi(optarg);
                        break;
                default:
                        fprintf(stderr, "Usage: %s [-s ip:port] [-m file.ivf] [-d device] [-f framerate] [-h height]"
                            "[-w width] [-b bitrate] [-t threads]\n", argv[0]);
                        exit(0);
                }
        }

        if (Capture_start(&cfg) < 0)
                err(1, "Could not start capturing");
        if (Encoder_start(&cfg) < 0)
                errx(1, "Could not start encoding");
        if (mux_parm && Muxing_open_file(mux_parm) < 0)
                err(1, "Could not open mux file");

        printf("\n%s --- Configuration ---\n"
            " | device: %s\n | codec: %s\n | resolution: %dx%d\n | fps: %d\n | bitrate: %d\n | threads: %d%s\n",
            CBLUE, cfg.device, cfg.codec.name, cfg.width, cfg.height, cfg.framerate, cfg.bitrate, cfg.thread_num, CDFLT);

        while (!_kbhit()) {
                BZERO_ARRAY(frames)
                if (Capture_get_frame(frames) < 0) {
                        perror("Fetching frame failed");
                        continue;
                }
                if (Encoder_encode_frame(&cfg, frames) < 0) {
                        fprintf(stderr, "Encoding frame failed\n");
                        continue;
                }
                for (int i = 0; i < MAX_SIMULT_FRAMES; ++i)
                        if (!(frames[i].flags & EMPTY)) {
                                if (mux_parm)
                                        Muxing_ivf_write_frame(frames + i);
                                ++frame_num;
                        }
        }

        if (mux_parm) {
                Muxing_ivf_write_header(&cfg, frame_num);
                if (Muxing_close_file() < 0)
                        err(1, "Could not close mux file");
        }
        if (Encoder_stop() < 0)
                errx(1, "Could not stop encoding");
        if (Capture_stop() < 0)
                err(1, "Could not stop capturing");

        return (0);
}
