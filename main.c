#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>
#include <string.h>
#include <sys/select.h>

#include "rtvs.h"

#include <vpx/vp8cx.h>
#include <vpx/vp8dx.h>

static volatile int sigcatch;

static void rtvs_config_default(rtvs_config_t *cfg, int codec)
{
        /* Global configuration */
        cfg->device         = "/dev/video0";
        cfg->framerate      = 30;
        cfg->bitrate        = 400;
        cfg->thread_num     = 1;
        cfg->width          = 320;
        cfg->height         = 240;

        /* Codec configuration */
        if (codec == VP8) {
                cfg->codec.name     = "VP8";
                cfg->codec.cx_iface = &vpx_codec_vp8_cx;
                cfg->codec.dx_iface = &vpx_codec_vp8_dx;
                cfg->codec.fourcc   = VP8_FOURCC;
        }
#ifdef HAS_VP9_SUPPORT
        else if (codec == VP9) {
                cfg->codec.name     = "VP9";
                cfg->codec.cx_iface = &vpx_codec_vp9_cx;
                cfg->codec.dx_iface = &vpx_codec_vp9_dx;
                cfg->codec.fourcc   = VP9_FOURCC;
        }
#endif

        /* Capture configuration */
        /* TODO Add support for V4L2 parameters as well */
#ifdef WITH_MMAL_CAPTURE
        cfg->capture.rotation             = 0;
        cfg->capture.saturation           = 0;
        cfg->capture.sharpness            = 0;
        cfg->capture.contrast             = 0;
        cfg->capture.brightness           = 50;
        cfg->capture.iso                  = 0;
        cfg->capture.stabilize            = 0;
        cfg->capture.hflip                = 0;
        cfg->capture.vflip                = 0;
        cfg->capture.shutter_speed        = 0;
        cfg->capture.exposure_comp        = 0;
        cfg->capture.exposure_mode        = MMAL_PARAM_EXPOSUREMODE_AUTO;
        cfg->capture.exposure_meter       = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;
        cfg->capture.awb_mode             = MMAL_PARAM_AWBMODE_AUTO;
        cfg->capture.image_fx             = MMAL_PARAM_IMAGEFX_NONE;
#endif
}

static void sig_handler(int sig)
{
        printf("Received signal: %s, exiting...\n", strsignal(sig));
        sigcatch = 1;
}

static int _kbhit(void)
{
        fd_set         fds;
        struct timeval tv;

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

int main(int argc, char **argv)
{
        struct sigaction act;
        rtvs_frame_t     frames[MAX_SIMULT_FRAMES];
        size_t           frame_num = 0;
        rtvs_config_t    cfg;
        int              opt;

        const char       *mux_parm    = NULL;
        char             *stream_parm = NULL;

        BZERO_STRUCT(act)
        act.sa_handler = &sig_handler;
        if (sigaction(SIGTERM, &act, NULL) < 0 ||
            sigaction(SIGQUIT, &act, NULL) < 0 ||
            sigaction(SIGALRM, &act, NULL) < 0 ||
            sigaction(SIGINT, &act, NULL) < 0) {
                perror("Could not setup signal handlers");
                return (-1);
        }

        rtvs_config_default(&cfg, VP8);
        while ((opt = getopt(argc, argv, "qhs:m:d:f:x:b:c:t:")) != -1) {
                switch (opt) {
                case 's':
                        if (strchr(optarg, ':') == NULL)
                                goto usage;
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
                case 'x':
                {
                        char *p;
                        if ((p = strchr(optarg, 'x')) == NULL)
                                goto usage;
                        cfg.width = atoi(optarg);
                        cfg.height = atoi(p + 1);
                        break;
                }
                case 'b':
                        cfg.bitrate = atoi(optarg);
                        break;
                case 't':
                        cfg.thread_num = atoi(optarg);
                        break;
                case 'q':
                        freopen("/dev/null", "w", stdout);
                        break;
usage:
                case 'h':
                default:
                        fprintf(stderr, "Usage: %s [-qh] [-s [ip]:port] [-m file.ivf] [-d device] [-f framerate]"
                            " [-x widthxheight] [-b bitrate] [-t threads]\n", argv[0]);
                        exit(0);
                }
        }

        /* Startup */
        if (Capture_start(&cfg) < 0) {
                Capture_perror("Could not start capturing");
                goto cleanup;
        }
        if (Encoder_start(&cfg) < 0) {
                Encoder_perror("Could not start encoding");
                goto cleanup;
        }
        if (mux_parm && Muxing_open_file(mux_parm) < 0) {
                perror("Could not open mux file");
                goto cleanup;
        }
        if (stream_parm) {
                Packetizer_init();
                if (Rtp_start(stream_parm) < 0) {
                        perror("Could not start RTP transport");
                        goto cleanup;
                }
        }

        printf("\n%s --- Configuration ---\n"
            " | device: %s\n | codec: %s\n | resolution: %ux%u\n | fps: %u\n | bitrate: %u\n | threads: %u%s\n\n",
            CBLUE, cfg.device, cfg.codec.name, cfg.width, cfg.height, cfg.framerate, cfg.bitrate, cfg.thread_num, CDFLT);

        /* Capture loop */
        while (!_kbhit() && !sigcatch) {
                BZERO_ARRAY(frames)
                if (Capture_get_frame(frames) < 0) {
                        Capture_perror("Fetching frame failed");
                        continue;
                }
                if (Encoder_encode_frame(&cfg, frames) < 0) {
                        Encoder_perror("Encoding frame failed");
                        continue;
                }
                for (int i = 0; i < MAX_SIMULT_FRAMES; ++i)
                        if (frames[i].flags != EMPTY) {
                                if (mux_parm)
                                        Muxing_ivf_write_frame(frames + i);
                                if (stream_parm && Packetizer_packetize(frames + i) < 0)
                                        perror("Sending failed");
                                ++frame_num;
                        }
        }

cleanup: /* Cleanup */
        if (stream_parm)
                Rtp_stop();
        if (mux_parm) {
                Muxing_ivf_write_header(&cfg, frame_num);
                Muxing_close_file();
        }
        Encoder_stop();
        Capture_stop();

        return (0);
}
