/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#ifndef _ASCII_FRAME_H_
#define _ASCII_FRAME_H_

int decode_ascii_frame(int tcp_socket, unsigned char *netframe, int length);

#endif /* _ASCII_FRAME_H_ */

