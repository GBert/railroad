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
#include <linux/can.h>

#include "tools.h"
#include "can-monitor.h"

#if 0
int CS1(int hash) {
    if ((hash & (1 << 7)) == 0 && (hash & (1 << 8)) != 0 && (hash & (1 << 9)) != 0)
        return 0;
    else
        return 1;
}
#endif

int check_cs1_frame(uint32_t id) {
    if ((id & M_CS2_HASH_MASK) == M_CS2_HASH_FLAG)
	return 0;
    /* sometimes the match doesn't work as described by Maerklin - workaround */
    if ((id & 0x1C000000) || ((id & 0x1FFF0080) == 0x80))
	return 1;
    return 0;
}

/*  ID field for MS1, coding for normal operation and for detection:
    28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    <-PRIO-> <------          OBJECTHANDLE           ------> <-CMND-> <---    NODE    --->
    <-PRIO-> <---      UID      ---> <-STEP-> <--  MID   --> <-CMND-> <---    NODE    --->
*/

void decode_frame_cs1(struct can_frame *frame) {
    uint8_t mid;
    uint8_t stage;
    uint8_t id;
    uint16_t objhandle = (frame->can_id >> 10) & 0xffff;
    uint8_t node = frame->can_id & 0x7f;

    if (((node == 126) && !(frame->can_id & 0x80)) || (node & 1))
	printf("[MS1] Slave  Node %u ", node);
    else
	printf("[MS1] Master Node %u ", node);
    if ((frame->can_id & 0x1FFFFFFF) < 0x1C000000)
	printf("OH %u ", objhandle);
    switch (frame->can_id & 0x1C000380) {
    case 0x00000380:		// Prio 000, Cmd 111
	printf("Abfrage Bus belegt");
	break;
    case 0x08000100:		// Prio 010, Cmd 010
	printf("Änderung FKT %u auf %u", frame->data[0], frame->data[2]);
	break;
    case 0x0C000380:		// Prio 011, Cmd 111
	printf("Ping via Node %u", frame->data[3]);
	break;
    case 0x10000100:		// Prio 100, Cmd 010
	printf("Statusänderung Typ %u auf %u", frame->data[0], frame->data[2]);
	break;
    case 0x14000000:		// Prio 101, Cmd 000
	printf("Lok-Auswahl");
	if (frame->data[3] != 8)
	    printf(" ungültig");
	break;
    case 0x18000000:		// Prio 110, Cmd 000
	printf("Zuordnung zur MS");
	if (frame->can_dlc > 3)
	    printf((frame->data[3] == 8) ? " OK" : " NOT OK");
	break;
    case 0x18000080:		// Prio 110, Cmd 001
	switch (frame->data[0]) {
	case 0x02:
	    if (frame->can_dlc < 5)
		printf("Abfrage Namesteil[%02u]", frame->data[3]);
	    else {
		printf("Namesteil[%02u]: " GRN, frame->data[3]);
		for (int i = 4; i < frame->can_dlc; i++) {
		    if (isprint(frame->data[i]))
			printf("%c", frame->data[i]);
		    else
			putchar(46);
		}
		printf(RESET);
	    }
	    break;
	case 0x40:
	    if (frame->data[1] == 3) {
		printf("Loktyp");
		if (frame->can_dlc < 6)
		    printf("-Abfrage");
		else
		    printf(" ist %u", frame->data[5]);
	    } else {
		printf("Schienenformat %u", frame->data[2]);
		if (frame->can_dlc < 6)
		    printf(" Abfrage");
		else
		    printf(" ist %u", frame->data[5]);
	    }
	    break;
	case 0x41:
	    printf("Lokstackgröße");
	    if (frame->can_dlc == 6)
		printf(" ist %u", frame->data[5]);
	    break;
	}
	break;
    case 0x18000100:		// Prio 110, Cmd 010
	switch (frame->can_dlc) {
	case 2:
	    if (frame->data[1])
		printf("Typabfrage");
	    else
		printf("Zustandsabfrage");
	    printf(" von FKT %u", frame->data[0]);
	    break;
	case 3:
	    if (frame->data[1])
		printf("Typ");
	    else
		printf("Zustand");
	    printf(" von FKT %u ist %u", frame->data[0], frame->data[2]);
	    break;
	case 4:
	    if (frame->data[1] == 2)
		printf("Erw. Abfrage");
	    else
		printf("Typbeschreibung von FKT %u", frame->data[0]);
	    break;
	default:
	    printf("Erw. Funktion");	// TODO: clarify whats behind
	}
	break;
    case 0x18000180:		// Prio 110, Cmd 011
	switch (frame->data[0]) {
	case 0x03:
	    if (frame->can_dlc < 8)
		printf("System-Handle-Anforderung für Node %u", frame->data[1]);
	    else
		printf("System-Handle für Node %u ist %u", frame->data[1], be16(frame->data + 4));
	    break;
	case 0x40:
	    printf("SH-handle");	// TODO: remove, assumed to be unused
	    break;
	case 0x80:
	    if (frame->can_dlc < 8)
		printf("Lokstack-Anfrage nach %u", be16(frame->data + 2));
	    else
		printf("neuer Lokstack-Eintrag ist %u", be16(frame->data + 4));
	    break;
	default:
	    printf("Data0 %02X unbekannt", frame->data[0]);
	}
	break;
    case 0x18000200:		// Prio 110, Cmd 100
	switch (frame->data[0]) {
	case 0x40:
	    if (frame->can_dlc < 8)
		printf("SD-Handle-Anforderung für Node %u", frame->data[1]);
	    else
		printf("SD-Handle für Node %u ist %u", frame->data[1], be16(frame->data + 4));
	    break;
	case 0x80:
	    printf("Lokstackeintrag mit Index %u hinzufügen", be16(frame->data + 4));
	    break;
	}
	break;
    case 0x18000280:		// Prio 110, Cmd 101
	printf("Lokstackeintrag mit Index %u löschen", be16(frame->data + 4));
	break;
    case 0x1C000000:		// Prio 111, Cmd 00X
    case 0x1C000080:
	id    = (objhandle >> 8) & 0xff;
	stage = (objhandle >> 5) & 0x07;
	mid   = objhandle & 0x1f;
	printf("Anmeldung MID %u Stage %u ID %02X", mid, stage, id);
	if (frame->can_dlc == 8)
	    switch (stage) {
	    case 4:
		printf(" -> UID %08X", be32(frame->data));
		break;
	    case 7:
		if (frame->can_id & 0x80)
		    printf(" -> UID %08X OH %u Node %u", be32(frame->data), be16(frame->data + 4), (frame->data[6] & 0x7F));
		else
		    printf(" -> AP Version %u.%u", frame->data[4], frame->data[5]);
		break;
	    }
	break;
    default:
	printf("<message is still not decoded>");
    }
    printf("\n");
}
