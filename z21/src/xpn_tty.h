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

int xpn_tty_send(int fd, struct termios2 *config, unsigned char *data, int length);
int xpn_tty_init(int fd, struct termios2 *config);

#endif /* _XPN_TTY_H_ */
