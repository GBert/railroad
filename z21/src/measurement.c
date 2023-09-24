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
#include "measurement.h"
#include "utils.h"

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
    value = value * wert + c_messwert->min_bereich;
    if (asprintf(&s, "%0.1f %s", value, c_messwert->einheit) == -1)
	fprintf(stderr, "%s: can't alloc memory for measure", __func__);
    return s;
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
	strcpy(a_messwert->name, p);
	p = next_string(p);
	a_messwert->min_bereich = atof(p);
	p = next_string(p);
	a_messwert->max_bereich = atof(p);
	p = next_string(p);
	a_messwert->einheit = calloc(1, strlen(p) + 1);
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
	    // printf(" Anzahl Messwerte: %d Anzahl Kanäle: %d Gerätenummer: 0x%08x", messwerte, n_kanaele, id);
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
