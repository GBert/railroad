/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <linux/can.h>

#include "tools.h"
#include "can-monitor.h"

char *getLoco(uint8_t * data, char *s) {
    uint16_t locID = be16(&data[2]);
    char prot[32];
    int addrs;

    memset(prot, 0, sizeof(prot));

    if (locID <= 0x03ff) {
	strncpy(prot, " mm-", sizeof(prot));
	addrs = locID;
    } else if (locID >= 0x4000 && locID < 0xC000) {
	strncpy(prot, "mfx-", sizeof(prot));
	addrs = locID - 0x4000;
    } else if (locID >= 0xC000) {
	strncpy(prot, "dcc-", sizeof(prot));
	addrs = locID - 0xC000;
    } else {
	strncpy(prot, "unbekannt-", sizeof(prot));
	addrs = 0;
    }

    sprintf(s, "%s%d", prot, addrs);
    return s;
}

void decode_cs2_can_member(struct can_frame *frame) {
    uint32_t uid;
    uint16_t typ;

    uid = be32(frame->data);
    typ = be16(&frame->data[6]);
    printf("Ping Antwort von ");

    switch (typ) {
    case 0x0000:
	if ((uid & 0xff000000) == 0x42000000)
	    printf("Booster (6017x)");
	else
	    printf("GFP");
	break;
    case 0x0010:
    case 0x0011:
	printf("Gleisbox");
	break;
    case 0x0020:
	printf("Connect6021");
	break;

    case 0x0031:
    case 0x0032:
    case 0x0033:
    case 0x0034:
	printf("MS2");
	break;
    case 0x0040:
	if ((uid & 0xFFF00000) == 0x53300000)
	    printf("LinkS88");
	else if ((uid & 0xFFFF0000) == 0x43420000)
	    printf("S88 Gateway");
	else
	    printf("S88 Unbekannt");
	break;
    case 0x0051:
	if ((uid & 0xffff0000) == 0x4d430000)
	    printf("MäCAN Busankoppler");
	break;
    case 0x0052:
	if ((uid & 0xffff0000) == 0x4d430000)
	    printf("MäCAN MP5x16");
	break;
    case 0x0053:
	if ((uid & 0xffff0000) == 0x4d430000)
	    printf("MäCAN Dx32");
	else
	    printf("Cg Servo");
	break;
    case 0x0054:
	printf("Cg Rückmelder");
	break;
    case 0x4681:
    case 0x46FF:
	printf("Rocrail");
	break;
    case 0x1234:
	printf("MäCAN-Weichendecoder");
	break;
    case 0xEEEE:
	printf("CS2 Software");
	break;
    case 0xFFFF:
	printf("CS2-GUI (Master)");
	break;
    default:
	printf("unbekannt");
	break;
    }
    printf(" UID 0x%08X, Software Version %d.%d\n", uid, frame->data[4], frame->data[5]);
}

void decode_cs2_system(struct can_frame *frame) {
    uint32_t uid, response;
    uint16_t sid, wert;
    static uint16_t crcreg;
    uint8_t modul;
    char s[32];

    memset(s, 0, sizeof(s));
    response = frame->can_id & 0x00010000;

    /* CdB extension */
    if (frame->can_dlc == 0) {
	modul = frame->can_id & 0x7F;
	if (response)
	    printf("System: CdB Such-Antwort Modul %d\n", modul);
	else
	    printf("System: CdB Suche\n");
	return;
    }

    uid = be32(frame->data);
    if (frame->can_dlc == 4) {
	if (uid)
	    printf("System: UID 0x%08X ", uid);
	else
	    printf("System: alle ");
	printf("Stopp/Go-Abfrage\n");
	return;
    }
    switch (frame->data[4]) {
    case 0x00:
	if (uid)
	    printf("System: UID 0x%08X ", uid);
	else
	    printf("System: alle ");
	writeRed("Stopp");
	break;
    case 0x01:
	if (uid)
	    printf("System: UID 0x%08X ", uid);
	else
	    printf("System: alle ");
	writeGreen("Go");
	break;
    case 0x02:
	if (uid)
	    printf("System: UID 0x%08X ", uid);
	else
	    printf("System: alle ");
	writeRed("Halt");
	break;
    case 0x03:
	printf("System: ");
	if (uid)
	    printf("Lok %s Nothalt", getLoco(frame->data, s));
	else
	    writeRed("Nothalt alle Loks");
	break;
    case 0x04:
	printf("System: Lok %s Zyklus Ende", getLoco(frame->data, s));
	break;
    case 0x05:
	printf("System: Lok %s Gleisprotokoll: %d", getLoco(frame->data, s), frame->data[5]);
	break;
    case 0x06:
	wert = be16(&frame->data[5]);
	printf("System: System Schaltzeit Zubehör UID 0x%08X Zeit 0x%04X", uid, wert);
	break;
    case 0x07:
	sid = be16(&frame->data[5]);
	printf("System: Fast Read mfx UID 0x%08X SID %d", uid, sid);
	break;
    case 0x08:
	printf("System: Gleisprotokoll freischalten -");
	if (frame->data[5] & 1)
	    printf(" MM2");
	if (frame->data[5] & 2)
	    printf(" MFX");
	if (frame->data[5] & 4)
	    printf(" DCC");
	if (frame->data[5] & 8)
	    printf(" SX1");
	if (frame->data[5] & 16)
	    printf(" SX2");
	break;
    case 0x09:
	wert = be16(&frame->data[5]);
	printf("System: Neuanmeldezähler setzen UID 0x%08X Zähler 0x%04X", uid, wert);
	break;
    case 0x0a:
	printf("System: Überlast UID 0x%08X Kanal 0x%04X", uid, frame->data[5]);
	break;
    case 0x0b:
	if (frame->can_dlc == 6)
	    printf("System: Statusabfrage UID 0x%08X Kanal 0x%02X", uid, frame->data[5]);
	if (frame->can_dlc == 7) {
	    printf("System: Konfiguration UID 0x%08X Kanal 0x%02X ", uid, frame->data[5]);
	    if (frame->data[6])
		printf(" gültig(%d)", frame->data[6]);
	    else
		printf(" ungültig(%d)", frame->data[6]);
	}
	if (frame->can_dlc == 8) {
	    wert = be16(&frame->data[6]);
	    if (response)
		printf("System: Statusabfrage UID 0x%08X Kanal 0x%02X Messwert 0x%04X", uid, frame->data[5], wert);
	    else
		printf("System: Konfiguration UID 0x%08X Kanal 0x%02X Konfigurationswert 0x%04X", uid, frame->data[5],
		       wert);
	}
	break;
    case 0x0c:
	if (frame->can_dlc == 5) {
	    printf("System: Geraetekennung UID 0x%08X", uid);
	} else {
	    wert = be16(&frame->data[5]);
	    printf("System: Geraetekennung UID 0x%08X ist 0x%04X", uid, wert);
	}
	break;
	/* Modellzeit */
    case 0x20:
	if (frame->data[7])
	    printf("System: Uhrzeit UID 0x%08X %d:%d Faktor %d (1:%d)",
		   uid, frame->data[5], frame->data[6], frame->data[7], 60 / frame->data[7]);
	else
	    printf("System: Uhrzeit UID 0x%08X %d:%d angehalten", uid, frame->data[5], frame->data[6]);
	break;
    case 0x30:
	printf("System: mfx Seek von 0x%08X:", uid);
	if (frame->can_dlc > 5)
	    switch (frame->data[5]) {
	    case 0x00:
		printf(" Warten beenden");
		break;
	    case 0x01:
		printf(" auf 1-Bit-Antwort warten");
		break;
	    case 0x02:
		printf(" auf %d-Byte-Antwort warten", frame->data[6]);
		break;
	    case 0x82:
		printf(GRN " Antwortbyte %d mit Wert 0x%02X", frame->data[6], frame->data[7]);
		if (frame->data[6] == 0)
		    crcreg = 0xFF;
		else if (frame->data[7] == crcreg)
		    printf(", CRC OK");
		crcreg ^= (crcreg << 1) ^ (crcreg << 2) ^ frame->data[7];
		if (crcreg & 0x0100)
		    crcreg ^= 0x0107;
		if (crcreg & 0x0200)
		    crcreg ^= 0x020E;
		break;
	    case 0x83:
		printf(GRN " 1-Bit-Antwort mit ASK %d", frame->data[6]);
		break;
	    default:
		printf(" unbekannter Code");
	    }
	break;
    case 0x80:
	printf("System: System Reset UID 0x%08X Ziel 0x%02X", uid, frame->data[5]);
	break;
    default:
	printf("System: unbekannt 0x%02X", frame->data[4]);
	break;
    }
    printf("\n");
}

void print_loc_proto(uint8_t proto) {
    if (proto <= 32) {
	printf("Protokoll mfx Range 0 - %d", proto);
    } else
	switch (proto) {
	case 0x21:
	    printf("Protokoll Erkennung MM2 20kHz");
	    break;
	case 0x22:
	    printf("Protokoll Erkennung MM2 40kHz");
	    break;
	case 0x23:
	case 0x24:
	case 0x25:
	    printf("Protokoll Erkennung DCC");
	    break;
	case 0x26:
	case 0x27:
	    printf("Protokoll Erkennung SX1");
	    break;
	case 0x28:
	    printf("Protokoll Erkennung mfx Zubehör");
	    break;
	default:
	    printf("Protokoll Erkennung 0x%02x", proto);
	    break;
	}
}
