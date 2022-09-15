/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ascii-frame.h"
#include "cs2-config.h"
#include "uthash.h"

extern struct loco_data_t *loco_data;
extern struct loco_names_t *loco_names;

/* TODO */
char ascii_buffer[4096];

void set_loco_list(char *s) {
    struct loco_data_t *l;

    for (l = loco_data; l != NULL; l = l->hh.next) {
	strcat(s, l->name);
	strcat(s, ";");
    }
    strcat(s, "\n");
}

void set_loco_icon(char *s, char *loco_name) {
    struct loco_data_t *l;

    HASH_FIND_STR(loco_data, loco_name, l);

    if (l) {
	strcat(s, l->icon);
	strcat(s, ";");
    }
    strcat(s, "\n");
}

int decode_ascii_frame(int tcp_socket, unsigned char *netframe, int length) {

    memset(ascii_buffer, 0, sizeof(ascii_buffer));
    memcpy(ascii_buffer, netframe, length - 1);
    strcat(ascii_buffer, "ok;");

    if (!memcmp(netframe, "get loco list;", 14)) {
	set_loco_list(ascii_buffer);
	send(tcp_socket, ascii_buffer, strlen(ascii_buffer), 0);
    }

    if (!memcmp(netframe, "get loco icon name;", 18)) {
	printf("get loco icon name;\n");
	char *loco_icon_name;
	loco_icon_name = strtok((char *) netframe, ";");
	loco_icon_name = strtok(NULL, ";");
	set_loco_icon(ascii_buffer, loco_icon_name);
	send(tcp_socket, ascii_buffer, strlen(ascii_buffer), 0);
    }
    return EXIT_SUCCESS;
}
