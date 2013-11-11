#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "rtvs.h"

#include <linux/videodev2.h>

#define NB_BUFFER 4
static struct
{
        void   *ptr;
        size_t size;
}                         membuf[NB_BUFFER];
static int                fd = -1;
static fd_set             fds;
static struct v4l2_buffer buf;
static int                streamon, hwcx_supported;
static int64_t            old_pts;
static uint64_t           frame_num;

int Capture_start(rtvs_config_t *cfg)
{
        struct v4l2_streamparm     setfps;
        struct v4l2_capability     cap;
        struct v4l2_format         fmt;
        struct v4l2_requestbuffers reqb;

        FAIL_ON_NEGATIVE(fd = open(cfg->device, O_RDWR | O_NONBLOCK))
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Check device capabilities */
        BZERO_STRUCT(cap)
        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_QUERYCAP, &cap))
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                errno = ENODEV;
                fprintf(stderr, "%s is no video capture device\n", cfg->device);
                return (-1);
        }
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                errno = ENOTSUP;
                fprintf(stderr, "%s does not support streaming\n", cfg->device);
                return (-1);
        }

        /* Capture format settings */
        BZERO_STRUCT(fmt)
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = cfg->width;
        fmt.fmt.pix.height = cfg->height;
#ifdef HAS_VP8_HW_SUPPORT
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_VP8;
#endif
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
        /* XXX: VP9 compressed format isn't supported by V4L2
         * TODO Hardware VP8 support */
        if ( CONFIG_VP9(cfg) || CONFIG_VP8(cfg) || ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
                printf("%sHardware acceleration not supported\n"
                    "Fallback to raw YUYV, software encoding%s\n", CRED, CDFLT);
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_S_FMT, &fmt))
        }
        else
                hwcx_supported = 1;

        /* Frame rate settings */
        BZERO_STRUCT(setfps);
        setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_G_PARM, &setfps))
        if (!(setfps.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
                errno = ENOTSUP;
                fprintf(stderr, "%s does not support frame rate settings\n", cfg->device);
                return (-1);
        }
        BZERO_STRUCT(setfps)
        setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        setfps.parm.capture.timeperframe.numerator = 1;
        setfps.parm.capture.timeperframe.denominator = cfg->framerate;
        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_S_PARM, &setfps))
        assert(setfps.parm.capture.timeperframe.numerator == 1);
        if (cfg->framerate != setfps.parm.capture.timeperframe.denominator) {
                printf("%sDriver has changed the requested framerate%s\n", CRED, CDFLT);
                cfg->framerate = setfps.parm.capture.timeperframe.denominator;
        }

        /* Fetch device frame buffers */
        BZERO_STRUCT(reqb)
        reqb.count = NB_BUFFER;
        reqb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqb.memory = V4L2_MEMORY_MMAP;
        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_REQBUFS, &reqb))

        /* Map the buffers */
        for (int i = 0; i < NB_BUFFER; ++i)
        {
                BZERO_STRUCT(buf)
                buf.index = i;
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_QUERYBUF, &buf))
                membuf[i].size = buf.length;
                membuf[i].ptr = mmap(NULL, buf.length, PROT_READ|PROT_WRITE,
                    MAP_SHARED, fd, buf.m.offset);
                if (membuf[i].ptr == MAP_FAILED)
                        return (-1);
        }

        /* Queue the buffers */
        for (int i = 0; i < NB_BUFFER; ++i)
        {
                BZERO_STRUCT(buf)
                buf.index = i;
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_QBUF, &buf))
        }
        buf.index = 0;

        /* Start capturing */
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_STREAMON, &type))
        streamon = 1;

        return (0);
}

void Capture_stop(void)
{
        /* Stop capturing */
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (streamon) {
                PERR_ON_NEGATIVE(ioctl(fd, VIDIOC_STREAMOFF, &type))
                streamon = 0;
        }

        /* Unmap the buffers */
        for (int i = 0; i < NB_BUFFER; ++i) {
                if (membuf[i].ptr != NULL)
                        PERR_ON_NEGATIVE(munmap(membuf[i].ptr, membuf[i].size))
                membuf[i].ptr = NULL;
        }

        if (fd > 0) {
                PERR_ON_NEGATIVE(close(fd))
                fd = -1;
        }
}

static int64_t get_curtime()
{
        struct timespec ts;

        FAIL_ON_NEGATIVE(clock_gettime(CLOCK_REALTIME, &ts))
        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int Capture_get_frame(rtvs_frame_t *frame)
{
        FAIL_ON_NEGATIVE(select(fd + 1, &fds, NULL, NULL, NULL))
        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_DQBUF, &buf))

        if (hwcx_supported)
                frame->flags = HARD_ENCODED;
        frame->data = membuf[buf.index].ptr;
        frame->size = buf.bytesused;
        FAIL_ON_NEGATIVE(frame->pts = get_curtime())
        ++frame_num;

        if (old_pts) {
                printf("\rfps = %.2f, frames = %" PRIu64,
                    (float) 1000 / (frame->pts - old_pts), frame_num);
                fflush(stdout);
        }
        old_pts = frame->pts;

        FAIL_ON_NEGATIVE(ioctl(fd, VIDIOC_QBUF, &buf))
        return (0);
}
