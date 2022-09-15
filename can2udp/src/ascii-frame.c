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

int decode_ascii_frame(int tcp_socket, unsigned char *netframe, int length) {
    printf("got ASCII frame\n");

    send(tcp_socket, netframe, length, 0);

    return EXIT_SUCCESS;
}


