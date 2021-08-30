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

#define XPN_SPEED       62500

int xpn_tty_send(int fd, struct termios2 *config, unsigned char *data, int length) {
    int ret;

    /* use parity mark for address */
    config->c_cflag |= PARENB | CMSPAR | PARODD;
    ioctl(fd, TCSETS2, config);
    ret = write(fd, data, 1);
    if (ret < 0) {
        fprintf(stderr, "can't write address - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (length == 1)
	return EXIT_SUCCESS;

    /* use parity space for data */
    config->c_cflag &= ~PARODD;
    ioctl(fd, TCSETS2, config);
    ret = write(fd, data + 1, length - 1);
    if (ret < 0) {
        fprintf(stderr, "can't write data - %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int xpn_tty_init(int fd, struct termios2 *config) {

    config->c_cflag |= BOTHER | CS8 | PARENB | CMSPAR | PARODD;
    config->c_ispeed = XPN_SPEED;
    config->c_ospeed = XPN_SPEED;
    if (ioctl(fd, TCSETS2, &config) < 0) {
        fprintf(stderr, "can't set speed - %s\n", strerror(errno));
        return(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
#else
void unsused(void) {}
#endif
