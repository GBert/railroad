/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 *
 */

#ifndef _MEASUSREMENT_
#define _MEASUSREMENT_

#define MAX_PAKETE      256

struct messwert_t {
    uint32_t uid;
    uint8_t index;
    int8_t potenz;
    uint8_t farbe_bereich1;
    uint8_t farbe_bereich2;
    uint8_t farbe_bereich3;
    uint8_t farbe_bereich4;
    uint16_t nullpunkt;
    uint16_t max_limit;
    uint16_t ende_bereich1;
    uint16_t ende_bereich2;
    uint16_t ende_bereich3;
    uint16_t ende_bereich4;
    char *name;
    float min_bereich;
    float max_bereich;
    char *bezeichnung_ende;
    char *einheit;
};

struct knoten {
    void *daten;
    struct knoten *next;
};

struct messwert_t *suche_messwert(struct knoten *liste, uint32_t uid, uint8_t index);
char *berechne_messwert(struct messwert_t *c_messwert, uint16_t wert);
void decode_cs2_can_channels(struct can_frame *frame);
void decode_cs2_can_identifier(struct can_frame *frame);
void decode_cs2_system(struct can_frame *frame);

#endif /* _MEASUSREMENT_ */
