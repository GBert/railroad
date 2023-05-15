/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#define BROADCAST_IP	1

#include <stdint.h>

struct node {
    int id;
    struct node *next;
};

struct config_data {
    int deflated_stream_size;
    int deflated_size;
    int inflated_size;
    int verbose;
    uint16_t crc;
    char *name;
    char *directory;
    char *filename;
    uint8_t *deflated_data;
    uint8_t *inflated_data;
};

void usec_sleep(int usec);
uint8_t xor(unsigned char *data, int length);
void url_encoder_rfc_tables_init(void);
char *url_encode(unsigned char *s, char *enc);
uint16_t loco_address_mapping(uint16_t uid);
uint16_t loco_address_demapping(uint16_t uid);
char *search_interface_ip(char *search, int type);
char *find_first_ip(char *list, int type);
void print_udp_frame(char *format, int length, unsigned char *udpframe);
void print_net_frame(char *format, unsigned char *udpframe, int verbose);
int time_stamp(char *timestamp);

uint16_t le16(uint8_t * u);
uint16_t be16(uint8_t * u);
uint32_t be32(uint8_t * u);
uint32_t le32(uint8_t * u);

void to_le16(uint8_t * u, uint16_t n);
void to_be16(uint8_t * u, uint16_t n);

#endif /* _UTILS_H_ */
