#pragma once

#include <stdio.h>

#include "codec.h"
#include "config.h"
#include "frame.h"
#include "bed.h"
#include "rtp.h"

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

#define PERR_ON_NEGATIVE(x) if ((x) < 0) { perror(__FUNCTION__); };

#define BZERO_STRUCT(x) memset(&(x), 0, sizeof(x));
#define BZERO_ARRAY(x) memset((x), 0, sizeof(x));

/* Modules definitions */

extern int Capture_start(rtvs_config_t *cfg);
extern void Capture_stop(void);
extern int Capture_get_frame(rtvs_frame_t *frame);

extern void Encoder_perror(const char *err);
extern int Encoder_start(rtvs_config_t *cfg);
extern void Encoder_stop(void);
extern int Encoder_encode_frame(const rtvs_config_t *cfg, rtvs_frame_t *frames);

extern int Muxing_open_file(const char *outfile);
extern void Muxing_close_file(void);
extern void Muxing_ivf_write_header(const rtvs_config_t *cfg, size_t frame_num);
extern void Muxing_ivf_write_frame(const rtvs_frame_t *frame);

extern void Packetizer_init(void);
extern int Packetizer_packetize(rtvs_frame_t *frame);

extern void Frame_init_partitions(rtvs_frame_t *frame);

extern void Bed_init(rtvs_bed_t *bed, const unsigned char *data, size_t size);
extern int Bed_get_bit(rtvs_bed_t *bed);
extern int Bed_get_uint(rtvs_bed_t *bed, int bits);
extern int Bed_get_int(rtvs_bed_t *bed, int bits);
extern int Bed_maybe_get_int(rtvs_bed_t *bed, int bits);

extern int Rtp_start(char *addr);
extern void Rtp_stop(void);
extern int Rtp_send(rtvs_packet_t packet, size_t size);
