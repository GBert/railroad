/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/* MäCAN frame info added by github.com/Ixam97 */

#include <stdio.h>
#include <stdint.h>
#include <linux/can.h>

#include "tools.h"
#include "can-monitor.h"

void decode_can_maecan(struct can_frame *frame) {
    uint32_t uid;
    uint16_t value;

    /* MäCAN Bootloader/Updater */
    switch ((frame->can_id & 0x01FF0000UL) >> 16) {
    case 0x80:
	uid = be32(frame->data);
	printf("MäCAN Bootloader ");
	switch (frame->can_dlc) {
	case 4:
	    printf("UID 0x%08X Update-Angebot an ", uid);
	    break;
	case 6:{
		switch (frame->data[4]) {
		case 0x04:
		    printf("UID 0x%08X Page %d Beginn", uid, frame->data[5]);
		    break;
		case 0x05:
		    printf("UID 0x%08X Page %d Ende", uid, frame->data[5]);
		    break;
		case 0x07:{
			printf("UID 0x%08X Update abgeschlossen für ", uid);
			switch (frame->data[5]) {
			case 0x51:
			    printf("MäCAN Busankoppler");
			    break;
			case 0x52:
			    printf("MäCAN MP5x16");
			    break;
			case 0x53:
			    printf("MäCAN Dx32");
			    break;
			default:
			    printf("unbekannt");
			    break;
			}
			break;
		    }
		default:
		    printf("unbekannt");
		    break;
		}
		break;
	    }
	case 8:
	    printf("Data");
	    break;
	default:
	    break;
	}
	printf("\n");
	break;
    case 0x81:
	printf("MäCAN Bootloader ");
	switch (frame->can_dlc) {
	case 6:{
		uid = be32(frame->data);
		printf("UID 0x%08X ", uid);
		switch (frame->data[4]) {
		case 0x01:{
			printf("Bestätigung durch ");
			switch (frame->data[5]) {
			case 0x51:
			    printf("MäCAN Busankoppler");
			    break;
			case 0x52:
			    printf("MäCAN MP5x16");
			    break;
			case 0x53:
			    printf("MäCAN Dx32");
			    break;
			default:
			    printf("unbekannt");
			    break;
			}
			break;
		    }
		}
		break;
	case 7:
		value = be16(&frame->data[5]);
		uid = be32(frame->data);
		printf("UID 0x%08X ", uid);
		switch (frame->data[4]) {
		case 1:{

			if (frame->data[6] == 1)
			    printf("Bestätigung");
			else
			    printf("Ablehnung");
			printf(" durch Updater für ");
			switch (frame->data[5]) {
			case 0x51:
			    printf("MäCAN Busankppler");
			    break;
			case 0x52:
			    printf("MäCAN MP5x16");
			    break;
			case 0x53:
			    printf("MäCAN Dx32");
			    break;
			default:
			    printf("unbekannt");
			    break;
			}
			break;
		    }
		case 2:
		    printf("Page-Größe %d", value);
		    break;
		case 3:
		    printf("Page-Anzahl %d", value);
		    break;
		case 5:{
			printf("Page %d ", frame->data[5]);
			if (frame->data[6] == 1)
			    printf("empfangen");
			else
			    printf("fehler");
			break;
		    }

		default:
		    printf("unbekannt");
		    break;
		}
		break;
	    }
	default:
	    break;
	}
    }
}
