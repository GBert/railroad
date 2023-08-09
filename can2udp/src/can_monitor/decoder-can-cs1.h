/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 *
 */

#ifndef _DECODE_CAN_CS1_H_
#define _DECODE_CAN_CS1_H_

#include "can-monitor.h"

int check_cs1_frame(uint32_t id);
void decode_frame_cs1(struct can_frame *frame);

#endif /* _DECODE_CAN_CS1_H_ */
