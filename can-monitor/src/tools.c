/* ------------------------------------------- ---------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "zlib.h"
#include "can-monitor.h"

uint16_t be16(uint8_t *u) {
    return (u[0] << 8) | u[1];
}

uint32_t be32(uint8_t *u) {
    return (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
}

uint32_t le32(uint8_t *u) {
    return (u[3] << 24) | (u[2] << 16) | (u[1] << 8) | u[0];
}

void writeRed(const char *s) {
    printf(RED "%s", s);
    printf(RESET);
}

void writeGreen(const char *s) {
    printf(GRN "%s", s);
    printf(RESET);
}

int inflate_data(struct cs2_config_data_t *config_data) {
    int ret;
    z_stream strm;

    /* NULL terminating for printf */
    config_data->inflated_data = malloc(config_data->inflated_size + 1);
    if (!config_data->inflated_data) {
	fprintf(stderr, "Unable to allocate inflated data buffer\n");
        return -1;
    }

    config_data->inflated_data[config_data->inflated_size] = 0;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
	return ret;
    strm.avail_in = config_data->deflated_size;
    strm.avail_out = config_data->inflated_size;
    strm.next_in = config_data->deflated_data + 4;
    strm.next_out = config_data->inflated_data;
    ret = inflate(&strm, Z_NO_FLUSH);

    assert(ret != Z_STREAM_ERROR);	/* state not clobbered */
    switch (ret) {
    case Z_NEED_DICT:
	ret = Z_DATA_ERROR;
	/* falls through */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
	(void)inflateEnd(&strm);
	inflateEnd(&strm);
	return ret;
    }
    inflateEnd(&strm);
    return 0;
}
