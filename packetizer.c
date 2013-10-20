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
        /* XXX: Not really unpredictable */
        srand(time(NULL));
        BZERO_STRUCT(packet);

        packet.version     = 2;
        packet.payloadtype = 98;
        packet.seqnum      = rand();
        packet.ssrc        = rand();
}

#define IS_KEYFRAME(x) (!(x->data[0] & 0x01))

int Packetizer_packetize(rtvs_frame_t *frame)
{
        size_t size;

        Frame_init_partitions(frame);

        /* Convert the pts to a 90kHz resolution timestamp */
        packet.timestamp = frame->pts * 90;
        packet.marker = 0;
        packet.pid = 0;
        packet.nonref = 0;

        /* Frame fits in one packet */
        if (frame->size <= RTP_PAYLOAD_SIZE) {
                packet.pstart = 1;
                packet.marker = 1;
                memcpy(packet.payload.data, frame->data, frame->size);
                FAIL_ON_NEGATIVE(Rtp_send(packet, frame->size))
                ++packet.seqnum;
        }
        /* Send partitions separately */
        else {
                for (; packet.pid < frame->partition_num; ++packet.pid) {
                        packet.pstart = 1;
                        size = frame->partition_size[packet.pid];
                        if (packet.pid == 0) /* First partition, don't forget the frame header */
                                size += IS_KEYFRAME(frame) ? FRAME_HEADER_SIZE + KEYFRAME_HEADER_SIZE
                                                           : FRAME_HEADER_SIZE;
                        /* Still too big for our packet, split it in chunks of size RTP_PAYLOAD_SIZE */
                        while (size > RTP_PAYLOAD_SIZE) {
                                memcpy(packet.payload.data, frame->data, RTP_PAYLOAD_SIZE);
                                frame->data += RTP_PAYLOAD_SIZE;
                                frame->size -= RTP_PAYLOAD_SIZE;
                                FAIL_ON_NEGATIVE(Rtp_send(packet, RTP_PAYLOAD_SIZE))
                                size -= RTP_PAYLOAD_SIZE;
                                ++packet.seqnum;
                                packet.pstart = 0;
                        }
                        if (packet.pid == frame->partition_num - 1) /* Last partition, last packet */
                                packet.marker = 1;
                        memcpy(packet.payload.data, frame->data, size);
                        frame->data += size;
                        frame->size -= size;
                        FAIL_ON_NEGATIVE(Rtp_send(packet, size))
                        ++packet.seqnum;
                }
                assert(frame->size == 0);
        }
        return (0);
}
