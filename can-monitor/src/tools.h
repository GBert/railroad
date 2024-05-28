/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "can-monitor.h"

uint16_t be16(uint8_t *u);
uint16_t le16(uint8_t *u);
uint32_t be32(uint8_t *u);
uint32_t le32(uint8_t *u);
int inflate_data(struct cs2_config_data_t *config_data);
void writeRed(const char *s);
void writeGreen(const char *s);

#endif /* _TOOLS_H_ */
