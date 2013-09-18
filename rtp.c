#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "rtvs.h"

#define MAX_PACKETS_PER_FRAME (MAX_SIMULT_FRAMES * 4)

static struct
{
        rtvs_packet_t packet;
        size_t        size;
}                         packets_pool[MAX_PACKETS_PER_FRAME];
static int                pcount = 0;
static int                sock;
static struct sockaddr_in saddr;

int Rtp_start(char *addr)
{
        uint16_t port;
        char     *p = strchr(addr, ':');

        *p = '\0';
        port = atoi(p + 1); /* TODO fix ugliness */

        FAIL_ON_NEGATIVE(sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))
        BZERO_STRUCT(saddr)
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        if (*addr && inet_pton(AF_INET, addr, &saddr.sin_addr.s_addr) != 1)
                return (-1);
        else if (*addr == '\0')
                saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        return (0);
}

int Rtp_stop(void)
{
        FAIL_ON_NEGATIVE(close(sock))
        return (0);
}

void Rtp_register(const rtvs_packet_t *packet, size_t size)
{
        assert(pcount < MAX_PACKETS_PER_FRAME);
        memcpy(&packets_pool[pcount].packet, packet, size + RTP_HEADER_SIZE);
        packets_pool[pcount].size = size + RTP_HEADER_SIZE;
        /* Format endianness sensitive fields */
        packets_pool[pcount].packet.seqnum = htons(packets_pool[pcount].packet.seqnum);
        packets_pool[pcount].packet.timestamp = htonl(packets_pool[pcount].packet.timestamp);
        packets_pool[pcount].packet.ssrc = htonl(packets_pool[pcount].packet.ssrc);
        ++pcount;
}

int Rtp_send_frame(void)
{
        struct iovec  iov[MAX_PACKETS_PER_FRAME];
        struct msghdr msg;

        for (int i = 0; i < pcount; ++i) {
                iov[i].iov_base = &packets_pool[i].packet;
                iov[i].iov_len = packets_pool[i].size;
        }
        msg.msg_name = &saddr;
        msg.msg_namelen = sizeof(saddr);
        msg.msg_iov = iov;
        msg.msg_iovlen = pcount;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;
        pcount = 0;

        FAIL_ON_NEGATIVE(sendmsg(sock, &msg, 0))
        return (0);
}
