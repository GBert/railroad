/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "net.h"
#include "utils.h"
#include "z21.h"

#define MAXLINE       		256
#define MAERKLIN_RECV_PORT	15730
#define MAERKLIN_SEND_PORT	15731

extern unsigned char udpframe[MAXDG];

static unsigned char M_CAN_PING[] = { 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xEE, 0xEE };

struct in_addr find_cs2(char *broadcast_ip, int retries) {
    int n, attemps, sb, sr, nready;
    struct sockaddr_in baddr, raddr;
    struct sockaddr_in responder;
    socklen_t len;
    fd_set readfds;
    struct in_addr cs2_ip;
    struct timespec ts;

    attemps = 0;
    memset(&baddr, 0, sizeof baddr);
    memset(&raddr, 0, sizeof raddr);
    memset(&raddr, 0, sizeof responder);
    memset(&cs2_ip, 0, sizeof cs2_ip);

    sb = setup_udp_socket(&baddr, broadcast_ip, MAERKLIN_SEND_PORT, UDP_SENDING);
    if (sb <= 0) {
	fprintf(stderr, "problem to setup sending socket\n");
	return (cs2_ip);
    }

    sr = setup_udp_socket(&raddr, NULL, MAERKLIN_RECV_PORT, UDP_READING);
    if (sr <= 0) {
	fprintf(stderr, "problem to setup receiving socket\n");
	return (cs2_ip);
    }

    if (sendto(sb, M_CAN_PING, sizeof M_CAN_PING, 0, (struct sockaddr *)&baddr, sizeof baddr) != sizeof M_CAN_PING) {
	fprintf(stderr, "failure to send CAN Ping: %s\n", strerror(errno));
	return (cs2_ip);
    }

    /* set select timeout -> send periodic (reversed) CAN Ping */
    ts.tv_sec = 1;
    ts.tv_nsec = 0;

    FD_ZERO(&readfds);
    FD_SET(sr, &readfds);

    while (1) {
	nready = pselect(sr + 1, &readfds, NULL, NULL, &ts, NULL);
	if (nready < 0) {
	    fprintf(stderr, "select error: %s\n", strerror(errno));
	    return (cs2_ip);
	};

	if (nready == 0) {
	    ts.tv_sec = 1;
	    ts.tv_nsec = 0;
	    if (sendto(sb, M_CAN_PING, sizeof M_CAN_PING, 0, (struct sockaddr *)&baddr, sizeof baddr) != sizeof M_CAN_PING) {
		fprintf(stderr, "failure to send CAN Ping: %s\n", strerror(errno));
		return (cs2_ip);
	    }
	    if (++attemps == retries) {
		fprintf(stderr, "failed to find CS2 IP after %d retries\n", attemps);
		return (cs2_ip);
	    }
	}

	if (FD_ISSET(sr, &readfds)) {
	    len = sizeof responder;
	    n = recvfrom(sr, udpframe, sizeof udpframe, 0, &responder, &len);
	    // printf("received UDP packet len %d from %s\n", n, inet_ntop(AF_INET, &responder.sin_addr, buffer, sizeof buffer));
	    if (n == 13) {
		/* CAN Ping and response are reversed */
		if ((be16(udpframe) & 0x0030) != 0x0030)
		    continue;

		if ((be16(&udpframe[11]) == 0xFFFF) || (be16(&udpframe[11]) == 0xEEEE)) {
		    return responder.sin_addr;
		}
	    }
	}
    }
    return cs2_ip;
}
