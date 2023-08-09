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

void cdb_extension_grd(struct can_frame *frame) {
    uint8_t kontakt, index, modul;
    uint16_t wert;

    kontakt = *frame->data;
    index = frame->data[1];
    modul = frame->can_id & 0x7F;

    if (kontakt) {
	if (frame->can_dlc == 2) {
	    printf("CdB: Abfrage Modul %d Kontakt %d ", modul, kontakt);
	    switch (index) {
	    case 0x01:
		printf("Version\n");
		break;
	    case 0x02:
		printf("Einschaltverzögerung\n");
		break;
	    case 0x03:
		printf("Ausschaltverzögerung\n");
		break;
	    default:
		printf("unbekannt\n");
		break;
	    }
	}
	if (frame->can_dlc == 4) {
	    wert = be16(&frame->data[2]);
	    printf("CdB: Antwort Modul %d Kontakt %d ", modul, kontakt);
	    switch (index) {
	    case 0x01:
		printf("Version %d.%d\n", frame->data[2], frame->data[3]);
		break;
	    case 0x02:
		printf("Einschaltverzögerung %d ms\n", wert);
		break;
	    case 0x03:
		printf("Ausschaltverzögerung %d ms\n", wert);
		break;
	    default:
		printf("unbekannt: %d\n", wert);
		break;
	    }
	}
    /* System */
    } else {
	if (frame->can_dlc == 2) {
	    printf("CdB: Abfrage ");
	    switch (index) {
	    case 0x01:
		printf("Version\n");
		break;
	    case 0x10:
		printf("Gerätekennung\n");
		break;
	    case 0x11:
		printf("Refresh CS2-Layout\n");
		break;
	    case 0x12:
		printf("Sende Master CS2\n");
		break;
	    default:
		printf("CdB: Abfrage unbekannter Index %d\n", index);
		break;
	    }
	}
	if (frame->can_dlc == 4) {
	    wert = be16(&frame->data[2]);
	    switch (index) {
	    case 0x01:
		printf("CdB: Antwort Version %d.%d\n", frame->data[2], frame->data[3]);
		break;
	    case 0x10:
		printf("CdB: Gerätekennung %d\n", wert);
		break;
	    case 0x11:
		if (wert == 1)
		    printf("CdB: Refresh CS2-Layout\n");
		else
		    printf("CdB: kein Refresh CS2-Layout\n");
		break;
	    case 0x12:
		if (wert == 1)
		    printf("CdB: Sende Master CS2\n");
		else
		    printf("CdB: kein Sende Master CS2\n");
		break;
	    default:
		printf("CdB: Antwort unbekannter Index %d Wert 0x%04X\n", index, wert);
		break;
	    }
	}
    }
}

void cdb_extension_wc(struct can_frame *frame) {
    uint8_t modul, servo;
    uint16_t wert;

    modul = frame->can_id & 0x7F;
    servo = frame->data[0];

    if (frame->can_dlc == 2) {
	if (servo) {
	    if (frame->data[1] == 0x55)
		printf("CdB: WeichenChef Abfrage Modul %d Kontakt %d\n", modul, servo);
	    else
		printf("CdB: WeichenChef Abfrage Modul %d Servo %d\n", modul, servo);
	} else {
	    if (frame->data[1] == 1)
		printf("CdB: WeichenChef Abfrage Modul %d Version\n", modul);
	    else
		printf("CdB: WeichenChef Abfrage Modul %d System %d\n", modul, frame->data[1]);
	}
    } else if (frame->can_dlc == 4) {
	wert = be16(&frame->data[2]);
	if (servo) {
	    printf("CdB: WeichenChef Modul %d ", modul);

	    switch (frame->data[1]) {
	    case 0x04:
		printf("Servo %d Position in us Schritten: %d\n", servo, wert);
		break;
	    case 0x06:
		if (wert)
		    printf("Servo %d Spannung aus\n", servo);
		else
		    printf("Servo %d Spannung ein\n", servo);
		break;
	    case 0x08:
		if (wert)
		    printf("Servo %d CdB-Meldungen\n", servo);
		else
		    printf("Servo %d keine CdB-Meldungen\n", servo);
		break;
	    case 0x09:
		printf("Ausgang %d Adresse %d ", frame->data[0], frame->data[2] + 1);
		if (frame->data[3] == 0x38)
		    printf("DCC\n");
		else if (frame->data[3] == 0x30)
		    printf("MM\n");
		else
		    printf("Protokoll unbekannt\n");
		break;
	    case 0x0A:
		printf("Ausgang %d Adresse %d ", frame->data[0], frame->data[3] + 1);
		if (frame->data[2] == 0x38)
		    printf("DCC\n");
		else if (frame->data[2] == 0x30)
		    printf("MM\n");
		else
		    printf("Protokoll unbekannt\n");
		break;
	    case 0x0B:
		printf("Ausgang %da Adresse %d ", frame->data[0], frame->data[2] + 1);
		if (frame->data[3] == 0x38)
		    printf("DCC\n");
		else if (frame->data[3] == 0x30)
		    printf("MM\n");
		else
		    printf("Protokoll unbekannt\n");
		break;
	    case 0x0C:
		printf("Ausgang %da Adresse %d ", frame->data[0], frame->data[3] + 1);
		if (frame->data[2] == 0x38)
		    printf("DCC\n");
		else if (frame->data[2] == 0x30)
		    printf("MM\n");
		else
		    printf("Protokoll unbekannt\n");
		break;
	    case 0x12:
		printf("Servo %d Funktion %d\n", servo, wert);
		break;
	    case 0x14:
		printf("Servo %d Dauer/Zeitbetrieb %d * 0,7s\n", servo, wert);
		break;
	    case 0x22:
		printf("Servo %d Speichern als Position ", servo);
		if (frame->data[3] == 0x02)
		    printf("rot\n");
		else if (frame->data[3] == 0x03)
		    printf("grün\n");
		else
		    printf("- unbekannt\n");
		break;
	    case 0x24:
		printf("Servo %d ", servo);
		switch (frame->data[3]) {
		case 0x02:
		    printf("Speichern für Weg rot - Verzögerung %d ms\n", frame->data[2]);
		    break;
		case 0x03:
		    printf("Speichern für Weg grün - Verzögerung %d ms\n", frame->data[2]);
		    break;
		case 0x0A:
		    printf("Verzögerung %d ms je Schritt\n", frame->data[2]);
		    break;
		default:
		    printf("- unbekannt\n");
		    break;
		}
		break;
	    default:
		printf("Servo %d unbekannter Wert %d\n", servo, wert);
		break;
	    }
	/* Servo 0 -> System */
	} else {
	    switch (frame->data[1]) {
	    case 0x01:
		printf("CdB: WeichenChef Modul %d Version %d.%d\n", modul, frame->data[2], frame->data[3]);
		break;
	    case 0x18:
		printf("CdB: WeichenChef Modul %d -> Modul %d \n", modul, wert);
		break;
	    case 0xFF:
		printf("CdB: WeichenChef Modul %d Reset\n", modul);
		break;
	    default:
		printf("CdB: WeichenChef Modul %d System %d\n", modul, frame->data[1]);
		break;
	    }
	}
    } else {
	printf("CdB: WeichenChef unbekannt\n");
    }
}

void cdb_extension_set_grd(struct can_frame *frame) {
    uint16_t wert;
    uint8_t index, kontakt, modul;

    modul = frame->can_id & 0x1F;
    kontakt = frame->data[0];
    index = frame->data[1];

    if (frame->can_dlc == 4) {
	wert = be16(&frame->data[2]);
	printf("CdB: Setze Modul %d ", modul);
	if (kontakt)
	    printf("Kontakt %d ", kontakt);
	switch (index) {
	case 0x01:
	    printf("Version %d.%d\n", frame->data[2], frame->data[3]);
	    break;
	case 0x02:
	    printf("Einschaltverzögerung %d ms\n", wert);
	    break;
	case 0x03:
	    printf("Ausschaltverzögerung %d ms\n", wert);
	    break;
	case 0x10:
	    printf("Gerätekennung %d\n", wert);
	    break;
	case 0x11:
	    if (wert == 1)
		printf("Refresh CS2-Layout\n");
	    else
		printf("kein Refresh CS2-Layout\n");
	    break;
	case 0x12:
	    if (wert == 1)
		printf("Sende Master CS2\n");
	    else
		printf("Sende nicht Master CS2\n");
	    break;
	default:
	    printf("unbekannt: %d\n", wert);
	    break;
	}
    }
}
