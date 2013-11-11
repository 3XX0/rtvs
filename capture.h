#pragma once

#include <time.h>

#ifdef WITH_MMAL_CAPTURE
#include <interface/mmal/mmal_types.h>
#include <interface/mmal/mmal_common.h>
#include <interface/mmal/mmal_parameters_camera.h>
#endif

#ifdef WITH_V4L2_CAPTURE
#define Capture_perror(x) perror(x)
#endif

static inline int64_t get_curtime()
{
        struct timespec ts;

        if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
            return (-1);
        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* Exposure mode
 *
 * MMAL_PARAM_EXPOSUREMODE_OFF,
 * MMAL_PARAM_EXPOSUREMODE_AUTO,
 * MMAL_PARAM_EXPOSUREMODE_NIGHT,
 * MMAL_PARAM_EXPOSUREMODE_NIGHTPREVIEW,
 * MMAL_PARAM_EXPOSUREMODE_BACKLIGHT,
 * MMAL_PARAM_EXPOSUREMODE_SPOTLIGHT,
 * MMAL_PARAM_EXPOSUREMODE_SPORTS,
 * MMAL_PARAM_EXPOSUREMODE_SNOW,
 * MMAL_PARAM_EXPOSUREMODE_BEACH,
 * MMAL_PARAM_EXPOSUREMODE_VERYLONG,
 * MMAL_PARAM_EXPOSUREMODE_FIXEDFPS,
 * MMAL_PARAM_EXPOSUREMODE_ANTISHAKE,
 * MMAL_PARAM_EXPOSUREMODE_FIREWORKS,
 */

/* Exposure meter mode
 *
 * MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE,
 * MMAL_PARAM_EXPOSUREMETERINGMODE_SPOT,
 * MMAL_PARAM_EXPOSUREMETERINGMODE_BACKLIT,
 * MMAL_PARAM_EXPOSUREMETERINGMODE_MATRIX
 */

/* AWB mode
 *
 * MMAL_PARAM_AWBMODE_OFF,
 * MMAL_PARAM_AWBMODE_AUTO,
 * MMAL_PARAM_AWBMODE_SUNLIGHT,
 * MMAL_PARAM_AWBMODE_CLOUDY,
 * MMAL_PARAM_AWBMODE_SHADE,
 * MMAL_PARAM_AWBMODE_TUNGSTEN,
 * MMAL_PARAM_AWBMODE_FLUORESCENT,
 * MMAL_PARAM_AWBMODE_INCANDESCENT,
 * MMAL_PARAM_AWBMODE_FLASH,
 * MMAL_PARAM_AWBMODE_HORIZON,
 */

/* Image effect
 *
 * MMAL_PARAM_IMAGEFX_NONE,
 * MMAL_PARAM_IMAGEFX_NEGATIVE,
 * MMAL_PARAM_IMAGEFX_SOLARIZE,
 * MMAL_PARAM_IMAGEFX_POSTERIZE,
 * MMAL_PARAM_IMAGEFX_WHITEBOARD,
 * MMAL_PARAM_IMAGEFX_BLACKBOARD,
 * MMAL_PARAM_IMAGEFX_SKETCH,
 * MMAL_PARAM_IMAGEFX_DENOISE,
 * MMAL_PARAM_IMAGEFX_EMBOSS,
 * MMAL_PARAM_IMAGEFX_OILPAINT,
 * MMAL_PARAM_IMAGEFX_HATCH,
 * MMAL_PARAM_IMAGEFX_GPEN,
 * MMAL_PARAM_IMAGEFX_PASTEL,
 * MMAL_PARAM_IMAGEFX_WATERCOLOUR,
 * MMAL_PARAM_IMAGEFX_FILM,
 * MMAL_PARAM_IMAGEFX_BLUR,
 * MMAL_PARAM_IMAGEFX_SATURATION,
 * MMAL_PARAM_IMAGEFX_COLOURSWAP,
 * MMAL_PARAM_IMAGEFX_WASHEDOUT,
 * MMAL_PARAM_IMAGEFX_POSTERISE,
 * MMAL_PARAM_IMAGEFX_COLOURPOINT,
 * MMAL_PARAM_IMAGEFX_COLOURBALANCE,
 * MMAL_PARAM_IMAGEFX_CARTOON,
 */

typedef struct
{
#ifdef WITH_MMAL_CAPTURE
   int                                 rotation;       /* Range 0, 359 */
   int                                 saturation;     /* Range -100, 100 */
   int                                 sharpness;      /* Range -100, 100 */
   int                                 contrast;       /* Range -100, 100 */
   int                                 brightness;     /* Range 0, 100 */
   int                                 iso;            /* Light sensitivity */
   int                                 stabilize;
   int                                 hflip;
   int                                 vflip;
   int                                 shutter_speed;  /* Exposure time (0 for auto) */
   int                                 exposure_comp;  /* Range -10, 10 */
   MMAL_PARAM_EXPOSUREMODE_T           exposure_mode;
   MMAL_PARAM_EXPOSUREMETERINGMODE_T   exposure_meter;
   MMAL_PARAM_AWBMODE_T                awb_mode;       /* Auto white balance mode */
   MMAL_PARAM_IMAGEFX_T                image_fx;
#endif
} rtvs_capture_t;
