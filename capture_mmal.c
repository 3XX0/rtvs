#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bcm_host.h>

#include "rtvs.h"

#include <interface/vcos/vcos.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_buffer.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_connection.h>

#define ERR_ON_MMAL_FAILURE(x) if ((x) != MMAL_SUCCESS) { goto error; };
#define PERR_ON_MMAL_FAILURE(x) do { if ((r = x) != MMAL_SUCCESS) {  \
                                     mmal_err = mmal_strerror(r);    \
                                     Capture_perror(__FUNCTION__); } \
                                   } while (0);

#define NB_BUFFER 4

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT   1

static VCOS_SEMAPHORE_T  frame_lock;
static MMAL_COMPONENT_T  *camera, *preview;
static MMAL_CONNECTION_T *preview_connection;
static MMAL_PORT_T       *preview_port, *video_port;
static MMAL_POOL_T       *video_port_pool;
static const char        *mmal_err;
static uint64_t          frame_num;
static rtvs_frame_t      framebuf;
static int64_t           old_pts;

void Capture_perror(const char *err)
{
        if (mmal_err == NULL)
                mmal_err = "Unknown error";
        fprintf(stderr, "%s: %s\n", err, mmal_err);
}

static const char *mmal_strerror(MMAL_STATUS_T status)
{
        switch (status)
        {
                case MMAL_ENOMEM:    return("Out of memory");
                case MMAL_ENOSPC:    return("Out of resources");
                case MMAL_EINVAL:    return("Invalid argument");
                case MMAL_ENOSYS:    return("Function not implemented");
                case MMAL_ENOENT:    return("No such file or directory");
                case MMAL_ENXIO:     return("No such device or address");
                case MMAL_EIO:       return("I/O error");
                case MMAL_ESPIPE:    return("Illegal seek");
                case MMAL_ECORRUPT:  return("Data is corrupted");
                case MMAL_ENOTREADY: return("Component is not ready");
                case MMAL_ECONFIG:   return("Component is not configured");
                case MMAL_EISCONN:   return("Port is already connected");
                case MMAL_ENOTCONN:  return("Port is disconnected");
                case MMAL_EAGAIN:    return("Resource temporarily unavailable");
                case MMAL_EFAULT:    return("Bad address");
                default:             return("Unknown error");
        }
}

static int camera_default_parameters(MMAL_COMPONENT_T *camera, const rtvs_config_t *cfg)
{
        MMAL_STATUS_T r;

        int                                   rotation       = ((cfg->capture.rotation % 360 ) / 90) * 90;
        MMAL_RATIONAL_T                       saturation     = {cfg->capture.saturation, 100};
        MMAL_RATIONAL_T                       sharpness      = {cfg->capture.sharpness, 100};
        MMAL_RATIONAL_T                       contrast       = {cfg->capture.contrast, 100};
        MMAL_RATIONAL_T                       brightness     = {cfg->capture.brightness, 100};
        MMAL_PARAMETER_MIRROR_T               mirror         = {{MMAL_PARAMETER_MIRROR, sizeof(mirror)}, MMAL_PARAM_MIRROR_NONE};
        MMAL_PARAMETER_EXPOSUREMODE_T         exposure_mode  = {{MMAL_PARAMETER_EXPOSURE_MODE, sizeof(exposure_mode)}, cfg->capture.exposure_mode};
        MMAL_PARAMETER_EXPOSUREMETERINGMODE_T exposure_meter = {{MMAL_PARAMETER_EXP_METERING_MODE, sizeof(exposure_meter)}, cfg->capture.exposure_meter};
        MMAL_PARAMETER_AWBMODE_T              awb_mode       = {{MMAL_PARAMETER_AWB_MODE, sizeof(awb_mode)}, cfg->capture.awb_mode};
        MMAL_PARAMETER_IMAGEFX_T              image_fx       = {{MMAL_PARAMETER_IMAGE_EFFECT, sizeof(image_fx)}, cfg->capture.image_fx};

        /* Rotation */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_int32(camera->output[MMAL_CAMERA_PREVIEW_PORT], MMAL_PARAMETER_ROTATION, rotation))
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_int32(camera->output[MMAL_CAMERA_VIDEO_PORT], MMAL_PARAMETER_ROTATION, rotation))
        /* Saturation */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SATURATION, saturation))
        /* Sharpness */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SHARPNESS, sharpness))
        /* Contrast */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_CONTRAST, contrast))
        /* Brightness */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_BRIGHTNESS, brightness))
        /* ISO */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_ISO, cfg->capture.iso))
        /* Video stabilisation */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_boolean(camera->control, MMAL_PARAMETER_VIDEO_STABILISATION, cfg->capture.stabilize))
        /* Flips */
        if (cfg->capture.hflip && cfg->capture.vflip)
                mirror.value = MMAL_PARAM_MIRROR_BOTH;
        else if (cfg->capture.hflip)
                mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
        else if (cfg->capture.vflip)
                mirror.value = MMAL_PARAM_MIRROR_VERTICAL;
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->output[MMAL_CAMERA_PREVIEW_PORT], &mirror.hdr))
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->output[MMAL_CAMERA_VIDEO_PORT], &mirror.hdr))
        /* Shutter speed */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_SHUTTER_SPEED, cfg->capture.shutter_speed))
        /* Exposure compensation */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_int32(camera->control, MMAL_PARAMETER_EXPOSURE_COMP, cfg->capture.exposure_comp))
        /* Exposure mode */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->control, &exposure_mode.hdr));
        /* Exposure meter mode */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->control, &exposure_meter.hdr))
        /* Auto white balance mode */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->control, &awb_mode.hdr))
        /* Image effect */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->control, &image_fx.hdr))

        return (0);
error:
        mmal_err = mmal_strerror(r);
        return (-1);
}

static void camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buf)
{
        (void) port;
        mmal_buffer_header_release(buf);
}

static void video_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buf)
{
        MMAL_POOL_T *video_port_pool = (MMAL_POOL_T *) port->userdata;

        /* Get the frame from the video buffer */
        mmal_buffer_header_mem_lock(buf);
        memcpy(framebuf.data, buf->data, buf->length);
        framebuf.size = buf->length;
        mmal_buffer_header_mem_unlock(buf);

        if (vcos_semaphore_trywait(&frame_lock) != VCOS_SUCCESS)
                vcos_semaphore_post(&frame_lock);

        mmal_buffer_header_release(buf);

        /* Enqueue a fresh buffer */
        if (port->is_enabled) {
                buf = mmal_queue_get(video_port_pool->queue);
                if (!buf || mmal_port_send_buffer(port, buf) != MMAL_SUCCESS)
                        fprintf(stderr, "Could not return mmal buffer\n");
        }
}

int Capture_start(rtvs_config_t *cfg)
{
        MMAL_STATUS_T    r;
        MMAL_ES_FORMAT_T *format;

        /* Initialize the GPU */
        bcm_host_init();

        vcos_semaphore_create(&frame_lock, "frame locker", 0); // TODO error
        framebuf.data = malloc(cfg->width * cfg->height * YUV420_RATIO);
        if (!framebuf.data) {
                r = MMAL_ENOMEM;
                goto error;
        }

        /* Create components (no preview required) */
        ERR_ON_MMAL_FAILURE(r = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera))
        ERR_ON_MMAL_FAILURE(r = mmal_component_create("vc.null_sink", &preview))

        /* Setup ports */
        assert(camera->output_num != 0);
        preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
        video_port   = camera->output[MMAL_CAMERA_VIDEO_PORT];

        /* Setup the camera controls and default parameters */
        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
        {
                { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
                .max_stills_w = cfg->width,
                .max_stills_h = cfg->height,
                .stills_yuv422 = 0,
                .one_shot_stills = 0,
                .max_preview_video_w = cfg->width,
                .max_preview_video_h = cfg->height,
                .num_preview_video_frames = 2,
                .stills_capture_circular_buffer_height = 0,
                .fast_preview_resume = 1,
                .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
        };
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set(camera->control, &cam_config.hdr))
        ERR_ON_MMAL_FAILURE(r = mmal_port_enable(camera->control, &camera_control_callback))
        if (camera_default_parameters(camera, cfg) < 0)
                return (-1);

        /* Preview port format */
        format = preview_port->format;
        format->encoding = MMAL_ENCODING_I420;
        format->encoding_variant = MMAL_ENCODING_I420;
        format->es->video.width = cfg->width;
        format->es->video.height = cfg->height;
        format->es->video.crop.x = 0;
        format->es->video.crop.y = 0;
        format->es->video.crop.width = cfg->width;
        format->es->video.crop.height = cfg->height;
        format->es->video.frame_rate.num = cfg->framerate;
        format->es->video.frame_rate.den = 1;

        ERR_ON_MMAL_FAILURE(r = mmal_port_format_commit(preview_port))

        /* Video port format */
        mmal_format_full_copy(video_port->format, format); /* Copy the preview format */
        ERR_ON_MMAL_FAILURE(r = mmal_port_format_commit(video_port))

        /* Create buffers pool */
        video_port->buffer_num = NB_BUFFER;
        video_port->buffer_size = cfg->width * cfg->height * YUV420_RATIO;
        video_port_pool = (MMAL_POOL_T *) mmal_port_pool_create(video_port, video_port->buffer_num, video_port->buffer_size);
        video_port->userdata = (struct MMAL_PORT_USERDATA_T *) video_port_pool;

        /* Video callback */
        ERR_ON_MMAL_FAILURE(r = mmal_port_enable(video_port, video_buffer_callback))

        /* Enable components */
        ERR_ON_MMAL_FAILURE(r = mmal_component_enable(preview))
        ERR_ON_MMAL_FAILURE(r = mmal_component_enable(camera))

        /* Connect the video port to the null_sink preview (see RPI_README) */
        ERR_ON_MMAL_FAILURE(r = mmal_connection_create(&preview_connection, preview_port, preview->input[0],
                                MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT))
        ERR_ON_MMAL_FAILURE(r = mmal_connection_enable(preview_connection))

        /* Queue the buffers on the video port */
        int buf_num = mmal_queue_length(video_port_pool->queue);

        for (int i = 0; i < buf_num; ++i) {
            MMAL_BUFFER_HEADER_T *buf = mmal_queue_get(video_port_pool->queue);
            if (!buf || mmal_port_send_buffer(video_port, buf) != MMAL_SUCCESS)
                    fprintf(stderr, "Could not setup mmal buffer\n");
        }

        /* Start the capture */
        ERR_ON_MMAL_FAILURE(r = mmal_port_parameter_set_boolean(video_port, MMAL_PARAMETER_CAPTURE, 1))

        return (0);
error:
        mmal_err = mmal_strerror(r);
        return (-1);
}

void Capture_stop(void)
{
        MMAL_STATUS_T r;

        free(framebuf.data);
        framebuf.data = NULL;

        /* Stop the capture */
        if (video_port)
                PERR_ON_MMAL_FAILURE(mmal_port_parameter_set_boolean(video_port, MMAL_PARAMETER_CAPTURE, 0))

        /* Unqueue the buffers */
        if (video_port_pool) {
                mmal_queue_destroy(video_port_pool->queue);
                video_port_pool = NULL;
        }

        /* Destroy the video/preview connection */
        if (preview_connection) {
                PERR_ON_MMAL_FAILURE(mmal_connection_disable(preview_connection))
                PERR_ON_MMAL_FAILURE(mmal_connection_destroy(preview_connection))
                preview_connection = NULL;
        }

        /* Disable all ports */
        if (video_port && video_port->is_enabled)
                PERR_ON_MMAL_FAILURE(mmal_port_disable(video_port))
        if (camera && camera->control->is_enabled)
                PERR_ON_MMAL_FAILURE(mmal_port_disable(camera->control))
        video_port = NULL;
        preview_port = NULL;

        /* Destroy components */
        if (preview) {
                PERR_ON_MMAL_FAILURE(mmal_component_disable(preview))
                PERR_ON_MMAL_FAILURE(mmal_component_destroy(preview))
                preview = NULL;
        }
        if (camera) {
                PERR_ON_MMAL_FAILURE(mmal_component_disable(camera))
                PERR_ON_MMAL_FAILURE(mmal_component_destroy(camera))
                camera = NULL;
        }
}

int Capture_get_frame(rtvs_frame_t *frame)
{
        alarm(5);
        if (vcos_semaphore_wait(&frame_lock) != VCOS_SUCCESS) // TODO error
                return (-1);
        alarm(0);

        frame->data = framebuf.data;
        frame->size = framebuf.size;
        if ((frame->pts = get_curtime()) < 0) {
                mmal_err = strerror(errno);
                return (-1);
        }
        ++frame_num;

        if (old_pts) {
                printf("\rfps = %.2f, frames = %" PRIu64,
                    (float) 1000 / (frame->pts - old_pts), frame_num);
                fflush(stdout);
        }
        old_pts = frame->pts;

        return (0);
}
