/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <stdint.h>

uint16_t be16(uint8_t *u) {
    return (u[0] << 8) | u[1];
}

uint32_t be32(uint8_t *u) {
    return (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
}

uint32_t le32(uint8_t *u) {
    return (u[3] << 24) | (u[2] << 16) | (u[1] << 8) | u[0];
}

