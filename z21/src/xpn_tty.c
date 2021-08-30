/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 *
 */

#ifndef NO_XPN_TTY

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <asm/termbits.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "xpn_tty.h"

#define XPN_SPEED       62500

int xpn_tty_send(struct xpn_tty_t *xpn_tty) {

    /* use parity mark for address */
    xpn_tty->config.c_cflag |= PARENB | CMSPAR | PARODD;
    ioctl(xpn_tty->fd, TCSETS2, xpn_tty->config);
    if (write(xpn_tty->fd, xpn_tty->data, 1) < 0) {
        fprintf(stderr, "can't write address - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (xpn_tty->length == 1)
	return EXIT_SUCCESS;

    /* use parity space for data */
    xpn_tty->config.c_cflag &= ~PARODD;
    ioctl(xpn_tty->fd, TCSETS2, xpn_tty->config);
    if (write(xpn_tty->fd, xpn_tty->data + 1, xpn_tty->length - 1) < 0) {
        fprintf(stderr, "can't write data - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int xpn_tty_init(struct xpn_tty_t *xpn_tty) {

    xpn_tty->config.c_cflag |= BOTHER | CS8 | PARENB | CMSPAR | PARODD;
    xpn_tty->config.c_ispeed = XPN_SPEED;
    xpn_tty->config.c_ospeed = XPN_SPEED;
    if (ioctl(xpn_tty->fd, TCSETS2, xpn_tty->config) < 0) {
        fprintf(stderr, "can't set speed - %s\n", strerror(errno));
        return(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
#else
void unsused(void) {}
#endif
