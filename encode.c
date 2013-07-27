#include <assert.h>

#include "rtvs.h"

#include <libswscale/swscale.h>
#include <vpx/vp8cx.h>

static vpx_codec_ctx_t      codec;
static vpx_image_t          yuyv_img;
static vpx_image_t          yuv420_img;
static struct SwsContext    *scale_ctx;

int Encoder_start(rtvs_config_t *cfg)
{
        vpx_codec_enc_cfg_t* const vpx_cfg = &cfg->codec.vpx_cfg;

        FAIL_ON_NONZERO(vpx_codec_enc_config_default(cfg->codec.cx_iface(), vpx_cfg, 0))

        vpx_cfg->rc_target_bitrate = cfg->bitrate; /* Stream data rate in kb/s */
        vpx_cfg->g_h = cfg->height;                /* Frame resolution */
        vpx_cfg->g_w = cfg->width;
        vpx_cfg->g_timebase.num = 1;               /* Stream timebase units (ms) */
        vpx_cfg->g_timebase.den = (int) 1000;
        vpx_cfg->rc_min_quantizer = 4;             /* Best quality quantizer */
        vpx_cfg->rc_max_quantizer = 50;            /* Worst quality quantizer */
        vpx_cfg->g_pass = VPX_RC_ONE_PASS;         /* One encoding pass only */
        vpx_cfg->g_error_resilient = 1;            /* Enable error resilient mode */
        //vpx_cfg->kf_mode = VPX_KF_DISABLED;        /* GOP, Disable key frame placement (manual placement) */
        //vpx_cfg->kf_max_dist = 999999;
        vpx_cfg->g_threads = cfg->thread_num;      /* Thread num */
        vpx_cfg->g_lag_in_frames = 0;              /* Disable cached frames */
        vpx_cfg->rc_dropframe_thresh = 1;          /* Allow frame dropping to meet the data rate requirements */
        vpx_cfg->rc_end_usage = VPX_CBR;           /* Constant bitrate mode */
        vpx_cfg->rc_buf_sz = 6000;                 /* Encoder buffer size in ms of encoded data (1s) */
        vpx_cfg->rc_buf_initial_sz = 4000;
        vpx_cfg->rc_buf_optimal_sz = 5000;

        FAIL_ON_NONZERO(vpx_codec_enc_init(&codec, cfg->codec.cx_iface(), vpx_cfg, 0))

        vpx_codec_control(&codec, VP8E_SET_CPUUSED, 4);                         /* CPU utilisation bound */
        vpx_codec_control(&codec, VP8E_SET_STATIC_THRESHOLD, 500);              /* Control the threshold on blocks encoding (< 1000)*/
        vpx_codec_control(&codec, VP8E_SET_ENABLEAUTOALTREF, 0);                /* Disable automatic reference frames */
        if (cfg->thread_num > 1)
                vpx_codec_control(&codec, VP8E_SET_TOKEN_PARTITIONS, 2);        /* Split the coefficient encoding for parallel processing */
#ifdef USE_VP9
        if (CONFIG_VP9(cfg))
                vpx_codec_control(&codec, VP9E_SET_FRAME_PARALLEL_DECODING, 1); /* Enable parallel decoding */
#endif

        /* Initialize scaling context for subsampling conversion 4:2:2 -> 4:2:0 */
        scale_ctx = sws_getContext(cfg->width, cfg->height, PIX_FMT_YUYV422,
            cfg->width, cfg->height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

        FAIL_ON_ZERO(vpx_img_alloc(&yuv420_img, VPX_IMG_FMT_I420, cfg->width, cfg->height, 1))
        FAIL_ON_ZERO(vpx_img_alloc(&yuyv_img, VPX_IMG_FMT_YUY2, cfg->width, cfg->height, 1))
        return (0);
}

int Encoder_stop(void)
{
        vpx_img_free(&yuv420_img);
        vpx_img_free(&yuyv_img);
        FAIL_ON_NONZERO(vpx_codec_destroy(&codec))
        return (0);
}

int Encoder_encode_frame(const rtvs_config_t *cfg, rtvs_frame_t *frames)
{
        if ((frames[0].flags & HARD_ENCODED))
                return (0);

        const vpx_codec_cx_pkt_t         *pkt;
        vpx_codec_iter_t                 iter = NULL;
        const vpx_codec_enc_cfg_t* const vpx_cfg = &cfg->codec.vpx_cfg;

        const vpx_codec_pts_t pts      = frames[0].pts;
        const unsigned long   duration = (double) (1 / cfg->framerate) / /* timeperframe / timebase */
                                         (vpx_cfg->g_timebase.num / vpx_cfg->g_timebase.den);

        assert(frames[0].size == (size_t) cfg->width * cfg->height * 2);
        memcpy(yuyv_img.planes[0], frames[0].data, frames[0].size);

        /* Scaling and subsampling conversion */
        FAIL_ON_NEGATIVE(sws_scale(scale_ctx, (const uint8_t * const *) yuyv_img.planes, yuyv_img.stride,
            0, cfg->height, yuv420_img.planes, yuv420_img.stride))

        FAIL_ON_NONZERO(vpx_codec_encode(&codec, &yuv420_img, pts, duration, 0, VPX_DL_REALTIME))
        while ((pkt = vpx_codec_get_cx_data(&codec, &iter))) {
                if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
                        frames->flags = SOFT_ENCODED;
                        frames->data  = pkt->data.frame.buf;
                        frames->size  = pkt->data.frame.sz;
                        frames->pts   = pkt->data.frame.pts;
                        ++frames;
                }
        }
        return (0);
}
