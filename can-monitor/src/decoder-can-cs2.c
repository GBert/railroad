/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/can.h>

#include "tools.h"
#include "can-monitor.h"

struct messwert_t Gleisbox_Messwerte[5] = { { 0, 0,  0,   0,   0,   0,   0,  0,    0,    0,    0,    0,    0,      "",  0.00,  0.00, "",  "" },
					    { 0, 1, -3,  48, 240, 224, 192, 15, 2060, 1648, 1730, 1895, 2060, "TRACK",  0.00,  2.50, "", "A" },
					    { 0, 2,  0,   0,   0,   0,   0,  0,    0,    0,    0,    0,    0,      "",  0.00,  0.00, "",  "" },
					    { 0, 3, -3, 192,  12,  48, 192,  0, 3145,  925, 1202, 2590, 3145,  "VOLT", 10.00, 27.00, "", "V" },
					    { 0, 4,  0,  12,   8, 240, 192,  0,  219,  107,  164,  205,  219,  "TEMP",  0.00, 80.00, "", "C" } };

extern struct cs2_config_data_t config_data;
int kanal = 0;
uint32_t channel_uid;
uint8_t channel;
uint8_t messwerte;
struct messwert_t *a_messwert = NULL;
struct knoten *messwert_knoten = NULL;
unsigned char channel_buffer[MAX_PAKETE * 8];

char *next_string(char *p) {
    /* TODO: range check */
    while (*p++) ;
    return p++;
}

int insert_right(struct knoten *liste, void *element) {
    struct knoten *tmp = liste;
    struct knoten *node = calloc(1, sizeof(struct knoten));
    if (node == NULL) {
	fprintf(stderr, "calloc failed in %s: %s\n", __func__, strerror(errno));
	return -1;
    }
    while (tmp->next != NULL)
	tmp = tmp->next;
    tmp->next = node;
    tmp->daten = element;
    return 0;
}

void print_llist(struct knoten *liste) {
    struct knoten *tmp = liste;
    struct messwert_t *messwert;

    while (tmp->daten) {
	messwert = tmp->daten;
	printf("Messwerte: %s 0x%08X Index %d\n", messwert->name, messwert->uid, messwert->index);
	tmp = tmp->next;
    }
}

struct messwert_t *suche_messwert(struct knoten *liste, uint32_t uid, uint8_t index) {
    struct knoten *tmp = liste;
    struct messwert_t *messwert_tmp;

    while (tmp) {
	messwert_tmp = tmp->daten;
	if (messwert_tmp == NULL) {
	    /* MS2 doesn't read the channel definitions so chek if the UID belongs to GB2 and use defaults */
	    if (((uid & 0xFFF00000) == 0x47400000) && (index <= sizeof(Gleisbox_Messwerte) / sizeof(Gleisbox_Messwerte[0])))
		return &Gleisbox_Messwerte[index];
	    return NULL;
	}
	if ((messwert_tmp->uid == uid) && (messwert_tmp->index == index)) {
	    // printf("Gefunden ");
	    return tmp->daten;
	}
	tmp = tmp->next;
    }
    return NULL;
}

char *berechne_messwert(struct messwert_t *c_messwert, uint16_t wert) {
    float value;
    char *s = NULL;

    value = (c_messwert->max_bereich - c_messwert->min_bereich) / (c_messwert->max_limit - c_messwert->nullpunkt);
    value = value * (wert - c_messwert->nullpunkt) + c_messwert->min_bereich;
    if (asprintf(&s, "%0.1f %s", value, c_messwert->einheit) == -1)
	fprintf(stderr, "%s: can't alloc memory for measure", __func__);
    return s;
}

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

void decode_cs2_can_identifier(struct can_frame *frame) {
    uint32_t uid;
    uint16_t ident;

    uid = be32(frame->data);
    ident = be16(&frame->data[6]);

    switch (ident) {
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
    case 0x0030:
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
    case 0xFFF0:		/* beobachtet */
    case 0xFFFF:
	printf("CS2-GUI (Master)");
	break;
    default:
	printf("unbekannt");
	break;
    }
    printf(" UID 0x%08X, Software Version %d.%d\n", uid, frame->data[4], frame->data[5]);
}

#if 0
void print_measure_data(struct messwert_t *messwert) {
    printf("UID: 0x%08X\n", messwert->uid);
    printf("Index: %d\n", messwert->index);
    printf("Potenz: %d\n", messwert->potenz);
    printf("Farbe Bereich1: %d\n", messwert->farbe_bereich1);
    printf("Farbe Bereich2: %d\n", messwert->farbe_bereich2);
    printf("Farbe Bereich3: %d\n", messwert->farbe_bereich3);
    printf("Farbe Bereich4: %d\n", messwert->farbe_bereich4);
    printf("Nullpunkt: %d\n", messwert->nullpunkt);
    printf("Max Limit: %d\n", messwert->max_limit);
    printf("Ende Bereich1: %d\n", messwert->ende_bereich1);
    printf("Ende Bereich2: %d\n", messwert->ende_bereich2);
    printf("Ende Bereich3: %d\n", messwert->ende_bereich3);
    printf("Ende Bereich4: %d\n", messwert->ende_bereich4);
    printf("Messwert Name: %s\n", messwert->name);
    printf("Min Bereich: %f\n", messwert->min_bereich);
    printf("Max Bereich: %f\n", messwert->max_bereich);
    printf("Bezeichnung Ende: %s\n", messwert->bezeichnung_ende);
    printf("Messwert Einheit: %s\n", messwert->einheit);
}
#endif

void decode_cs2_channel_data(unsigned char *buffer, uint32_t uid, int kanal, int messwerte) {
    char *p;

    /* TODO: still not all values are kept */
    if (kanal && messwerte) {
	/* Messwert */
	a_messwert = calloc(1, sizeof(struct messwert_t));
	if (!a_messwert) {
	    fprintf(stderr, "Unable to allocate measurement buffer\n");
	    return;
	}
	a_messwert->uid = uid;
	a_messwert->index = buffer[0];
	a_messwert->potenz = buffer[1];
	a_messwert->farbe_bereich1 = buffer[2];
	a_messwert->farbe_bereich2 = buffer[3];
	a_messwert->farbe_bereich3 = buffer[4];
	a_messwert->farbe_bereich4 = buffer[5];
	a_messwert->nullpunkt     = be16(&buffer[6]);
	a_messwert->ende_bereich1 = be16(&buffer[8]);
	a_messwert->ende_bereich2 = be16(&buffer[10]);
	a_messwert->ende_bereich3 = be16(&buffer[12]);
	a_messwert->ende_bereich4 = be16(&buffer[14]);
	a_messwert->max_limit = a_messwert->ende_bereich4;
	p = (char *)&buffer[16];
	a_messwert->name = calloc(1, strlen(p) + 1);
	if (!a_messwert->name) {
	    fprintf(stderr, "Unable to allocate buffer for measurement name\n");
	    return;
	}
	strcpy(a_messwert->name, p);
	p = next_string(p);
	a_messwert->min_bereich = (float)atof(p);
	p = next_string(p);
	a_messwert->max_bereich = (float)atof(p);
	p = next_string(p);
	a_messwert->einheit = calloc(1, strlen(p) + 1);
	if (!a_messwert->einheit) {
	    fprintf(stderr, "Unable to allocate buffer for unit name\n");
	    return;
	}
	strcpy(a_messwert->einheit, p);
	insert_right(messwert_knoten, a_messwert);
	/* print_measure_data(a_messwert); */
    } else if (messwerte) {
	/* Kanal Beschreibung */
    }
}

void decode_cs2_can_channels(struct can_frame *frame) {
    uint32_t id, uid;
    uint16_t paket;
    uint8_t n_kanaele;

    paket = 0;
    uid = be32(frame->data);
    if (frame->can_dlc == 5) {
	kanal = frame->data[4];
	printf("Statusdaten: UID 0x%08X Index 0x%02X\n", uid, kanal);
	/* Datensatz ist komplett übertragen */
	if (frame->can_id & 0x00010000UL) {
	    channel_uid = be32(frame->data);
	    decode_cs2_channel_data(channel_buffer, channel_uid, kanal, messwerte);
	}
    }
    if (frame->can_dlc == 6) {
	printf("Statusdaten: UID 0x%08X Index 0x%02X Paketanzahl %d\n", uid, frame->data[4], frame->data[5]);
	/* Datensatz ist komplett übertragen */
	if (frame->can_id & 0x00010000UL) {
	    channel_uid = be32(frame->data);
	    decode_cs2_channel_data(channel_buffer, channel_uid, kanal, messwerte);
	}

    }
    if (frame->can_dlc == 8) {
	paket = (frame->can_id & 0xFCFF) - 1;
	printf("Statusdaten: Paket %d ", paket);
	if (paket == 0)
	    memset(channel_buffer, 0, sizeof(channel_buffer));
	if (paket < MAX_PAKETE)
	    memcpy(&channel_buffer[paket * 8], frame->data, 8);
	if ((kanal == 0) && (paket == 0)) {
	    messwerte = frame->data[0];
	    n_kanaele = frame->data[1];
	    id = be32(&frame->data[4]);
	    printf(" Anzahl Messwerte: %d Anzahl Kanäle: %d Gerätenummer: 0x%08x", messwerte, n_kanaele, id);
	} else
	    for (int i = 0; i < 8; i++) {
		if (isprint(frame->data[i]))
		    putchar(frame->data[i]);
		else
		    putchar(' ');
	    }
	printf("\n");
    }
}

void decode_cs2_config_data(struct can_frame *frame, int expconf) {
    char s[32];
    uint16_t crc;

    switch (frame->can_dlc) {
    case 6:
	config_data.deflated_size = be32(frame->data);
	config_data.crc = be16(&frame->data[4]);
	if (config_data.deflated_data)
	    free(config_data.deflated_data);
	config_data.deflated_data = malloc((size_t)config_data.deflated_size + 8);
	config_data.deflated_size_counter = 0;
	printf("Config Data Stream: Länge 0x%08X CRC 0x%04X\n", config_data.deflated_size, config_data.crc);
	break;
    case 7:
	config_data.deflated_size = be32(frame->data);
	config_data.crc = be16(&frame->data[4]);
	if (config_data.deflated_data)
	    free(config_data.deflated_data);
	config_data.deflated_data = malloc((size_t)config_data.deflated_size + 8);
	config_data.deflated_size_counter = 0;
	memset(config_data.name, 0, sizeof(config_data.name));
	printf("Config Data Stream: Länge 0x%08X CRC 0x%04X (unbekannt 0x%02X)\n",
	       config_data.deflated_size, config_data.crc, frame->data[6]);
	break;
    case 8:
	if ((frame->can_id & 0x01FFFF00UL) == 0x00434700) {
	    memset(s, 0, sizeof(s));
	    memcpy(s, frame->data, frame->can_dlc);
	    printf("Config Data Broadcast Trigger ID 0x%02X für Datei %s\n", (frame->can_id & 0xFF), s);
	    break;
	}
	if (config_data.deflated_size_counter < config_data.deflated_size) {
	    memcpy(config_data.deflated_data + config_data.deflated_size_counter, frame->data, 8);
	    config_data.deflated_size_counter += 8;
	}
	printf("Config Data Stream: Daten (%d/%d)\n", config_data.deflated_size_counter, config_data.deflated_size);
	if ((config_data.deflated_size_counter >= config_data.deflated_size) && config_data.deflated_data) {
	    crc = CRCCCITT(config_data.deflated_data, config_data.deflated_size_counter, 0xFFFF);
	    if (crc == config_data.crc) {
		printf(GRN "Config Data %s mit CRC 0x%04X, Länge %d, ",
		       config_data.name, config_data.crc, config_data.deflated_size);
		if (config_data.deflated_data[0] == 0) {
		    config_data.inflated_size = be32(config_data.deflated_data);
		    printf("inflated %d Bytes\n", config_data.inflated_size);
		    /* now we can inflate collected data */
		    if (expconf) {
			if (inflate_data(&config_data))
			    printf("\nFehler: Daten konnten nicht dekomprimiert werden");
			else
			    printf(RESET "%s", config_data.inflated_data);
			free(config_data.inflated_data);
		    }
		} else {
		    printf("unkomprimiert\n");
		    if (expconf) {
			config_data.deflated_data[config_data.deflated_size] = 0;
			printf(RESET "%s\n", config_data.deflated_data);
		    }
		}
	    } else {
		printf(RED "Config Data %s mit ungültigem CRC 0x%04X, erwartet 0x%04X\n",
		       config_data.name, crc, config_data.crc);
	    }
	    free(config_data.deflated_data);
	    config_data.deflated_data = NULL;
	}
	break;
    default:
	printf("Data Stream mit unerwartetem DLC %d\n", frame->can_dlc);
    }
}

void decode_cs2_s88(struct can_frame *frame) {
    uint16_t kenner, kontakt;

    kenner = be16(frame->data);
    kontakt = be16(&frame->data[2]);

    if (frame->can_id & 0x00010000) {
	if (frame->can_dlc == 8)
	    printf("S88 Event Kennung %d Kontakt %d Zustand alt %d Zustand neu %d Zeit %d",
		    kenner, kontakt, frame->data[4], frame->data[5], be16(&frame->data[6]));
	printf("\n");
    } else {
	if (frame->can_dlc == 4)
	    printf("S88 Event Kennung %d Kontakt %d", kenner, kontakt);
	else if (frame->can_dlc == 5)
	    printf("S88 Event Kennung %d Kontakt %d Parameter %d", kenner, kontakt, frame->data[4]);
	else if (frame->can_dlc == 7) {
	    printf("S88 Event Blockmodus Kennung %d Kontakt Start %d Kontakt Ende %d ", kenner, kontakt, be16(&frame->data[4]));
	    /* TODO: Parameter */
	    switch(frame->data[6]) {
	    case 0x00:
		printf("Pin zurück setzen");
		break;
	    case 0x01:
		printf("Pin lesen");
		break;
	    default:
		printf("Parameter %d", frame->data[6]);
		break;
	    }
	}
	printf("\n");
    }
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
	printf("System: Überlast UID 0x%08X Kanal %d", uid, frame->data[5]);
	break;
    case 0x0b:
	if (frame->can_dlc == 6)
	    printf("System: Statusabfrage UID 0x%08X Kanal %d", uid, frame->data[5]);
	if (frame->can_dlc == 7) {
	    printf("System: Konfiguration UID 0x%08X Kanal %d", uid, frame->data[5]);
	    if (frame->data[6])
		printf(" gültig(%d)", frame->data[6]);
	    else
		printf(" ungültig(%d)", frame->data[6]);
	}
	if (frame->can_dlc == 8) {
	    wert = be16(&frame->data[6]);
	    if (response) {
		printf("System: Statusabfrage UID 0x%08X Kanal %d Messwert", uid, frame->data[5]);
		struct messwert_t *c_messwert = suche_messwert(messwert_knoten, uid, frame->data[5]);
		if (c_messwert) {
		    char *s = berechne_messwert(c_messwert, wert);
		    if (s) {
			printf(" %s", s);
			free(s);
		    }
		} else {
		    printf(" 0x%04X", wert);
		}
	    } else {
		printf("System: Konfiguration UID 0x%08X Kanal %d Konfigurationswert 0x%04X", uid, frame->data[5], wert);
	    }
	}
	break;
    case 0x0c:
	if (frame->can_dlc == 5) {
	    printf("System: Geraetekennung UID 0x%08X", uid);
	} else {
	    wert = be16(&frame->data[5]);
	    printf("System: Geraetekennung UID 0x%08X ist %d (0x%04X)", uid, wert, wert);
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
