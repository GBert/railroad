/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <net/if.h>
#include <sys/time.h>
#include <time.h>

#include "cs2-config.h"
#include "read-cs2-config.h"

#define MAX_LINE		1024
#define MAX_BUFFER		4096
#define MAXLOLILOCOS	40		// max nmbr of locos in MS2
#define MAX(a,b)	((a) > (b) ? (a) : (b))

struct ms2_data_t {
    char *loco_file;
    char *config_data;
    char loco_name[32];
    int size;
    int sb;
    int sc;
    int loco_list_low;
    int loco_list_len;
    uint16_t hash;
    uint16_t crc;
    uint32_t mask;
    int verbose;
};

extern struct loco_data_t *loco_data;
uint16_t CRCCCITT(uint8_t * data, size_t length, uint16_t seed);

char loco_dir[MAX_LINE];

static char *F_CAN_FORMAT_STRG   = "      CAN->  CANID 0x%08X R [%d]";
static char *F_S_CAN_FORMAT_STRG = "short CAN->  CANID 0x%08X R [%d]";
static char *T_CAN_FORMAT_STRG   = "      CAN<-  CANID 0x%08X   [%d]";

static unsigned char M_GET_LOCO_LIST[] = "lokliste";
static unsigned char M_GET_LOCO_NAME[] = "loknamen";
static unsigned char M_GET_LOCO_INFO[] = "lokinfo";
static unsigned char M_GET_MS_CONFIG[] = "ms2-conf";

static char M_SIMU_GUI[] = { 0, 0, 0, 0, 3, 8, 0xFF, 0xFF };
static char M_SIMU_MAS[] = { 0x4D, 0x53, 0x32, 0x01 };		// MS serno 1

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -i <can interface>\n", prg);
    fprintf(stderr, "   Version 1.1\n\n");
    fprintf(stderr, "         -c <loco_dir>       set the locomotive file dir - default %s\n", loco_dir);
    fprintf(stderr, "         -i <can int>        can interface - default can0\n");
    fprintf(stderr, "         -d                  daemonize\n");
    fprintf(stderr, "         -g                  simulate GUI to force MS to CS slave mode\n");
    fprintf(stderr, "         -m                  simulate master MS2 to force MS2 slave mode\n");
    fprintf(stderr, "         -w                  TrainController work around F16-F31\n\n");
}

int time_stamp(char *timestamp) {
    struct timeval tv;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);

    sprintf(timestamp, "%02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec / 1000);
    return 0;
}

void toc32(uint8_t *data, uint32_t x) {
    data[0] = (x & 0xFF000000) >> 24;
    data[1] = (x & 0x00FF0000) >> 16;
    data[2] = (x & 0x0000FF00) >> 8;
    data[3] =  x & 0x000000FF;
}

void toc16(uint8_t *data, uint16_t x) {
    data[0] = (x & 0xFF00) >> 8;
    data[1] =  x & 0x00FF;
}

unsigned int read_pipe(FILE *input, char *d, size_t n) {
    unsigned int i = 0;
	int g;

    while (((g = fgetc(input)) != EOF) && (i < n)) {
		i++;
		*d++ = g;
		if (g == 0x0a) while (i % 8) { i++; *d++ = ' '; }	// fill spaces
	}

    memset(d, 0, (i % 8));
    return i;
}

void print_can_frame(char *format_string, struct can_frame *frame) {
    int i;
    char timestamp[16];
    time_stamp(timestamp);
    printf("%s ", timestamp);
    printf(format_string, frame->can_id & CAN_EFF_MASK, frame->can_dlc);
    for (i = 0; i < frame->can_dlc; i++) {
	printf(" %02x", frame->data[i]);
    }
    if (frame->can_dlc < 8) {
	for (i = frame->can_dlc; i < 8; i++) {
	    printf("   ");
	}
    }
    printf("  ");
    for (i = 0; i < frame->can_dlc; i++) {
	if (isprint(frame->data[i]))
	    printf("%c", frame->data[i]);
	else
	    putchar(46);
    }
    printf("\n");
}

int send_can_frame(int can_socket, struct can_frame *frame, int verbose) {
    frame->can_id &= CAN_EFF_MASK;
    frame->can_id |= CAN_EFF_FLAG;
    /* send CAN frame */
    if (write(can_socket, frame, sizeof(*frame)) != sizeof(*frame)) {
	fprintf(stderr, "error writing CAN frame: %s\n", strerror(errno));
	return -1;
    }
    if (verbose)
	print_can_frame(T_CAN_FORMAT_STRG, frame);
    return 0;
}

int send_config_data(struct ms2_data_t *ms2_data) {
    int i;
    struct can_frame frame;

    frame.can_id = 0x00420300 | ms2_data->hash;
    frame.can_dlc = 6;
    toc32(&frame.data[0], ms2_data->size);
    toc16(&frame.data[4], ms2_data->crc);
    send_can_frame(ms2_data->sc, &frame, ms2_data->verbose);

    frame.can_dlc = 8;
    for (i = 0; i < ms2_data->size; i += 8) {
	memcpy(frame.data, &ms2_data->config_data[i], 8);
	usleep(2000);
	send_can_frame(ms2_data->sc, &frame, ms2_data->verbose);
    }
    return (EXIT_SUCCESS);
}

int get_loco_list(struct ms2_data_t *ms2_data) {
    int fdp[2], nframes;
    FILE *input_fd, *output_fd;

    if (read_loco_data(ms2_data->loco_file, CONFIG_FILE))
	fprintf(stderr, "can't read loco_file %s\n", ms2_data->loco_file);

    if ((pipe(fdp) == -1)) {
	fprintf(stderr, "setup pipe error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    if (ms2_data->verbose) {
	printf("Loco list size: %d\n", HASH_COUNT(loco_data));
    }

    input_fd = fdopen(fdp[1], "w");
    output_fd = fdopen(fdp[0], "r");

	if (ms2_data->loco_list_low < 0) {
		int lle = show_loco_list(input_fd, MAXLOLILOCOS);
		if (ms2_data->verbose)
		printf("Locolist filled with %d entries\n", lle);
	}
    else show_loco_names(input_fd, ms2_data->loco_list_low, ms2_data->loco_list_len);
    fclose(input_fd);

    ms2_data->size = read_pipe(output_fd, ms2_data->config_data, MAX_BUFFER);

    nframes = (ms2_data->size + 7) / 8;
//	ms2_data->size = nframes * 8;
    ms2_data->crc = CRCCCITT((unsigned char *)ms2_data->config_data, nframes * 8, 0xffff);

    if (ms2_data->verbose)
	printf("Length %d CRC 0x%04X\n", ms2_data->size, ms2_data->crc);

    fclose(output_fd);
    return (EXIT_SUCCESS);
}

int get_loco_by_name(struct ms2_data_t *ms2_data) {
    int fdp[2], nframes;
    FILE *input_fd, *output_fd;

    if ((pipe(fdp) == -1)) {
	fprintf(stderr, "setup pipe error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    input_fd = fdopen(fdp[1], "w");
    output_fd = fdopen(fdp[0], "r");
    print_loco_by_name(input_fd, ms2_data->loco_name, MS2FKT | ms2_data->mask);
    fclose(input_fd);

    ms2_data->size = read_pipe(output_fd, ms2_data->config_data, MAX_BUFFER);

    nframes = (ms2_data->size + 7) / 8;
//	ms2_data->size = nframes * 8;
    ms2_data->crc = CRCCCITT((unsigned char *)ms2_data->config_data, nframes * 8, 0xffff);

    if (ms2_data->verbose)
	printf("Length %d CRC 0x%04X\n", ms2_data->size, ms2_data->crc);
    fclose(output_fd);
    return (EXIT_SUCCESS);
}

void get_msconfig_data(struct ms2_data_t *ms2_data) {

    ms2_data->size = sprintf(ms2_data->config_data, "[config]\n .magprot=0\n .lokprot=7\n");

    int nframes = (ms2_data->size + 7) / 8;
    ms2_data->crc = CRCCCITT((unsigned char *)ms2_data->config_data, nframes * 8, 0xffff);

    if (ms2_data->verbose)
	printf("Length %d CRC 0x%04X\n", ms2_data->size, ms2_data->crc);
}

int main(int argc, char **argv) {
    int max_fds, opt;
    struct can_frame frame;
    struct sockaddr_can caddr;
    struct ifreq ifr;
    socklen_t caddrlen = sizeof(caddr);
    fd_set read_fds;
    struct ms2_data_t ms2_data;
	enum {W_IDLE, W_NAME1, W_NAME2, W_NAMEV} waitstate = W_IDLE;

    int background = 0, simgui = 0, simmms = 0;
	uint16_t dev;
    strcpy(ifr.ifr_name, "can0");
    strcpy(loco_dir, "/www/config");

    memset(&ms2_data, 0, sizeof(ms2_data));

    ms2_data.config_data = calloc(MAX_BUFFER, 1);
    ms2_data.verbose = 1;

    while ((opt = getopt(argc, argv, "c:i:dgmhw?")) != -1) {
	switch (opt) {
	case 'c':
	    if (strnlen(optarg, MAX_LINE) < MAX_LINE) {
		strncpy(loco_dir, optarg, sizeof(loco_dir) - 1);
	    } else {
		fprintf(stderr, "config file dir too long\n");
		exit(EXIT_FAILURE);
	    }
	    break;
	case 'i':
	    strncpy(ifr.ifr_name, optarg, sizeof(ifr.ifr_name) - 1);
	    break;
	case 'd':
	    ms2_data.verbose = 0;
	    background = 1;
	    break;
	case 'g':
	    simgui = 1;
	    break;
	case 'm':
	    simmms = 1;
	    break;
	case 'w':
	    ms2_data.mask = TCWA;
	    break;
	case 'h':
	case '?':
	    print_usage(basename(argv[0]));
	    exit(EXIT_SUCCESS);
	    break;
	default:
	    fprintf(stderr, "Unknown option %c\n", opt);
	    print_usage(basename(argv[0]));
	    exit(EXIT_FAILURE);
	}
    }

    /* prepare reading lokomotive.cs */
    if (asprintf(&ms2_data.loco_file, "%s/%s", loco_dir, "lokomotive.cs2") < 0) {
	fprintf(stderr, "can't alloc buffer for loco_name: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    /* prepare CAN socket */
    memset(&caddr, 0, sizeof(caddr));
    if ((ms2_data.sc = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
	fprintf(stderr, "error creating CAN socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    caddr.can_family = AF_CAN;
    if (ioctl(ms2_data.sc, SIOCGIFINDEX, &ifr) < 0) {
	fprintf(stderr, "setup CAN socket error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    caddr.can_ifindex = ifr.ifr_ifindex;

    if (bind(ms2_data.sc, (struct sockaddr *)&caddr, caddrlen) < 0) {
	fprintf(stderr, "error binding CAN socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    /* daemonize the process if requested */
    if (background) {
	if (daemon(0, 0) < 0) {
	    fprintf(stderr, "Going into background failed: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
    }

    FD_ZERO(&read_fds);
    FD_SET(ms2_data.sc, &read_fds);
    max_fds = ms2_data.sc;

    while (1) {
	if (select(max_fds + 1, &read_fds, NULL, NULL, NULL) < 0) {
	    fprintf(stderr, "select error: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	/* received a CAN frame */
	if (FD_ISSET(ms2_data.sc, &read_fds)) {
	    if (read(ms2_data.sc, &frame, sizeof(struct can_frame)) < 0) {
		fprintf(stderr, "error reading CAN frame: %s\n", strerror(errno));
	    } else if (frame.can_id & CAN_EFF_FLAG) {	/* only EFF frames are valid */
		if (ms2_data.verbose) {
		    print_can_frame(F_CAN_FORMAT_STRG, &frame);
		}

		switch ((frame.can_id & 0x00FF0000UL) >> 16) {
		case 0x31:  // Ping response
			dev = frame.data[6] << 8 | frame.data[7];
			frame.can_id = 0x00314711;
			if ((dev & 0xFFF0) == 0x0030) {			// MS2 active
				if (simgui) {
			    memcpy(frame.data, M_SIMU_GUI, 8);
			    send_can_frame(ms2_data.sc, &frame, ms2_data.verbose);
				}
				else if (simmms) {
			    memcpy(frame.data, M_SIMU_MAS, 4);
			    send_can_frame(ms2_data.sc, &frame, ms2_data.verbose);
				}
			}
			break;
		case 0x40:
			if (frame.can_dlc != 8) continue;
			if ((frame.can_id & 0xFFFF) != ms2_data.hash) waitstate = W_IDLE;
			ms2_data.hash = frame.can_id & 0XFFFF;	// must be from same source

			switch (waitstate) {
			case W_NAME1:
			    memcpy(ms2_data.loco_name, frame.data, 8);
				waitstate = W_NAME2;
				break;
			case W_NAME2:
			    memcpy(&ms2_data.loco_name[8], frame.data, 8);
			    if (ms2_data.verbose)
					printf("sending lokinfo >%s<\n", ms2_data.loco_name);
			    frame.can_id = 0x00410300;
			    send_can_frame(ms2_data.sc, &frame, ms2_data.verbose);
			    get_loco_by_name(&ms2_data);
			    send_config_data(&ms2_data);
				waitstate = W_IDLE;
				break;
			case W_NAMEV:
				sscanf((char *)frame.data, "%d %d", &ms2_data.loco_list_low, &ms2_data.loco_list_len);
				if (ms2_data.verbose)
					printf("requesting loco names from %d len %d\n", ms2_data.loco_list_low, ms2_data.loco_list_len);
				frame.can_id = 0x00410300;
				send_can_frame(ms2_data.sc, &frame, ms2_data.verbose);
				get_loco_list(&ms2_data);
				send_config_data(&ms2_data);
				waitstate = W_IDLE;
				break;
			default:
				if (memcmp(&frame.data[0], M_GET_LOCO_LIST, 8) == 0) {
					if (ms2_data.verbose)
						printf("requesting loco name list\n");
					frame.can_id = 0x00410300;
					send_can_frame(ms2_data.sc, &frame, ms2_data.verbose);
					ms2_data.loco_list_low = -1;
					get_loco_list(&ms2_data);
					send_config_data(&ms2_data);
				}
				else if (memcmp(&frame.data[0], M_GET_LOCO_NAME, 8) == 0) {
					waitstate = W_NAMEV;
				}
				else if (memcmp(&frame.data[0], M_GET_LOCO_INFO, 7) == 0) {
					waitstate = W_NAME1;
					memset(ms2_data.loco_name, 0, sizeof(ms2_data.loco_name));
				}
				else if (memcmp(&frame.data[0], M_GET_MS_CONFIG, 8) == 0) {
					if (ms2_data.verbose)
						printf("requesting MS2 config\n");
					frame.can_id = 0x00410300;
					send_can_frame(ms2_data.sc, &frame, ms2_data.verbose);
					get_msconfig_data(&ms2_data);
					send_config_data(&ms2_data);
				}
		    }
		    break;
		}
	    } else
		print_can_frame(F_S_CAN_FORMAT_STRG, &frame);
	}
    }
    close(ms2_data.sc);
    return 0;
}
