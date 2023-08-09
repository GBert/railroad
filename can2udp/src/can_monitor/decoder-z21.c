/* Contributed by Rainer Müller */

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include "can-monitor.h"

void z21_conf_info(unsigned char *data, int datsize) {
    int show = datsize;
    if (show > 16)
	show = 16;
    printf(" Länge %4d:  ", datsize);
    for (int i = 0; i < 16; i++) {
	if (i < show)
	    printf(" %02X", data[i]);
	else
	    printf("   ");
    }
    printf("  '");
    for (int i = 0; i < show; i++) {
	if (isprint(data[i]))
	    printf("%c", data[i]);
	else
	    putchar(46);
    }
    printf("'\n");
}

void z21_comm_ext(char *timestamp, int source, unsigned char *data, int datsize) {
    while (datsize >= 4) {
	int datlen = data[0] + data[1] * 256;
	int header = data[2] + data[3] * 256;
	int par1st = 4;

	printf("%s %.3d>  UDP  Z21 L %03X, H %04X ", timestamp, source, datlen, header);
	if (header == 0x0040) {	// LAN_X_...
	    uint8_t xorval = 0;
	    for (int i = 4; i < datlen; i++)
		xorval ^= data[i];
	    if (xorval)
		printf(RED "LAN_CHECKSUM_ERROR");
	    else {
		uint8_t xheader = data[4];
		if ((xheader & 0x20) && (xheader != 0xEF)) {	// use DB0
		    uint8_t db0 = data[5];
		    printf("%02X %02X                 ", xheader, db0);
		    par1st = 6;
		    printf(BLU);
		    switch (xheader * 256 + db0) {
		    case 0x2121:
			printf("LAN_X_GET_VERSION");
			break;
		    case 0x2124:
			printf("LAN_X_GET_STATUS");
			break;
		    case 0x2180:
			printf("LAN_X_SET_TRACK_POWER_OFF");
			break;
		    case 0x2181:
			printf("LAN_X_SET_TRACK_POWER_ON");
			break;
// ...
		    case 0x6100:
			printf("LAN_X_BC_TRACK_POWER_OFF");
			break;
		    case 0x6101:
			printf("LAN_X_BC_TRACK_POWER_ON");
			break;
		    case 0x6102:
			printf("LAN_X_BC_PROGRAMMING_MODE");
			break;
		    case 0x6108:
			printf("LAN_X_BC_TRACK_SHORT_CIRCUIT");
			break;
// ...
		    case 0x6182:
			printf("LAN_X_UNKNOWN_COMMAND");
			break;
		    case 0x6222:
			printf("LAN_X_STATUS_CHANGED");
			break;
		    case 0x6321:
			printf("LAN_X_GET_VERSION");
			break;
		    case 0xE3F0:
			printf("LAN_X_GET_LOCO_INFO");
			break;
		    case 0xE410:
			printf("LAN_X_SET_LOCO_DRIVE_14");
			break;
		    case 0xE412:
			printf("LAN_X_SET_LOCO_DRIVE_28");
			break;
		    case 0xE413:
			printf("LAN_X_SET_LOCO_DRIVE_128");
			break;
		    case 0xF10A:
		    case 0xF30A:
			printf("LAN_X_GET_FIRMWARE_VERSION");
			break;
		    default:
			printf("unbekannt");
		    }
		} else {
		    printf("%02X                    ", xheader);
		    printf(BLU);
		    par1st = 5;
		    switch (xheader) {
		    case 0x43:
			printf("LAN_X_GET_TURNOUT_INFO");
			break;
		    case 0x53:
			printf("LAN_X_SET_TURNOUT");
			break;
		    case 0x80:
			printf("LAN_X_SET_STOP");
			break;
		    case 0x81:
			printf("LAN_X_BC_STOPPED");
			break;
		    case 0xEF:
			printf("LAN_X_LOCO_INFO    ");
			break;
		    default:
			printf("unbekannt");
		    }
		}
	    }
	    if ((datlen - 1) > par1st) {
		printf(":  ");
		for (int i = par1st; i < (datlen - 1); i++)
		    printf(" %02X", data[i]);
	    }
	} else {
	    printf(BLU);
	    printf("                      ");
	    switch (header) {
	    case 0x10:
		printf("LAN_GET_SERIAL_NUMBER");
		break;
	    case 0x18:
		printf("LAN_GET_CODE");
		break;
	    case 0x1A:
		printf("LAN_GET_HWINFO");
		break;
	    case 0x30:
		printf("LAN_LOGOFF");
		break;
	    case 0x50:
		printf("LAN_SET_BROADCASTFLAGS");
		break;
	    case 0x51:
		printf("LAN_GET_BROADCASTFLAGS");
		break;
	    case 0x60:
		printf("LAN_GET_LOCOMODE");
		break;
	    case 0x61:
		printf("LAN_SET_LOCOMODE");
		break;
	    case 0x70:
		printf("LAN_GET_TURNOUTMODE");
		break;
	    case 0x71:
		printf("LAN_SET_TURNOUTMODE");
		break;
	    case 0x81:
		printf("LAN_RMBUS_GETDATA");
		break;
	    case 0x82:
		printf("LAN_RMBUS_PROGRAMMODULE");
		break;
	    case 0x85:
		printf("LAN_SYSTEMSTATE_GETDATA");
		break;
	    default:
		printf("unbekannt");
	    }
	    if (datlen > 4) {
		printf(":  ");
		for (int i = 4; i < datlen; i++)
		    printf(" %02X", data[i]);
	    }
	}
	printf("\n");
	if (datlen < 4)
	    return;		// dataset violates min length          
	datsize -= datlen;
	data += datlen;
    }
}

