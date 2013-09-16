#include <time.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "rtvs.h"

static rtvs_packet_t packet;

void Packetizer_init()
{
        srand(time(NULL));
        BZERO_STRUCT(packet);

        packet.version     = 2;
        packet.payloadtype = 100;
        packet.seqnum      = rand();
        packet.ssrc        = rand();
}

/* TODO Handle non reference frames (0 by default) */
void Packetizer_packetize(rtvs_frame_t *frame)
{
        printf("frame size = %zu\n", frame->size);

        Frame_init_partitions(frame);

        /* Convert the pts to a 90kHz resolution timestamp */
        packet.timestamp = frame->pts * 90;

        /* Header + 1st partition */
        packet.marker = 0;
        packet.pstart = 1;
        packet.pid = 0;
        //Rtp_register(&packet);
        ++packet.seqnum;

        for (unsigned int i = 0; i < frame->partition_num; ++i)
                printf("part%u size = %zu\n", i, frame->partition_size[i]);
}
