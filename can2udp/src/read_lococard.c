/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/* M*rklin smartcard (loco card) tool
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cs2-config.h"
#include "read-cs2-config.h"

#define check_free(a) \
            do { if ( a ) free(a); } while (0)

#define PREAMBLE_MM	0x0075
#define PREAMBLE_OTHER	0x00C5
#define PREAMBLE_MFX2	0x00E5
#define PREAMBLE_MFX	0x00F5

static char *I2C_DEF_PATH = "/sys/bus/i2c/devices/1-0050/eeprom";

static const char *loco_function_string[] = {
    " ",
    "Stirnbeleuchtung",
    "Innenbeleuchtung",
    "Außenlicht/Rücklicht",
    "Fernlicht",
    "Audio hören",
    "Pantograf",
    "Rauchgenerator",
    "Rangiergang",
    "Telexkupplung",
    "Horn",
    "Schaffnerpfiff",
    "Pfeife",
    "Glocke",
    "Kran seitswärts bewegen",
    "Kran hoch/runter",
    "Kran links drehen",
    "Kran neigen",
    "ABV aus"
};

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -v -f\n", prg);
    fprintf(stderr, "   Version 0.3\n\n");
    fprintf(stderr, "         -o                  lokomotive.cs2 style output\n");
    fprintf(stderr, "         -h                  this help\n");
    fprintf(stderr, "         -v                  verbose output\n\n");
}

uint16_t le16(uint8_t * u) {
    return (u[1] << 8) | u[0];
}

uint32_t le32(uint8_t * u) {
    return (u[3] << 24) | (u[2] << 16) | (u[1] << 8) | u[0];
}

void print_bitmap(unsigned char *data) {
    uint8_t i, j, mask;
    unsigned char *line;

    line = data;
    for (i = 0; i < 8; i++) {
	mask = 0x80;
	for (j = 0; j < 8; j++) {
	    if (*line & mask)
		printf("*");
	    else
		printf(".");
	    mask >>= 1;
	}
	printf("\n");
	line++;
    }
}

unsigned char *read_data(struct loco_config_t *loco_config) {
    FILE *fp;
    unsigned char *data;

    fp = fopen(loco_config->filename, "rb");
    if (fp == NULL) {
	fprintf(stderr, "%s: error fopen failed [%s]\n", __func__, loco_config->filename);
	return NULL;
    }

    fseek(fp, 0, SEEK_END);
    loco_config->eeprom_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if ((data = malloc(loco_config->eeprom_size)) == NULL) {
	fprintf(stderr, "%s: can't alloc %u bytes for data\n", __func__, loco_config->eeprom_size);
	fclose(fp);
	return NULL;
    }

    if ((fread((void *)data, 1, loco_config->eeprom_size, fp)) != loco_config->eeprom_size) {
	fprintf(stderr, "%s: error: fread failed for [%s]\n", __func__, loco_config->filename);
	fclose(fp);
	free(data);
	return NULL;
    }

    fclose(fp);
    return data;
}

int write_data(struct loco_config_t *loco_config) {
    FILE *fp;
    unsigned char *data;

    data = loco_config->bin;

    fp = fopen(loco_config->filename, "wb");
    if (fp == NULL) {
	fprintf(stderr, "%s: error fopen failed [%s]\n", __func__, loco_config->filename);
	return EXIT_FAILURE;
    }

    if ((fwrite((void *)data, 1, loco_config->eeprom_size, fp)) != loco_config->eeprom_size) {
	fprintf(stderr, "%s: error writing failed [%s]\n", __func__, loco_config->filename);
	fclose(fp);
	return EXIT_FAILURE;
    }

    fclose(fp);
    return 0;
}

char *extract_string(unsigned int *index, unsigned char *bin, unsigned int length) {
    char *s;
    s = calloc(length + 1, 1);
    if (s == NULL)
	return NULL;
    strncpy(s, (char *)&bin[*index], length);
    *index += length;
    return s;
}

int decode_sc_data(struct loco_config_t *loco_config, struct loco_data_t *loco_data) {
    unsigned int i, j, k, func, id, png_size;
    uint8_t value[4];
    unsigned char index, length;
    struct mfxAdr_t *mfxAdr;
    char *loco_name;
    char *proto_name;
    char *png_name;

    index = 0;

    /* preamble */
    length = loco_config->bin[0];
    if (length != 2) {
	printf("unknown loco card type - first byte 0x%02x should be 2", length);
	return EXIT_FAILURE;
    }
    id = le16(&loco_config->bin[1]);
    switch (id) {
    case PREAMBLE_MFX:
	printf("type mfx\n");
	break;
    case PREAMBLE_MFX2:
	printf("type mfx2\n");
	break;
    case PREAMBLE_MM:
	printf("type mm\n");
	break;
    case PREAMBLE_OTHER:
	printf("type: other\n");
	break;
    default:
	printf("unknown loco card type 0x%04x", id);
	return EXIT_FAILURE;
    }
    i = 3;

    while (i < loco_config->eeprom_size) {
	index = loco_config->bin[i++];
	length = loco_config->bin[i++];

	switch (index) {

	case 0:
	    printf("index [0x%02x @ 0x%04x] sub-index [%u]: ", index, i, length);
	    length = le16(&loco_config->bin[i]);
	    i += 2;
	    printf(" total length [%u]\n", length);
	    id = loco_config->bin[i++];
	    while ((id != 0) && (id != 255)) {
		length = loco_config->bin[i++];
		/* printf("i 0x%02x [i] 0x%02x length %u\n" , i, loco_config->bin[i], length); */
		switch (id) {
		case 0x1e:
		    loco_name = extract_string(&i, loco_config->bin, length);
		    if (loco_name == NULL)
			return EXIT_FAILURE;
		    loco_data->name = loco_name;
		    printf("    loco name: >%s<\n", loco_name);
		    break;
		case 0x1f:
		    proto_name = extract_string(&i, loco_config->bin, length);
		    if (proto_name == NULL)
			return EXIT_FAILURE;
		    printf("    proto name: >%s<\n", proto_name);
		    loco_data->type = proto_name;
		    break;
		case 0x20:
		    png_name = extract_string(&i, loco_config->bin, length);
		    if (png_name == NULL)
			return EXIT_FAILURE;
		    printf("    png name: >%s<\n", png_name);
		    loco_data->icon = png_name;
		    break;
		case 0x05:
		    png_size = length + (loco_config->bin[i++] << 8);
		    printf("    png start: 0x%04x  end: 0x%04x  size: 0x%04x  %u bytes\n",
			   i, i + png_size, png_size, png_size);
		    return EXIT_SUCCESS;
		default:
		    printf("decoding problem: 0x%02x\n", id);
		    break;
		}
		id = loco_config->bin[i++];
		if (id == 0)
		    id = loco_config->bin[i++];
	    }
	    break;
	/* Loco functions */
	case 9:
	    printf("index [0x%02x @ 0x%04x] length [%3d]: ", index, i, length);
	    func = 0;
	    printf("\n");
	    for (j = 0; j < length / 10; j++) {
		loco_data->function[func].type = loco_config->bin[i];
		loco_data->function[func].duration = loco_config->bin[i + 1];
		printf(" function %2u: ", func++);
		for (k = 0; k < 10; k++) {
		    printf(" 0x%02x", loco_config->bin[i++]);
		}
		printf("\n");
		i -= 10;
		for (k = 0; k < 10; k++) {
		    uint8_t ti = loco_config->bin[i++];
		    switch (k) {
		    case 0:
			/* TODO */
			if (ti < 20)
			    printf(" %20s 0x%02x", loco_function_string[ti & 0x0f], ti);
			break;
		    case 99:	/* TODO */
			printf("\n");
			print_bitmap(&loco_config->bin[i]);
			break;
		    default:
			break;
		    }
		}
		printf("\n");
	    }
	    break;
	/* mfx Address */
	case 12:
	    mfxAdr = (struct mfxAdr_t *)calloc(1, sizeof(struct mfxAdr_t));
	    if (!mfxAdr) {
		fprintf(stderr, "%s: error alloc mfxAdr structure memory\n", __func__);
		return EXIT_FAILURE;
	    }
	    mfxAdr->target =      le16(&loco_config->bin[i]);
	    mfxAdr->name =        le16(&loco_config->bin[i + 2]);
	    mfxAdr->speedtable =  le16(&loco_config->bin[i + 4]);
	    mfxAdr->address =     le16(&loco_config->bin[i + 6]);
	    mfxAdr->xcel =        le16(&loco_config->bin[i + 8]);
	    mfxAdr->volume =      le16(&loco_config->bin[i + 10]);
	    mfxAdr->numfunction = le16(&loco_config->bin[i + 12]);
	    mfxAdr->function =    le16(&loco_config->bin[i + 14]);
	    loco_data->mfxAdr = mfxAdr;

	    printf("index [0x%02x @ 0x%04x] length [%3d]: ", index, i, length);
	    printf("\nmfxAdr:\n");
	    printf("    target: %3u\n", le16(&loco_config->bin[i]));
	    printf("      name: %3u\n", le16(&loco_config->bin[i + 2]));
	    printf(" speetable: %3u\n", le16(&loco_config->bin[i + 4]));
	    printf("      xcel: %3u\n", le16(&loco_config->bin[i + 6]));
	    printf("    volume: %3u\n", le16(&loco_config->bin[i + 8]));
	    printf("    (addr): %3u\n", le16(&loco_config->bin[i + 10]));
	    printf("  num func: %3u\n", le16(&loco_config->bin[i + 12]));
	    printf("      func: %3u\n", le16(&loco_config->bin[i + 14]));
	    i += length;
	    break;
	/* Loco functions 2 */
	case 13:
	    printf("index [0x%02x @ 0x%04x] length [%3d]: ", index, i, length);
	    if (length == 32) {
		printf("\n");
		func = 16;
		for (j = 0; j < length / 2; j++) {
		    loco_data->function[func].type = loco_config->bin[i];
		    loco_data->function[func].duration = loco_config->bin[i + 1];
		    printf(" function %2u: %3u 0x%02x\n", func++, loco_config->bin[i], loco_config->bin[i + 1]);
		    i += 2;
		}
	    } else {
		printf(" 0x%02x\n", loco_config->bin[i]);
		i += length;
	    }
	    printf("\n");
	    break;
	default:
	    printf("index [0x%02x @ 0x%04x] length [%3d]: ", index, i, length);
	    if (length <= 4)
		memcpy(value, &(loco_config->bin[i]), length);
	    else
		memset(value, 0, 4);
	    switch (index) {
	    case 1:
		loco_data->uid = le32(value);
		printf("               uid ");
		break;
	    case 2:
		loco_data->address = le16(value);
		printf("           address ");
		break;
	    case 3:
		loco_data->acc_delay = value[0];
		printf("acceleration delay ");
		break;
	    case 4:
		loco_data->slow_down_delay = value[0];
		printf("   slow down delay ");
		break;
	    case 5:
		loco_data->vmin = value[0];
		printf("              Vmin ");
		break;
	    case 6:
		loco_data->vmax = value[0];
		printf("              Vmax ");
		break;
	    case 7:
		loco_data->tmax = le16(value);
		printf("             tacho ");
		break;
	    case 8:
		loco_data->volume = value[0];
		printf("            volume ");
		break;
	    case 10:
		loco_data->mfxuid = le32(value);
		printf("           mfx uid ");
		break;
	    case 11:
		loco_data->mfxtype = value[0];
		printf("          mfx type ");
		break;
	    default:
		printf("           unknown ");
		break;
	    }

	    for (j = 0; j < length; j++) {
		printf(" 0x%02x", loco_config->bin[i++]);
	    }
	    break;
	}
	printf("\n");
	if (index == 0)
	    break;
    }

    return 0;
}

int main(int argc, char **argv) {
    int cs2_output, opt, verbose;
    struct loco_config_t loco_config;
    struct loco_data_t loco_data;
    char *filename;

    /* defaults */

    cs2_output = 0;
    memset(&loco_config, 0, sizeof(loco_config));
    memset(&loco_data, 0, sizeof(loco_data));
    verbose = 1;

    while ((opt = getopt(argc, argv, "ovh?")) != -1) {
	switch (opt) {

	case 'o':
	    cs2_output = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'h':
	case '?':
	    print_usage(basename(argv[0]));
	    exit(EXIT_SUCCESS);

	default:
	    fprintf(stderr, "Unknown option %c\n", opt);
	    print_usage(basename(argv[0]));
	    exit(EXIT_FAILURE);
	}
    }

    if (optind < argc) {
	filename = strdup(argv[optind]);
	if (filename == NULL)
	    return EXIT_FAILURE;
    } else {
	filename = strdup(I2C_DEF_PATH);
	if (filename == NULL)
	    return EXIT_FAILURE;
    }

    loco_config.filename = filename;
    loco_config.bin = read_data(&loco_config);
    if (loco_config.bin == NULL)
	return EXIT_FAILURE;

    if (verbose)
	printf("EEPROM Size (%s) : %u\n", filename, loco_config.eeprom_size);

    if (verbose)
	decode_sc_data(&loco_config, &loco_data);

    if (cs2_output)
	print_loco(stdout, &loco_data, 0);

    check_free(filename);
    check_free(loco_data.mfxAdr);
    check_free(loco_data.icon);
    check_free(loco_data.name);
    check_free(loco_data.type);
    check_free(loco_config.bin);

    return EXIT_SUCCESS;
}
