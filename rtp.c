#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "rtvs.h"

static int                sock = -1;
static struct sockaddr_in saddr;

int Rtp_start(char *addr)
{
        uint16_t port;
        char     *p;
        char     loop = 0;

        p = strchr(addr, ':');
        *p = '\0';
        port = atoi(p + 1);

        FAIL_ON_NEGATIVE(sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))
        FAIL_ON_NEGATIVE(fcntl(sock, F_SETFL, O_NONBLOCK))
        FAIL_ON_NEGATIVE(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(char)))
        BZERO_STRUCT(saddr)
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        if (*addr && inet_pton(AF_INET, addr, &saddr.sin_addr.s_addr) != 1)
                return (-1);
        else if (*addr == '\0')
                saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        FAIL_ON_NEGATIVE(connect(sock, (const struct sockaddr *) &saddr, sizeof(saddr)))
        return (0);
}

void Rtp_stop(void)
{
        if (sock > 0) {
                PERR_ON_NEGATIVE(close(sock))
                sock = -1;
        }
}

int Rtp_send(rtvs_packet_t packet, size_t payload_size)
{
        /* Format endianness sensitive fields */
        packet.seqnum = htons(packet.seqnum);
        packet.timestamp = htonl(packet.timestamp);
        packet.ssrc = htonl(packet.ssrc);
        /* It's ok to ignore ECONNREFUSED here, nobody is currently listening */
        if (sendto(sock, &packet, RTP_HEADER_SIZE + payload_size, 0,
                  (struct sockaddr*) &saddr, sizeof(saddr)) < 0 && errno != ECONNREFUSED)
                return (-1);
        return (0);
}
