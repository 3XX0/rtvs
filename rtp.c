#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "rtvs.h"

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

int Rtp_send(rtvs_packet_t packet, size_t size)
{
        /* Format endianness sensitive fields */
        packet.seqnum = htons(packet.seqnum);
        packet.timestamp = htonl(packet.timestamp);
        packet.ssrc = htonl(packet.ssrc);
        FAIL_ON_NEGATIVE(sendto(sock, &packet, size, 0, (struct sockaddr*) &saddr, sizeof(saddr)))
        return (0);
}
