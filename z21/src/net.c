/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "utils.h"
#include "net.h"

int setup_udp_socket(struct sockaddr_in *addr, char *dst_address, int port, int type) {
    int on, s;

    on = 1;
    s = 0;
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    if (type == UDP_SENDING) {
	s = inet_pton(AF_INET, dst_address, &(addr->sin_addr));
	if (s == 0)
	    fprintf(stderr, "IP invalid: %s\n", dst_address);
	if (s < 0)
	    fprintf(stderr, "invalid address family\n");

	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0)
	    fprintf(stderr, "error creating UDP sending socket: %s\n", strerror(errno));
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
	    fprintf(stderr, "error setup UDP broadcast option: %s\n", strerror(errno));
    }

    if (type == UDP_READING) {
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
	    fprintf(stderr, "creating UDP reading socket error: %s\n", strerror(errno));
	}
	if (bind(s, (struct sockaddr *)addr, sizeof *addr) < 0) {
	    fprintf(stderr, "binding UDP reading socket error: %s\n", strerror(errno));
	}
    }
    return s;
}
