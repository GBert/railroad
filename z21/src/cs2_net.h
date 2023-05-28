/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#ifndef _CS2_NET_H_
#define _CS2_NET_H_

#define MAXLINE       		256

struct in_addr find_cs2(char *broadcast_ip, int retries);

#endif /* _CS2_NET_H_ */
