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

#include "cs2_net.h"
#include "net.h"
#include "utils.h"
#include "z21.h"

struct z21_data_t z21_data;
unsigned char udpframe[MAXDG];

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -i <interface list>\n", prg);
    fprintf(stderr, "   Version 0.1\n\n");
    fprintf(stderr, "         -i <interface list> interface list\n");
}

int main(int argc, char **argv) {
    int opt;
    char *interface_list, *broadcast_ip;
    struct in_addr cs2_ip;

    while ((opt = getopt(argc, argv, "i:h?")) != -1) {
	switch (opt) {
	case 'i':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		interface_list = strndup(optarg, MAXLINE - 1);
	    } else {
		fprintf(stderr, "interface list to long\n");
		exit(EXIT_FAILURE);
	    }
	    break;
	case 'h':
	case '?':
	    print_usage(basename(argv[0]));
	    exit(EXIT_SUCCESS);
	    break;
	}
    }

    /* find the broadcast address */
    if (!(broadcast_ip = find_first_ip(interface_list, BROADCAST_IP)))
	exit(EXIT_FAILURE);

    printf("Using IP to send CAN Ping: %s\n", broadcast_ip);

    cs2_ip = find_cs2(broadcast_ip, 10);
    if (cs2_ip.s_addr)
	printf("Found CS2 IP: %s\n", inet_ntoa(cs2_ip));
    	
    free(interface_list);
    free(broadcast_ip);

    return EXIT_SUCCESS;
}
