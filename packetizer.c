#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "rtvs.h"

static rtvs_packet_t packet;

void Packetizer_init(void)
{
        srand(time(NULL));
        BZERO_STRUCT(packet);

        packet.version     = 2;
        packet.payloadtype = 98;
        packet.seqnum      = rand();
        packet.ssrc        = rand();
}

/* TODO Handle non reference frames (0 by default) */
void Packetizer_packetize(rtvs_frame_t *frame)
{
        Frame_init_partitions(frame);

        /* Convert the pts to a 90kHz resolution timestamp */
        packet.timestamp = frame->pts * 90;
        packet.pstart = 1;
        packet.pid = 0;
        if (frame->size <= RTP_PAYLOAD_SIZE) { /* Frame fits in one packet */
                packet.marker = 1;
                memcpy(packet.payload.data, frame->data, frame->size);
                Rtp_register(&packet, frame->size);
                ++packet.seqnum;
        }
        else { /* Send partitions separately */
                assert(frame->partition_num > 1);
                packet.marker = 0;
                for (unsigned int i = 0; i < frame->partition_num; ++i) {
                        if (i == frame->partition_num - 1)
                                packet.marker = 1;
                        assert(frame->partition_size[i] <= RTP_PAYLOAD_SIZE);
                        memcpy(packet.payload.data, frame->data, frame->partition_size[i]);
                        Rtp_register(&packet, frame->partition_size[i]);
                        ++packet.pid;
                        ++packet.seqnum;
                }
        }
}
