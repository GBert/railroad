/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/* Contributions by Rainer Müller */
/* MäCAN frame info added by github.com/Ixam97 */

#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <pcap.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#define  __FAVOR_BSD
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "../lib.h"
#include "decoder-can-cdb.h"
#include "decoder-can-cs1.h"
#include "decoder-can-cs2.h"
#include "decoder-can-maecan.h"
#include "decoder-z21.h"
#include "tools.h"
#include "can-monitor.h"

#define MAXDG   	4096	/* maximum datagram size */
#define MAXUDP  	16	/* maximum datagram size */
#define MAX(a,b)	((a) > (b) ? (a) : (b))

#define IPHDR_LEN       (20)
/* defines for the packet type code in an ETHERNET header */
#define ETHER_TYPE_IP	 (0x0800)
#define ETHER_TYPE_8021Q (0x8100)

unsigned char netframe[MAXDG];

struct cs2_config_data_t config_data;
extern struct knoten *messwert_knoten;

unsigned char buffer[MAX_PAKETE * 8];
int verbose = 0, expconf = 0;

static char *F_N_CAN_FORMAT_STRG = "  CAN  0x%08X  [%d]";
static char *F_N_UDP_FORMAT_STRG = "  UDP  0x%08X  [%d]";
static char *F_N_TCP_FORMAT_STRG = "  TCP  0x%08X  [%d]";
static char *F_N_SFF_FORMAT_STRG = "  CAN  <S>  0x%03X  [%d]";

void INThandler(int sig) {
    signal(sig, SIG_IGN);
    fputs(RESET, stdout);
    exit(0);
}

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -i <can|net interface>\n", prg);
    fprintf(stderr, "   Version 5.10\n\n");
    fprintf(stderr, "         -i <can|net int>  CAN or network interface - default can0\n");
    fprintf(stderr, "         -r <pcap file>    read PCAP file instead from CAN socket\n");
    fprintf(stderr, "         -s                select only network internal frames\n");
    fprintf(stderr, "         -l <candump file> read candump file instead from CAN socket\n");
    fprintf(stderr, "         -t <rocrail file> read Rocrail file instead from CAN socket\n");
    fprintf(stderr, "         -d                dump to candump file\n\n");
    fprintf(stderr, "         -v                verbose output for TCP/UDP and errorframes\n\n");
    fprintf(stderr, "         -x                expose config data\n\n");
    fprintf(stderr, "         -h                show this help\n\n");
}

struct timeval time_stamp(char *timestamp) {
    struct timeval tv;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);

    sprintf(timestamp, "%02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec / 1000);
    return tv;
}

void frame_to_can(unsigned char *netframe, struct can_frame *frame) {
    frame->can_id = be32(netframe);
    frame->can_dlc = netframe[4];
    memcpy(&frame->data, &netframe[5], 8);
}

void canframe_to_can(unsigned char *netframe, struct can_frame *frame) {
    frame->can_id = le32(netframe);
    frame->can_dlc = netframe[4];
    memcpy(&frame->data, &netframe[8], 8);
}

void ascii_to_can(char *s, struct can_frame *frame) {
    int i;
    unsigned char d[13];

    for (i = 0; i < 13; i++) {
	sscanf(&s[i * 3], "%hhx", &d[i]);
    }
    frame_to_can(d, frame);
}

void slcan_to_can(char *s, struct can_frame *frame) {
    int i;
    unsigned int dat;

    sscanf(s, "T%8X%1X", &frame->can_id, (unsigned int *)&frame->can_dlc);
    memset(&frame->data, 0, 8);
    for (i = 1; i <= frame->can_dlc; i++) {
	sscanf(&s[8 + i * 2], "%2X", &dat);
	frame->data[i - 1] = dat;
    }
}

void candump_to_can(char *s, struct can_frame *frame) {
    unsigned int i, dat;
    char *candata;

    sscanf(s, "%08x", &frame->can_id);
    candata = strstr(s, "#");
    i = (unsigned int)(candata - s);
    if ((i > 5) && ((frame->can_id & CAN_ERR_FLAG) == 0))
	frame->can_id |= CAN_EFF_FLAG;
    if (candata++ == NULL)
	return;
    if (candata[0] == 'R') {
	frame->can_id |= CAN_RTR_FLAG;
	frame->can_dlc = candata[1] & 0xF;
    } else {
	for (i = 0; i < 8; i++) {
	    if (sscanf(&candata[i * 2], "%2X", &dat) < 1)
		break;
	    frame->data[i] = dat;
	}
	frame->can_dlc = i;
    }
}

int print_can_frame(char *format_string, struct can_frame *frame) {
    int i;
    if (frame->can_dlc > 8) {
	printf(RED " Invalid DLC %d found\n" RESET, frame->can_dlc);
	return -1;
    }
    printf(format_string, frame->can_id & CAN_EFF_MASK, frame->can_dlc);
    if (frame->can_id & CAN_RTR_FLAG) {
	printf(" <RTR>                   ");
	return -2;
    }
    for (i = 0; i < frame->can_dlc; i++) {
	printf(" %02X", frame->data[i]);
    }
    if (frame->can_dlc < 8) {
	for (i = frame->can_dlc; i < 8; i++) {
	    printf("   ");
	}
    }
    printf(" ");
#if 0
    printf("  ");
    for (i = 0; i < frame->can_dlc; i++) {
	if (isprint(frame->data[i]))
	    printf("%c", frame->data[i]);
	else
	    putchar(46);
    }
    printf("\n");
#endif
    return 0;
}

void print_ascii_data(struct can_frame *frame) {
    int i;

    printf("  '");
    for (i = 0; i < 8; i++) {
	if (isprint(frame->data[i]))
	    printf("%c", frame->data[i]);
	else
	    putchar(46);
    }
    printf("'\n");
}

void write_candumpfile(FILE *fp, struct timeval tv, char *name, struct can_frame *frame) {

    fprintf(fp, "(%ld.%06ld) %s ", tv.tv_sec, tv.tv_usec, name);
    if (frame->can_id & (CAN_EFF_FLAG | CAN_ERR_FLAG)) {
	fprintf(fp, "%08X#", frame->can_id & ~CAN_EFF_FLAG);
    } else {
	fprintf(fp, "%03X#", frame->can_id & CAN_SFF_MASK);
	if (frame->can_id & CAN_RTR_FLAG) {
	    fprintf(fp, "R%d\n", frame->can_dlc);
	    return;
	}
    }
    if (frame->can_dlc <= 8)
	for (int i = 0; i < frame->can_dlc; i++)
	    fprintf(fp, "%02X", frame->data[i]);
    fprintf(fp, "\n");
}

void decode_frame(struct can_frame *frame) {
    uint32_t function, uid, cv_number, cv_index;
    uint16_t kenner, kontakt;
    char s[32];
    float v;

    if (frame->can_id & 0x00010000UL)
	printf(CYN);
    else
	printf(YEL);

    switch ((frame->can_id & 0x01FF0000UL) >> 16) {
    /* System Befehle */
    case 0x00:
    case 0x01:
	decode_cs2_system(frame);
	break;
    /* Lok Discovery */
    case 0x02:
    case 0x03:
	if (frame->can_dlc == 0)
	    printf("Lok Discovery - Erkennen alle Protokolle");
	if (frame->can_dlc == 1) {
	    printf("Lok Discovery - ");
	    print_loc_proto(frame->data[0]);
	}
	if (frame->can_dlc == 5) {
	    uid = be32(frame->data);
	    /* print decimal if MM2 (0x21 || 0x22) */
	    if (frame->data[4] == 0x21 || frame->data[4] == 0x22)
		printf("Lok Discovery - Adresse %u ", uid);
	    else
		printf("Lok Discovery - 0x%08X ", uid);
	    print_loc_proto(frame->data[4]);
	}
	if (frame->can_dlc == 6) {
	    uid = be32(frame->data);
	    printf("Lok Discovery - 0x%08X Range %d ASK %d", uid, frame->data[4], frame->data[5]);
	}
	printf("\n");
	break;
    /* MFX Bind */
    case 0x04:
    case 0x05:
	switch (frame->can_dlc) {
	case 2:
	case 4:
	    cdb_extension_grd(frame);
	    break;
	case 6:
	    uid = be32(frame->data);
	    printf("MFX Bind: MFX UID 0x%08X MFX SID %d\n", uid, be16(&frame->data[4]));
	    break;
	default:
	    printf("unbekannt\n");
	}
	break;
    /* MFX Verify */
    case 0x06:
    case 0x07:
	uid = be32(frame->data);
	if (frame->can_dlc == 2) {
	    kenner = be16(frame->data);
	    if (kenner == 0x00ff)
		printf("CdB: Reset");
	    else
		printf("CdB: unbekannt 0x%04x", kenner);
	}
	if (frame->can_dlc == 4) {
	    cdb_extension_set_grd(frame);
	}
	if (frame->can_dlc == 6)
	    printf("MFX Verify: MFX UID 0x%08X MFX SID %d", uid, be16(&frame->data[4]));
	if (frame->can_dlc == 7)
	    printf("MFX Verify: MFX UID 0x%08X MFX SID %d ASK-Verhältnis %d",
		   uid, be16(&frame->data[4]), frame->data[6]);
	printf("\n");
	break;
    /* Lok Geschwindigkeit */
    case 0x08:
    case 0x09:
	v = be16(&frame->data[4]);
	v = v / 10;
	if (frame->can_dlc == 4)
	    printf("Lok %s Abfrage Fahrstufe", getLoco(frame->data, s));
	else if (frame->can_dlc == 6)
	    printf("Lok %s Geschwindigkeit: %3.1f", getLoco(frame->data, s), v);
	printf("\n");
	break;
    /* Lok Richtung */
    case 0x0A:
    case 0x0B:
	memset(s, 0, sizeof(s));

	printf("Lok %s ", getLoco(frame->data, s));
	if (frame->can_dlc == 4) {
	    printf("Richtung wird abgefragt");
	} else if (frame->can_dlc == 5) {
	    switch (frame->data[4]) {
	    case 0:
		printf("Richtung bleibt gleich");
		break;
	    case 1:
		printf("Richtung: vorwärts");
		break;
	    case 2:
		printf("Richtung: rückwärts");
		break;
	    case 3:
		printf("Richtung wechseln");
		break;
	    default:
		printf("Richtung unbekannt");
		break;
	    }
	}
	printf("\n");
	break;
    /* Lok Funktion */
    case 0x0C:
    case 0x0D:
	if (frame->can_dlc == 5)
	    printf("Lok %s Funktion %d", getLoco(frame->data, s), frame->data[4]);
	else if (frame->can_dlc == 6)
	    printf("Lok %s Funktion %d Wert %d", getLoco(frame->data, s), frame->data[4], frame->data[5]);
	else if (frame->can_dlc == 7)
	    printf("Lok %s Funktion %d Wert %d Funktionswert %d",
		   getLoco(frame->data, s), frame->data[4], frame->data[5], be16(&frame->data[6]));
	printf("\n");
	break;
    /* Read Config */
    case 0x0E:
	if (frame->can_dlc == 7) {
	    cv_number = ((frame->data[4] & 0x3) << 8) + frame->data[5];
	    cv_index = frame->data[4] >> 2;
	    printf("Read Config Lok %s CV Nummer %u Index %u Anzahl %u",
		   getLoco(frame->data, s), cv_number, cv_index, frame->data[6]);
	}
	printf("\n");
	break;
    case 0x0F:
	cv_number = ((frame->data[4] & 0x3) << 8) + frame->data[5];
	cv_index = frame->data[4] >> 2;
	if (frame->can_dlc == 6)
	    printf("Read Config Lok %s CV Nummer %u Index %u", getLoco(frame->data, s), cv_number, cv_index);
	if (frame->can_dlc == 7)
	    printf("Read Config Lok %s CV Nummer %u Index %u Wert %u",
		   getLoco(frame->data, s), cv_number, cv_index, frame->data[6]);
	printf("\n");
	break;
    /* Write Config */
    case 0x10:
    case 0x11:
	/* TODO */
	cv_number = ((frame->data[4] & 0x3) << 8) + frame->data[5];
	cv_index = frame->data[4] >> 2;
	if (frame->can_dlc == 8)
	    printf("Write Config Lok %s CV Nummer %u Index %u Wert %u Ctrl 0x%02X\n", getLoco(frame->data, s),
		   cv_number, cv_index, frame->data[6], frame->data[7]);
	else
	    printf("Write Config Lok %s Befehl unbekannt\n", getLoco(frame->data, s));
	break;
    /* Zubehör schalten */
    case 0x16:
    case 0x17:
	uid = be32(frame->data);
	if (frame->can_dlc >= 6) {
	    if ((uid > 0x2FFF) && (uid < 0x3400))
		printf("Magnetartikel MM1 ID %u Ausgang %u Strom %u", uid - 0x2FFF, frame->data[4], frame->data[5]);
	    else if ((uid > 0x37FF) && (uid < 0x4000))
		printf("Magnetartikel DCC ID %u Ausgang %u Strom %u", uid - 0x37FF, frame->data[4], frame->data[5]);
	    else
		printf("Magnetartikel ID 0x%08X Ausgang %u Strom %u", uid, frame->data[4], frame->data[5]);
	}
	if (frame->can_dlc == 8)
	    printf(" Schaltzeit/Sonderfunktionswert %u (%u ms)", be16(&frame->data[6]), be16(&frame->data[6]) * 10U);
	printf("\n");
	break;
    /* S88 Polling */
    case 0x20:
	uid = be32(frame->data);
	printf("S88 Polling 0x%04X Modul Anzahl %d\n", uid, frame->data[4]);
	break;
    case 0x21:
	uid = be32(frame->data);
	printf("S88 Polling 0x%04X Modul %d Zustand %d\n", uid, frame->data[4], be16(&frame->data[5]));
	break;
    /* S88 Event */
    case 0x22:
	/* TODO: Parameter */
	kenner = be16(frame->data);
	kontakt = be16(&frame->data[2]);
	if (frame->can_dlc == 4)
	    printf("S88 Event Kennung %d Kontakt %d", kenner, kontakt);
	else if (frame->can_dlc == 5)
	    printf("S88 Event Kennung %d Kontakt %d Parameter %d", kenner, kontakt, frame->data[4]);
	else if (frame->can_dlc == 7)
	    printf("S88 Event Blockmodus Kennung %d Kontakt Start %d Parameter %d", kenner, kontakt, frame->data[6]);
	printf("\n");
	break;
    case 0x23:
	kenner = be16(frame->data);
	kontakt = be16(&frame->data[2]);
	if (frame->can_dlc == 8)
	    printf("S88 Event Kennung %d Kontakt %d Zustand alt %d Zustand neu %d Zeit %d",
		   kenner, kontakt, frame->data[4], frame->data[5], be16(&frame->data[6]));
	printf("\n");
	break;
    /* SX1 Event */
    case 0x24:
    case 0x25:
	uid = be32(frame->data);
	if (frame->can_dlc == 5)
	    printf("SX1 Event UID 0x%08X SX1-Adresse %d", uid, frame->data[4]);
	if (frame->can_dlc == 6)
	    printf("SX1 Event UID 0x%08X SX1-Adresse %d Zustand %d", uid, frame->data[4], frame->data[5]);
	printf("\n");
	break;
    /* Ping */
    case 0x30:
	printf("Ping Anfrage\n");
	break;
    case 0x31:
	printf("Ping Antwort von ");
	decode_cs2_can_identifier(frame);
	break;
    /* CAN Bootloader */
    case 0x36:
	printf("CAN Bootloader");
	uid = be32(frame->data);
	if (frame->can_dlc == 5) {
	    if (uid)
		printf(" UID 0x%08X", uid);
	    else
		printf(" alle");
	    switch (frame->data[4]) {
	    case 0x11:
		printf(" Go");
		break;
	    case 0x44:
		printf(" Block %d", frame->data[5]);
		break;
	    case 0xE4:
		printf(" ?");
		break;
	    default:
		printf(" ???");
		break;
	    }
	} else if (frame->can_dlc == 6) {
	    printf(" Anfrage Block %d", frame->data[5]);
	} else if (frame->can_dlc == 7) {
	    printf(" Block CRC 0x%04X", be16(&frame->data[5]));
	} else if (frame->can_dlc == 8) {
	    printf(" Data");
	} else {
	    printf(" Anfrage");
	}
	printf("\n");
	break;
    case 0x37:
	uid = be32(frame->data);
	if (frame->can_dlc == 5) {
	    if (frame->data[4] == 0x88)
		printf("CAN Bootloader Antwort ACK\n");
	    if (((frame->data[4] & 0xf0) == 0xf0) && ((frame->data[4] & 0x0f) <= 4))
		printf("CAN Bootloader Error %d\n", frame->data[4] & 0xf0);
	    break;
	}
	if (frame->can_dlc == 6) {
	    printf("CAN Bootloader Antwort Block %d\n", frame->data[5]);
	    break;
	}
	printf("Bootloader Antwort von ");
	decode_cs2_can_identifier(frame);
	break;
    /* Statusdaten Konfiguration */
    case 0x3A:
    case 0x3B:
	decode_cs2_can_channels(frame);
	break;
    /* Mag Update paket */
    case 0x3C:
    case 0x3D:
	printf("Update für Magnetartikel - undokumentiert\n");
	break;
    /* MfxRaw paket */
    case 0x3E:
    case 0x3F:
	printf("MFX-Rohpaket - undokumentiert\n");
	break;
    /* Anfordern Config Daten */
    case 0x41:
	if (config_data.deflated_data && (config_data.deflated_size_counter < config_data.deflated_size))
	    printf("[Config Data %s unvollständig] ", config_data.name);
	memset(config_data.name, 0, sizeof(config_data.name));
	memcpy(config_data.name, frame->data, 8);
	/* falls through */
    case 0x40:
	memset(s, 0, sizeof(s));
	memcpy(s, frame->data, frame->can_dlc);
	/* WeichenChef Erweiterung */
	if ((frame->can_id & 0x00FEFFFE) == 0x00404A80) {
	    if (frame->can_dlc == 8) {
		int i;
		printf("CdB: Weichenchef");
		for (i = 0; i < 4; i++) {
		    printf(" Adresse %d", frame->data[i * 2 + 1] + 1);
		    if (frame->data[i * 2] == 0x30)
			printf("MM");
		    else if (frame->data[i * 2] == 0x38)
			printf("DCC");
		}
		printf("\n");
	    } else {
		printf("CdB: Abfrage Weichenchef\n");
	    }
	} else {
	    printf("Anfordern Config Data: %s\n", s);
	}
	break;
    /* Config Data Stream */
    case 0x42:
    case 0x43:
	decode_cs2_config_data(frame, expconf);
	break;
    /* 6021 or CdB WeichenChef */
    case 0x44:
    case 0x45:
	if (frame->can_dlc == 6) {
	    uid = be32(frame->data);
	    kenner = be16(&frame->data[4]);
	    printf("Connect6021 UID 0x%08X mit Kenner 0x%04X\n", uid, kenner);
	} else {
	    cdb_extension_wc(frame);
	}
	break;
    /* Automatik schalten */
    case 0x60:
    case 0x61:
	kenner = be16(frame->data);
	function = be16(&frame->data[2]);
	if (frame->can_dlc == 6)
	    printf("Automatik schalten: ID 0x%04X Funktion 0x%04X Stellung 0x%02X Parameter 0x%02X\n",
		   kenner, function, frame->data[4], frame->data[5]);
	if (frame->can_dlc == 8)
	    printf("Automatik schalten: ID 0x%04X Funktion 0x%04X Lok %s\n", kenner, function,
		   getLoco(&frame->data[4], s));
	break;
    /* Blocktext zuordnen */
    case 0x62:
    case 0x63:
	kenner = be16(frame->data);
	function = be16(&frame->data[2]);
	if (frame->can_dlc == 4)
	    printf("Blocktext zuordnen: ID 0x%04X Funktion 0x%04X\n", kenner, function);
	if (frame->can_dlc == 8)
	    printf("Blocktext zuordnen: ID 0x%04X Funktion 0x%04X Lok %s\n", kenner, function, getLoco(&frame->data[4], s));
	break;
    case 0x64:
    case 0x65:
	printf("Debug - undokumentiert\n");
	break;
    case 0x84:
	printf("Debug Text");
	print_ascii_data(frame);
	break;

    /* MäCAN Bootloader/Updater */
    case 0x80:
    case 0x81:
	decode_can_maecan(frame);
	break;
    default:
	printf("unknown\n");
	break;
    }
}

void analyze_frame(struct can_frame *frame) {
    if (frame->can_id & CAN_EFF_FLAG) {	/* decode only EFF frames */
	print_can_frame(F_N_CAN_FORMAT_STRG, frame);
	if (check_cs1_frame(frame->can_id))
	    decode_frame_cs1(frame);
	else
	    decode_frame(frame);
    } else {
	if (frame->can_id & CAN_ERR_FLAG) {
	    print_can_frame(F_N_CAN_FORMAT_STRG, frame);
	    printf(RED "*** ERRORFRAME ***" RESET);
	    if (verbose) {
		char buf[CL_LONGCFSZ];
		snprintf_can_error_frame(buf, sizeof(buf), (struct canfd_frame *)frame, "\n\t");
		printf("\n\t%s", buf);
	    }
	} else {
	    print_can_frame(F_N_SFF_FORMAT_STRG, frame);
	}
	printf("\n");
    }
}

int main(int argc, char **argv) {
    int max_fds, opt, sc;
    struct can_frame frame;
    char pcap_file[256];
    char candump_file[256];
    char roctrc_file[256];
    struct sockaddr_can caddr;
    struct ifreq ifr;
    socklen_t caddrlen = sizeof(caddr);
    fd_set read_fds;
    char timestamp[32];
    int selint = 0;
    int live_capture = 0;
    FILE *fdump = NULL;

    strcpy(ifr.ifr_name, "can0");
    memset(pcap_file, 0, sizeof(pcap_file));
    memset(candump_file, 0, sizeof(candump_file));
    memset(roctrc_file, 0, sizeof(roctrc_file));

    signal(SIGINT, INThandler);

    while ((opt = getopt(argc, argv, "i:r:l:t:dsvxh?")) != -1) {
	switch (opt) {
	case 'i':
	    strncpy(ifr.ifr_name, optarg, sizeof(ifr.ifr_name) - 1);
	    break;
	case 'r':
	    strncpy(pcap_file, optarg, sizeof(pcap_file) - 1);
	    break;
	case 'l':
	    strncpy(candump_file, optarg, sizeof(candump_file) - 1);
	    break;
	case 't':
	    strncpy(roctrc_file, optarg, sizeof(roctrc_file) - 1);
	    break;
	case 'd':
	    fdump = fopen("candump.log", "w");
	    if (!fdump)
		fprintf(stderr, "\ncan't open file candump.log for writing - error: %s\n", strerror(errno));
	    break;
	case 's':
	    selint = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'x':
	    expconf = 1;
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

    messwert_knoten = calloc(1, sizeof (struct knoten));

    /* do we have a candump file ? */
    if (candump_file[0] != 0) {
	FILE *fp;
	char *line, *pos_r;
	char datum[MAXSIZE];
	size_t size = MAXSIZE;
	struct can_frame aframe;
	int time, milli;
	time_t rawtime;
	struct tm ts;

	fp = fopen(candump_file, "r");
	if (!fp) {
	    fprintf(stderr, "\ncan't open file %s for reading - error: %s\n", candump_file, strerror(errno));
	    exit(EXIT_FAILURE);
	}

	line = (char *)malloc(MAXSIZE);
	if (line == NULL) {
	    fprintf(stderr, "Unable to allocate buffer\n");
	    exit(EXIT_FAILURE);
	}

	memset(datum, 0, sizeof(datum));
	while (getline(&line, &size, fp) > 0) {
	    if (sscanf(line, "(%d.%3d)", &time, &milli) == 2) {
		rawtime = time;
		ts = *localtime(&rawtime);
		strftime(datum, sizeof(datum), "%Y%m%d.%H%M%S", &ts);
		pos_r = line;
		while (!isalpha(*pos_r))
		    pos_r++;
		printf(RESET "%s.%03d  %.5s", datum, milli, pos_r - 1);
		memset(&aframe, 0, sizeof(aframe));
		while (!isspace(*pos_r))
		    pos_r++;
		candump_to_can(pos_r, &aframe);
		analyze_frame(&aframe);
	    }
	}
	printf(RESET);
	return (EXIT_SUCCESS);
    }

    /* do we have a Rocrail trace file ? */
    if (roctrc_file[0] != 0) {
	FILE *fp;
	char *line;
	char can_string[MAXSIZE];
	char datum[MAXSIZE];
	size_t size = MAXSIZE;
	char *pos_r, *pos_w, *pos_0;
	struct can_frame aframe;
	int date, time, milli, slcan_format = 0;

	fp = fopen(roctrc_file, "r");
	if (!fp) {
	    fprintf(stderr, "\ncan't open file %s for reading - error: %s\n", roctrc_file, strerror(errno));
	    exit(EXIT_FAILURE);
	}

	line = (char *)malloc(MAXSIZE);
	if (line == NULL) {
	    fprintf(stderr, "Unable to allocate buffer\n");
	    exit(EXIT_FAILURE);
	}

	memset(datum, 0, sizeof(datum));
	while (getline(&line, &size, fp) > 0) {
	    /* line[strcspn(line, "\r\n")] = 0; */
	    memset(can_string, 0, sizeof(can_string));
	    pos_0 = strstr(line, "00000000: ");
	    if ((sscanf(line, "%8d.%6d.%3d", &date, &time, &milli)) == 3) {
		sscanf(line, "%19s ", datum);
	    }
	    pos_r = strstr(line, "ASCII read: ");
	    if (pos_r) {
		slcan_format = 1;
		sscanf(pos_r, "ASCII read: %27s", can_string);
	    }
	    pos_w = strstr(line, "ASCII write: ");
	    if (pos_w) {
		slcan_format = 1;
		sscanf(pos_w, "ASCII write: %27s", can_string);
	    }
	    if (pos_r || pos_w) {
		memset(&aframe, 0, sizeof(aframe));
		slcan_to_can(can_string, &aframe);
		printf(RESET "%s %30s", datum, can_string);
		print_can_frame(F_N_CAN_FORMAT_STRG, &aframe);
		decode_frame(&aframe);
	    }
	    if ((slcan_format == 0) && pos_0) {
		sscanf(pos_0, "00000000: %39c", can_string);
		memset(&aframe, 0, sizeof(aframe));
		ascii_to_can(can_string, &aframe);
		printf(RESET "%s  %30s", datum, can_string);
		print_can_frame(F_N_CAN_FORMAT_STRG, &aframe);
		decode_frame(&aframe);
	    }
	}
	return (EXIT_SUCCESS);
    }

    /* TODO : check the interface capabilities not the name */
    if (!strstr(ifr.ifr_name, "can"))
	live_capture = 1;

    /* do we have a PCAP file or live capture ? */
    if ((pcap_file[0] != 0) || live_capture) {
	unsigned int pkt_counter = 1;
	struct tcphdr *mytcp;
	struct udphdr *myudp;
	pcap_t *handle;
	char errbuf[PCAP_ERRBUF_SIZE];
	const unsigned char *packet;
	struct pcap_pkthdr header;
	struct ip *ip_hdr;
	struct tm *tm;
	uint16_t sport, dport;
	memset(timestamp, 0, sizeof(timestamp));

	if (live_capture) {
	    handle = pcap_open_live(ifr.ifr_name, 1514, 1, 50, errbuf);
	    if (handle == NULL) {
		fprintf(stderr, "Couldn't open interface %s: %s\n", ifr.ifr_name, errbuf);
		return (EXIT_FAILURE);
	    }
	} else {
	    handle = pcap_open_offline(pcap_file, errbuf);
	    if (handle == NULL) {
		fprintf(stderr, "Couldn't open pcap file %s: %s\n", pcap_file, errbuf);
		return (EXIT_FAILURE);
	    }
	}
	int caplinktype = pcap_datalink(handle);
	if (verbose)
	    printf("Analyzing file %s with capture link type %d\n", pcap_file, caplinktype);

	while (((packet = pcap_next(handle, &header)) != NULL) || live_capture) {
	    if (packet == NULL)
		continue;
	    pkt_counter++;
	    /* header contains information about the packet (e.g. timestamp) */
	    /* cast a pointer to the packet data */
	    unsigned char *pkt_ptr = (u_char *) packet;
	    tm = localtime(&header.ts.tv_sec);
	    sprintf(timestamp, "%02d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec,
		    (int)header.ts.tv_usec / 1000);

	    int ether_offset = 4;
	    if (caplinktype != DLT_NULL) {	/* skip for loopback encapsulation */
		/* parse the first (ethernet) header, grabbing the type field */
		ether_offset = (caplinktype == DLT_LINUX_SLL) ? 14 : 12;
		int ether_type = be16(&pkt_ptr[ether_offset]);

		if (ether_type == ETHER_TYPE_IP) {		/* most common */
		    ether_offset += 2;
		} else if (ether_type == ETHER_TYPE_8021Q) {	/* dot1q tag ? */
		    ether_offset += 6;
		} else if (ether_type == 0x000C) {		/* CAN ? */
		    ether_offset = 0;
		} else {					/* do not report ARP and HomePlug */
		    if (verbose && !live_capture && (ether_type != 0x0806) && (ether_type != 0x88E1))
			fprintf(stderr, RESET "Unknown ethernet type, %04X, skipping...\n", ether_type);
		    continue;
		}
	    }

	    if (ether_offset == 0) {
		if (be16(pkt_ptr) == 0x0001) {
		    canframe_to_can(&pkt_ptr[16], &frame);
		    printf("%s ", timestamp);
		    print_can_frame(F_N_CAN_FORMAT_STRG, &frame);
		    decode_frame(&frame);
		    printf(RESET);
		    if (fdump)
			write_candumpfile(fdump, header.ts, "can", &frame);
		}
		continue;
	    }

	    /* skip past the Ethernet II header */
	    pkt_ptr += ether_offset;
	    /* point to an IP header structure  */
	    ip_hdr = (struct ip *)pkt_ptr;

	    /* take only frames with source and destination in the same network */
	    if (selint && ((ip_hdr->ip_src.s_addr ^ ip_hdr->ip_dst.s_addr) & 0xFF) &&
		(ip_hdr->ip_dst.s_addr != 0xFFFFFFFF))
		continue;
	    int packet_length = ntohs(ip_hdr->ip_len);

	    if (ip_hdr->ip_p == IPPROTO_UDP) {
		myudp = (struct udphdr *)(pkt_ptr + sizeof(struct ip));
		dport = ntohs(myudp->uh_dport);
		sport = ntohs(myudp->uh_sport);
		if ((dport != 15730) && (sport != 15730) && (dport != 15731) && (sport != 15731)
			&& (dport != 5728) && (dport != 21105) && (dport != 21106))
		    continue;
		printf(RESET);
		int size_payload = packet_length - (IPHDR_LEN + sizeof(struct udphdr));
		if (verbose) {
		    printf("%s ", timestamp);
		    printf("%04u UDP %s -> ", pkt_counter, inet_ntoa(ip_hdr->ip_src));
		    printf("%s port %d -> %d", inet_ntoa(ip_hdr->ip_dst), ntohs(myudp->uh_sport), ntohs(myudp->uh_dport));
		    printf("  packet_length %d\n", size_payload);
		}
		unsigned char *dump = (unsigned char *)pkt_ptr + IPHDR_LEN + sizeof(struct udphdr);
		if (dport == 5728) {
		    printf("%s %.3d>  UDP  Z21-CONF", timestamp, (ip_hdr->ip_src.s_addr) >> 24);
		    z21_conf_info(dump, size_payload);
		    continue;
		} else if ((dport == 21105) || (dport == 21106)) {
		    z21_comm_ext(timestamp, (ip_hdr->ip_src.s_addr) >> 24, dump, size_payload);
		    continue;
		}
		for (int i = 0; i < size_payload; i += 13) {
		    printf("%s %.3d>", timestamp, (ip_hdr->ip_src.s_addr) >> 24);
		    frame_to_can(dump + i, &frame);
		    print_can_frame(F_N_UDP_FORMAT_STRG, &frame);
		    if (check_cs1_frame(frame.can_id))
			decode_frame_cs1(&frame);
		    else
			decode_frame(&frame);
		    printf(RESET);
		    if (fdump) {
			frame.can_id |= CAN_EFF_FLAG;
			write_candumpfile(fdump, header.ts, "udp", &frame);
		    }
		}
	    }
	    if (ip_hdr->ip_p == IPPROTO_TCP) {
		mytcp = (struct tcphdr *)(pkt_ptr + sizeof(struct ip));
		int tcp_offset = mytcp->th_off * 4;
		int size_payload = packet_length - (IPHDR_LEN + tcp_offset);
		unsigned char *dump = (unsigned char *)pkt_ptr + IPHDR_LEN + tcp_offset;

		dport = ntohs(mytcp->th_dport);
		sport = ntohs(mytcp->th_sport);
		/* look for HTTP */
		if (sport == 80 || dport == 80) {
		    if (size_payload) {
			if (verbose) {
			    printf("%s ", timestamp);
			    printf("%04u HTTP %s -> ", pkt_counter, inet_ntoa(ip_hdr->ip_src));
			    printf("%s port %d -> %d", inet_ntoa(ip_hdr->ip_dst), ntohs(mytcp->th_sport), ntohs(mytcp->th_dport));
			    printf("  packet_length %d\n", size_payload);
			}
			printf("%s %.3d>  HTTP    -> ", timestamp, (ip_hdr->ip_src.s_addr) >> 24);
			for (int i = 0; i < size_payload; i++)
			    putchar(dump[i]);
			if (dump[size_payload - 1] != '\n')
			    putchar('\n');
		    }
		    continue;
		} else if (sport == 5728 || dport == 5728) {
		    if (size_payload) {
			if (verbose) {
			    printf("%s ", timestamp);
			    printf("%04u TCP %s -> ", pkt_counter, inet_ntoa(ip_hdr->ip_src));
			    printf("%s port %d -> %d", inet_ntoa(ip_hdr->ip_dst), ntohs(mytcp->th_sport),
				   ntohs(mytcp->th_dport));
			    printf("  packet_length %4d\n", size_payload);
			}
			printf("%s %.3d>  TCP  Z21-CONF", timestamp, (ip_hdr->ip_src.s_addr) >> 24);
			z21_conf_info(dump, size_payload);
		    }
		    continue;
		}
		if ((dport != 15730) && (sport != 15730) &&
		    (dport != 15731) && (sport != 15731) && (dport != 15732) && (sport != 15732))
		    continue;
		if (size_payload > 0) {
		    if (verbose) {
			printf("%s ", timestamp);
			printf("%04u TCP %s -> ", pkt_counter, inet_ntoa(ip_hdr->ip_src));
			printf("%s port %d -> %d", inet_ntoa(ip_hdr->ip_dst), ntohs(mytcp->th_sport),
			       ntohs(mytcp->th_dport));
			printf("  packet_length %d\n", size_payload);
		    }
		    /* TCP port 15732 uses clear text */
		    if ((sport == 15732) || (dport == 15732)) {
			printf("%s %.3d>", timestamp, (ip_hdr->ip_src.s_addr) >> 24);
			printf(" TCP Port 15732      ");
			if (sport == 15732)
			    printf(CYN);
			else
			    printf(YEL);
			printf("      %.*s", size_payload, dump);
			printf(RESET);
		    } else {
			for (int i = 0; i < size_payload; i += 13) {
			    printf("%s %.3d>", timestamp, (ip_hdr->ip_src.s_addr) >> 24);
			    frame_to_can(dump + i, &frame);
			    print_can_frame(F_N_TCP_FORMAT_STRG, &frame);
			    if (check_cs1_frame(frame.can_id))
				decode_frame_cs1(&frame);
			    else
				decode_frame(&frame);
			    /* print_content(dump, size_payload); */
			    printf(RESET);
			    if (fdump) {
				frame.can_id |= CAN_EFF_FLAG;
				write_candumpfile(fdump, header.ts, "tcp", &frame);
			    }
			}
		    }
		}
	    }
	    printf(RESET);
	}
	pcap_close(handle);
	return (EXIT_SUCCESS);
    /* reading from CAN socket */
    } else {

	memset(&caddr, 0, sizeof(caddr));

	/* prepare CAN socket */
	if ((sc = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
	    fprintf(stderr, "error creating CAN socket: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	caddr.can_family = AF_CAN;
	if (ioctl(sc, SIOCGIFINDEX, &ifr) < 0) {
	    fprintf(stderr, "setup CAN socket error: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	caddr.can_ifindex = ifr.ifr_ifindex;

	if (bind(sc, (struct sockaddr *)&caddr, caddrlen) < 0) {
	    fprintf(stderr, "error binding CAN socket: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	can_err_mask_t err_mask = CAN_ERR_MASK;
	if (setsockopt(sc, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask)) < 0)
	    fprintf(stderr, "error enabling CAN error reporting: %s\n", strerror(errno));

	FD_ZERO(&read_fds);
	FD_SET(sc, &read_fds);
	max_fds = sc;

	while (1) {
	    if (select(max_fds + 1, &read_fds, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "select error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	    }
	    /* received a CAN frame */
	    if (FD_ISSET(sc, &read_fds)) {
		printf(RESET);
		if (read(sc, &frame, sizeof(struct can_frame)) < 0) {
		    fprintf(stderr, "error reading CAN frame: %s\n", strerror(errno));
		} else {
		    struct timeval tv = time_stamp(timestamp);
		    printf("%s ", timestamp);
		    analyze_frame(&frame);
		    if (fdump)
			write_candumpfile(fdump, tv, ifr.ifr_name, &frame);
		}
	    }
	}
	close(sc);
    }
    return 0;
}
