/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/*
 * Z21 Emulation for Roco WiFi Mouse
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <linux/can.h>

#include "utils.h"
#include "z21.h"

#define MAX(a,b)		((a) > (b) ? (a) : (b))
#define MAXIPLEN		40	/* maximum IP string length */
#define PRIMARY_UDP_PORT	21105
#define SECONDARY_UDP_PORT	21106
#define MAERKLIN_PORT		15731
#define MAXSIZE			16384

uint16_t CRCCCITT(unsigned char *data, size_t length, unsigned short seed);

extern struct z21_data_t z21_data;
char rfc3986[256] = { 0 };

uint16_t le16(uint8_t * u) {
    return (u[1] << 8) | u[0];
}

uint16_t be16(uint8_t * u) {
    return (u[0] << 8) | u[1];
}

uint32_t be32(uint8_t * u) {
    return (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
}

uint32_t le32(uint8_t * u) {
    return (u[3] << 24) | (u[2] << 16) | (u[1] << 8) | u[0];
}

void to_le16(uint8_t * u, uint16_t n) {
    u[0] = n & 0xff;
    u[1] = n >> 8;
}

void url_encoder_rfc_tables_init(void) {
    int i;

    for (i = 0; i < 256; i++) {
	rfc3986[i] = isalnum(i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
    }
}

char *url_encode(unsigned char *s, char *enc) {

    char *table = rfc3986;

    for (; *s; s++) {
	if (table[*s])
	    *enc = table[*s];
	else
	    sprintf(enc, "%%%02X", *s);
	while (*++enc) ;
    }
    return (enc);
}

void usec_sleep(int usec) {
    struct timespec to_wait;

    if (usec > 999999)
	usec = 999999;
    to_wait.tv_sec = 0;
    to_wait.tv_nsec = usec * 1000;
    nanosleep(&to_wait, NULL);
}

int time_stamp(char *timestamp) {
    struct timeval tv;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);

    sprintf(timestamp, "%02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec / 1000);
    return 0;
}

uint8_t xor(unsigned char *data, int length) {
    uint8_t res;
    int i;

    res = 0;
    for (i = 0; i < length; i++)
	res ^= data[i];
    return (res);
}

uint16_t loco_address_mapping(uint16_t uid) {
    /* dcc */
    if (uid >= 0xc000)
	return (uid - 0xc000 + 5000);
    /* mfx */
    if (uid >= 0x4000)
	return (uid - 0x4000 + 1000);
    return (uid);
}

uint16_t loco_address_demapping(uint16_t z21app_address) {
    /* dcc */
    if (z21app_address >= 5000)
	return (z21app_address - 5000 + 0xc000);
    /* mfx */
    if (z21app_address >= 1000)
	return (z21app_address - 1000 + 0x4000);
    return (z21app_address);
}

void print_udp_frame(char *format, int udplength, unsigned char *udpframe) {
    int i;
    uint16_t length, header;
    char timestamp[16];

    time_stamp(timestamp);
    printf("%s ", timestamp);
    if (z21_data.foreground) {
	/* print timestamp */

	if (udplength >= 4) {
	    length = le16(&udpframe[0]);
	    header = le16(&udpframe[2]);
	    printf(format, length, header);
	    for (i = 4; i < udplength; i++)
		printf(" %02x", udpframe[i]);
	    /* printf("\n"); */
	    for (; i < 16; i++)
		printf("   ");
	}
    }
}

void print_net_frame(char *format, unsigned char *netframe, int verbose) {
    uint32_t canid;
    int i, dlc;
    char timestamp[16];

    if (!verbose)
	return;

    canid = be32(netframe);
    dlc = netframe[4];
    time_stamp(timestamp);
    printf("%s ", timestamp);
    printf(format, canid & CAN_EFF_MASK, netframe[4]);
    for (i = 5; i < 5 + dlc; i++) {
	printf(" %02x", netframe[i]);
    }
    if (dlc < 8) {
	printf("(%02x", netframe[i]);
	for (i = 6 + dlc; i < 13; i++) {
	    printf(" %02x", netframe[i]);
	}
	printf(")");
    } else {
	printf(" ");
    }
    printf("  ");
    for (i = 5; i < 13; i++) {
	if (isprint(netframe[i]))
	    printf("%c", netframe[i]);
	else
	    putchar(46);
    }
    printf(" ");
}

char *search_interface_ip(char *search, int type) {
    struct sockaddr_in *bsa;
    struct ifaddrs *ifap, *ifa;
    char *s;

    /* trying to get the broadcast address */
    if (getifaddrs(&ifap) == -1) {
	perror("getifaddrs");
	exit(EXIT_FAILURE);
    }
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	if (ifa->ifa_addr == NULL)
	    continue;
	if (ifa->ifa_addr->sa_family == AF_INET) {
	    if (type == BROADCAST_IP)
		bsa = (struct sockaddr_in *)ifa->ifa_broadaddr;
	    else
		bsa = (struct sockaddr_in *)ifa->ifa_addr;
	    if (strncmp(ifa->ifa_name, search, strlen(search)) == 0) {
		s = strdup(inet_ntoa(bsa->sin_addr));
		freeifaddrs(ifap);
		return s;
	    }
	}
    }
    freeifaddrs(ifap);
    return NULL;
}

char *find_first_ip(char *list, int type) {
    char *ip_address = NULL;
    char *searchif;
    char *tempsp, *temp;

    tempsp = strdup(list);
    temp = tempsp;
    while ((searchif = strsep(&tempsp, ","))) {
	ip_address = search_interface_ip(searchif, type);
	if (ip_address) {
	    free(temp);
	    return ip_address;
	}
    }
    free(temp);
    return NULL;
}
