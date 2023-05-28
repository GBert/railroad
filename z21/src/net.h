/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#ifndef _NET_H_
#define _NET_H_

#define UDP_READING	0
#define UDP_SENDING	1

int setup_udp_socket(struct sockaddr_in *addr, char *dst_address, int port, int type);

#endif /* _NET_H_ */
