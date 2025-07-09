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
#include "loco-strings.h"

#define check_free(a) \
            do { if ( a ) free(a); } while (0)

#define PREAMBLE_MM		0x0075
#define PREAMBLE_OTHER		0x00C5
#define PREAMBLE_MFX		0x00E5
#define PREAMBLE_MM2_PRG	0x00E7
#define PREAMBLE_MFX2		0x00F5
#define PREAMBLE_MFX_F32	0x0117

#define BASIC_DATA_SIZE		384
static char *I2C_DEF_PATH  = "/dev/i2card";	/* sys/bus/i2c/devices/1-0050/eeprom"; */
static char *LOCOPIC_PATH  = "/www/config/locopic.png";
static char *BCASTFILENAME = "/www/config/bclocard";
static char *BCASTTRIGGER  = "cansend can0 00434763#62636C6F63617264";

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -v FILE\n", prg);
    fprintf(stderr, "   Version 0.5\n\n");
    fprintf(stderr, "         -o                  lokomotive.cs2 style output\n");
    fprintf(stderr, "         -b                  broadcast output to clients\n");
    fprintf(stderr, "         -h                  this help\n");
    fprintf(stderr, "         -v                  verbose output\n\n");
}

uint16_t le16(uint8_t *u) {
    return (u[1] << 8) | u[0];
}

uint32_t le32(uint8_t *u) {
    return (u[3] << 24) | (u[2] << 16) | (u[1] << 8) | u[0];
}

uint32_t be32(uint8_t *u) {
    return (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
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
    long int sz = ftell(fp);
    if (sz < 0L) {
	fprintf(stderr, "%s: error ftell failed, random access not supported.\n", __func__);
	return NULL;
    }
    loco_config->eeprom_size = sz;
    fseek(fp, 0, SEEK_SET);

    if ((data = malloc(loco_config->eeprom_size)) == NULL) {
	fprintf(stderr, "%s: can't alloc %u bytes for data\n", __func__, loco_config->eeprom_size);
	fclose(fp);
	return NULL;
    }

    if (sz > BASIC_DATA_SIZE)
	sz = BASIC_DATA_SIZE; 

    if ((fread((void *)data, 1, BASIC_DATA_SIZE, fp)) != BASIC_DATA_SIZE) {
	fprintf(stderr, "%s: error: fread failed for [%s]\n", __func__, loco_config->filename);
	fclose(fp);
	free(data);
	return NULL;
    }

    loco_config->fp = fp;
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

int write_locopic(struct loco_config_t *loco_config, unsigned int png_start, unsigned int png_size) {
    FILE *fp;
    unsigned int toread = png_start + png_size - BASIC_DATA_SIZE + 1;
    unsigned char *data = loco_config->bin + BASIC_DATA_SIZE;
    if ((png_start + png_size) > loco_config->eeprom_size) {
	fprintf(stderr, "%s: error: sizes will not fit for [%s]\n", __func__, loco_config->filename);
	return EXIT_FAILURE;
    }
    printf("Still %d bytes to read.\n", toread);
    if ((fread((void *)data, 1, toread, loco_config->fp)) != toread) {
	fprintf(stderr, "%s: error: fread failed for [%s]\n", __func__, loco_config->filename);
	return EXIT_FAILURE;
    }

    data = loco_config->bin + png_start;
    printf("PNG image %d pixel wide and %d pixel high.\n", be32(data + 16), be32(data + 20));
    fp = fopen(LOCOPIC_PATH, "wb");
    if (fp == NULL) {
	fprintf(stderr, "%s: error fopen failed [%s]\n", __func__, LOCOPIC_PATH);
	return EXIT_FAILURE;
    }
    if ((fwrite((void *)data, 1, png_size, fp)) != png_size) {
	fprintf(stderr, "%s: error writing failed [%s]\n", __func__, LOCOPIC_PATH);
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

void print_value(unsigned char *data, unsigned int *i, char *st, int index, int length) {
    int j;

    printf("index [0x%02x @ 0x%04x] length [%3d]: %s", index, *i, length, st);
    for (j = 0; j < length; j++) {
	printf(" 0x%02x", data[*i + j]);
    }
    *i += length;
}

int decode_sc_data(struct loco_config_t *loco_config, struct loco_data_t *loco_data) {
    unsigned int i, j, k, func, id, png_size;
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
	printf("ID 0x%04x type: mfx\n", id);
	break;
    case PREAMBLE_MFX2:
	printf("ID 0x%04x type: mfx\n", id);
	break;
    case PREAMBLE_MM:
	printf("ID 0x%04x type: mm 8 functions\n", id);
	break;
    case PREAMBLE_MM2_PRG:
	printf("ID 0x%04x type mm2\n", id);
	break;
    case PREAMBLE_MFX_F32:
	printf("ID 0x%04x type: mfx 32 functions\n", id);
	break;
    case PREAMBLE_OTHER:
	printf("ID 0x%04x type: other\n", id);
	break;
    default:
	printf("unknown loco card type 0x%04x\n", id);
	// return EXIT_FAILURE;
    }
    i = 3;

    while (i < loco_config->eeprom_size) {
	index = loco_config->bin[i++];
	length = loco_config->bin[i++];

	switch (index) {

	case 0:
	    printf("index [0x%02x @ 0x%04x] sub-index [%u]: ", index, i, length);
	    length = le16(&loco_config->bin[i]);
	    printf(" total length [%u]\n", length);
	    i += 2;
	    for (j = 0; j < length; j++) {
		printf(" 0x%02x", loco_config->bin[i + j]);
	    }
	    printf("\n");
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
		    return write_locopic(loco_config, i, png_size);
		default:
		    printf("decoding problem: 0x%02x\n", id);
		    break;
		}
		id = loco_config->bin[i++];
		if (id == 0)
		    id = loco_config->bin[i++];
	    }
	    break;
	case 1:
	    loco_data->uid = le32(&loco_config->bin[i]);
	    print_value(loco_config->bin, &i, "               uid ", index, length);
	    break;
	case 2:
	    loco_data->address = le16(&loco_config->bin[i]);
	    print_value(loco_config->bin, &i, "           address ", index, length);
	    break;
	case 3:
	    loco_data->acc_delay = loco_config->bin[i];
	    print_value(loco_config->bin, &i, "acceleration delay ", index, length);
	    break;
	case 4:
	    loco_data->slow_down_delay = loco_config->bin[i];
	    print_value(loco_config->bin, &i, "   slow down delay ", index, length);
	    break;
	case 5:
	    loco_data->vmin = loco_config->bin[i];
	    print_value(loco_config->bin, &i, "              Vmin ", index, length);
	    break;
	case 6:
	    loco_data->vmax = loco_config->bin[i];
	    print_value(loco_config->bin, &i, "              Vmax ", index, length);
	    break;
	case 7:
	    loco_data->tmax = le16(&loco_config->bin[i]);
	    print_value(loco_config->bin, &i, "             tacho ", index, length);
	    break;
	case 8:
	    loco_data->volume = loco_config->bin[i];
	    print_value(loco_config->bin, &i, "            volume ", index, length);
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
			if ((ti & 0x7f) < 100)
			    printf(" %20s 0x%02x", loco_function_string[ti & 0x7f], ti);
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
	case 10:
	    loco_data->mfxuid = le32(&loco_config->bin[i]);
	    print_value(loco_config->bin, &i, "           mfx uid ", index, length);
	    break;
	case 11:
	    loco_data->mfxtype = loco_config->bin[i];
	    print_value(loco_config->bin, &i, "          mfx type ", index, length);
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
	    printf("mfxAdr:\n");
	    for (j = 0; j < length; j++) {
		printf(" 0x%02x", loco_config->bin[i + j]);
	    }
	    printf("\n");
	    printf("    target: %3u\n", le16(&loco_config->bin[i]));
	    printf("      name: %3u\n", le16(&loco_config->bin[i + 2]));
	    printf("speedtable: %3u\n", le16(&loco_config->bin[i + 4]));
	    printf("      xcel: %3u\n", le16(&loco_config->bin[i + 6]));
	    printf("    volume: %3u\n", le16(&loco_config->bin[i + 8]));
	    printf("      addr: %3u\n", le16(&loco_config->bin[i + 10]));
	    printf("  num func: %3u\n", le16(&loco_config->bin[i + 12]));
	    printf("      func: %3u\n", le16(&loco_config->bin[i + 14]));
	    i += length;
	    break;
	/* Loco functions 2 */
	case 13:
	    if (length == 32) {
		printf("index [0x%02x @ 0x%04x] length [%3d]: ", index, i, length);
		printf("\n");
		func = 16;
		for (j = 0; j < length / 2; j++) {
		    loco_data->function[func].type = loco_config->bin[i];
		    loco_data->function[func].duration = loco_config->bin[i + 1];
		    printf(" function %2u: %3u 0x%02x\n", func++, loco_config->bin[i], loco_config->bin[i + 1]);
		    i += 2;
		}
		printf("\n");
	    /* old card seems to store the symbol here */
	    } else if (length == 1) {
		loco_data->symbol = loco_config->bin[i];
		print_value(loco_config->bin, &i, "            symbol ", index, length);
	    } else {
		printf(" 0x%02x\n", loco_config->bin[i]);
		i += length;
	    }
	    break;
	case 14:
	case 15:
	    loco_data->symbol = loco_config->bin[i];
	    if (index == 14)
		print_value(loco_config->bin, &i, "        CS2 symbol ", index, length);
	    else
		print_value(loco_config->bin, &i, "        MS2 symbol ", index, length);
	    break;
	default:
	    print_value(loco_config->bin, &i, "           unknown ", index, length);
	    break;
	}
	printf("\n");
	if (index == 0)
	    break;
    }

    return 0;
}

int main(int argc, char **argv) {
    int cs2_output, opt, verbose, broadcast;
    struct loco_config_t loco_config;
    struct loco_data_t loco_data;
    char *filename;

    /* defaults */

    cs2_output = 0;
    memset(&loco_config, 0, sizeof(loco_config));
    memset(&loco_data, 0, sizeof(loco_data));
    verbose = 1;
    broadcast = 0;

    while ((opt = getopt(argc, argv, "ovbh?")) != -1) {
	switch (opt) {

	case 'o':
	    cs2_output = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'b':
	    broadcast = 1;
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

    decode_sc_data(&loco_config, &loco_data);
    fclose(loco_config.fp);

    if (cs2_output)
	print_loco(stdout, &loco_data, 0);

    if (broadcast) {
	FILE *bcfile = fopen(BCASTFILENAME, "w");
	if (bcfile) {
	    fprintf(bcfile, "[lokomotive]\n");
	    print_loco(bcfile, &loco_data, 0);
	    fclose(bcfile);
	    int rc = system(BCASTTRIGGER);
	    if (rc)
		printf("Error occured: system return code is %d\n", rc);
	} else {
	    fprintf(stderr, "Opening file %s failed\n", BCASTFILENAME);
	}
    }
    check_free(filename);
    check_free(loco_data.mfxAdr);
    check_free(loco_data.icon);
    check_free(loco_data.name);
    check_free(loco_data.type);
    check_free(loco_config.bin);

    return EXIT_SUCCESS;
}
