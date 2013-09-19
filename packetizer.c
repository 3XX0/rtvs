#include <time.h>
#include <errno.h>
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
int Packetizer_packetize(rtvs_frame_t *frame)
{
        size_t size;

        Frame_init_partitions(frame);

        /* Convert the pts to a 90kHz resolution timestamp */
        packet.timestamp = frame->pts * 90;
        packet.pstart = 1;
        packet.pid = 0;

        /* Frame fits in one packet */
        if (frame->size <= RTP_PAYLOAD_SIZE) {
                packet.marker = 1;
                memcpy(packet.payload.data, frame->data, frame->size);
                if (Rtp_send(packet, frame->size) < 0)
                        return (-1);
                ++packet.seqnum;
        }
        /* Send partitions separately */
        else {
                if (frame->partition_num == 1)
                        goto too_big;
                packet.marker = 0;
                for (; packet.pid < frame->partition_num; ++packet.pid) {
                        size = frame->partition_size[packet.pid];
                        if (packet.pid == 0) /* First partition, don't forget the frame header */
                                size += (frame->data[0] & 0x01) ? FRAME_HEADER_SIZE
                                        : FRAME_HEADER_SIZE + KEYFRAME_HEADER_SIZE;
                        if (packet.pid == frame->partition_num - 1) /* Last partition */
                                packet.marker = 1;

                        if (size > RTP_PAYLOAD_SIZE)
                                goto too_big;
                        memcpy(packet.payload.data, frame->data, size);
                        frame->data += size;
                        frame->size -= size;
                        if (Rtp_send(packet, size) < 0)
                                return (-1);
                        ++packet.seqnum;
                }
                assert(frame->size == 0);
        }
        return (0);

too_big:
        errno = EMSGSIZE;
        return (-1);
}
