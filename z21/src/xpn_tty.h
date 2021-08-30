/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#ifndef _XPN_TTY_H_
#define _XPN_TTY_H_

#include <asm/termbits.h>

struct xpn_tty_t {
    int fd;
    struct termios2 config;
    unsigned char data[32];
    int length;
    char interface[32];
};

int xpn_tty_send(struct xpn_tty_t *xpn_tty);
int xpn_tty_init(struct xpn_tty_t *xpn_tty);

#endif /* _XPN_TTY_H_ */
