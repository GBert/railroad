/*
 * Copyright (C) 2005-2017 Darron Broad
 * All rights reserved.
 * 
 * This file is part of Pickle Microchip PIC ICSP.
 * 
 * Pickle Microchip PIC ICSP is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation. 
 * 
 * Pickle Microchip PIC ICSP is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License along
 * with Pickle Microchip PIC ICSP. If not, see http://www.gnu.org/licenses/
 */

/*
 * MCP 24 EEPROM test utility.
 *
 * It is expected that SCL and SDA are probed with an analyser for
 * debugging purposes, otherwise this utility may have little purpose.
 *
 * This utility depends on the Linux AT24 kernel module, please modprobe
 * at24 prior to running. This has been tesed on the Raspberry Pi.
 *
 * The AT24 kernel module interface uses file I/O to read and write to the
 * EEPROM. It handles the underlying EEPROM protocol transparently. However,
 * the AT24 module doesn't appear to support paged mode writes and only byte
 * mode writes have been seen.
 *
 * If STDIO is used for I/O, then reads will be buffered. The means that
 * reading a single EEPROM byte will result in multiple reads from the chip.
 */

#include "mcp24.h"

/*******************************************************************************
 *
 * Dump Bytes as Hex
 *
 ******************************************************************************/
static inline void
dump_hex(uint16_t a, uint8_t *s, int n)
{
	int i, c = 0;

	for (i = 0; i < n; ++i) {
		if (s[i] == 0xFF)
			++c;
	}
	if (c != n) {
		printf("[%04X] ", a);
		for (i = 0; i < n; ++i) {
		        printf("%02X ", s[i]);
		}
		putchar('|');
		for (i = 0; i < n; ++i) {
			if (s[i] >= ' ' && s[i] < 127)
		                putchar(s[i]);
			else
		                putchar('.');
		}
		putchar('|');
		putchar('\n');
	}
}

/*******************************************************************************
 *
 * Dump EEPROM as Hex
 *
 ******************************************************************************/
static inline void
dump_eeprom(FILE *fp)
{
	uint16_t ofs = 0;
	uint8_t buffer[BUFLEN];
	int16_t n;

	printf("DUMP\n");

	fseek(fp, 0, SEEK_SET);

	while (!feof(fp)) {
		n = fread(buffer, sizeof(uint8_t), BUFLEN, fp);
		if (n > 0) {
			dump_hex(ofs, buffer, n);
			ofs += n;
		}
	}
}

/*******************************************************************************
 *
 * Write byte to EEPROM
 *
 ******************************************************************************/
static inline void
write_byte(FILE *fp, uint32_t ofs, uint8_t byte, uint8_t count)
{
	if (count == 0) {
		printf("NOTHING TO WRITE\n");
		return;
	}

	int i;
	uint8_t buffer[STRLEN];

	for (i = 0; i < count; ++i)
		buffer[i] = byte;

#if USE_STDIO == 1
	fseek(fp, ofs, SEEK_SET);
	i = fwrite(buffer, sizeof(uint8_t), count, fp);
#else
	lseek(fileno(fp), ofs, SEEK_SET);
	i = write(fileno(fp), buffer, count);
#endif
	if (i <= 0) {
		printf("WRITE FAILURE\n");
	} else {
		printf("WROTE %d BYTES\n", i);
		dump_hex(0, buffer, i);
	}
}

/*******************************************************************************
 *
 * Read byte from EEPROM
 *
 ******************************************************************************/
static inline void
read_byte(FILE *fp, uint32_t ofs, uint8_t count)
{
	if (count == 0) {
		printf("NOTHING TO READ\n");
		return;
	}

	int i;
	uint8_t buffer[STRLEN];

#if USE_STDIO == 1
	fseek(fp, ofs, SEEK_SET);
	i = fread(buffer, sizeof(uint8_t), count, fp);
#else
	lseek(fileno(fp), ofs, SEEK_SET);
	i = read(fileno(fp), buffer, count);
#endif
	if (i <= 0) {
		printf("READ FAILURE\n");
	} else {
		printf("READ %d BYTES\n", i);
		dump_hex(0, buffer, i);
	}
}

/*******************************************************************************
 *
 * HOWTO
 *
 ******************************************************************************/
void
usage(const char *msg, int err)
{
	fprintf(stderr, "USAGE: mcp24 [OPTIONS] I2C-BUS I2C-ADDRESS\n\n");

	if (msg)
		fprintf(stderr, "%s.\n\n", msg);

	fprintf(stderr, "Options:\n"
		" -c N byte count\n"
		" -d   dump device\n"
		" -o A byte offset\n"
		" -r   read byte\n"
		" -w B write byte\n"
		"\n");

	fprintf(stderr, "Example:\n"
		" mcp24 1 0x57 -d\n"
		"      dump device on bus 1 address 0x57\n\n");

	exit(err);
}

/*******************************************************************************
 *
 * MCP 24 series EEPROM util
 *
 ******************************************************************************/
int
main(int argc, char **argv)
{
	int opt, dump = 0, read = 0, write = 0;
	uint32_t bus, address, byte = 0, count = 1, offset = 0;
	char eeprom[STRLEN];

	opterr = 0;
	while ((opt = getopt(argc, argv, "c:do:rw:")) != -1) {
		switch (opt) {
		case 'c':
			count = strtoul(optarg, NULL, 0);
			if (count == 0)
				usage("Invalid count arg.", EX_USAGE);
			break;
		case 'd':
			dump = 1;
			break;
		case 'o':
			offset = strtoul(optarg, NULL, 0);
			break;
		case 'r':
			read = 1;
			break;
		case 'w':
			write = 1;
			byte = strtoul(optarg, NULL, 0);
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 2) {
		usage("Invalid args.", EX_USAGE);
	}

	bus = strtoul(argv[0], NULL, 0);
	address = strtoul(argv[1], NULL, 0);

	if (snprintf(eeprom, STRLEN, EEPROM, bus, bus, address) < 0) {
		usage("EEPROM snprintf failed", EX_OSERR);
	}

	if (access(eeprom, F_OK) != 0) {
		usage("EEPROM not found", EX_SOFTWARE);
	}

	if (access(eeprom, R_OK | W_OK) != 0) {
		usage("EEPROM access denied", EX_SOFTWARE);
	}

	FILE *fp = fopen(eeprom, "rb+");
	if (fp == NULL) {
		usage("EEPROM open failed", EX_SOFTWARE);
	}

	printf("EEPROM FOUND AT 0x%04X\n", address);

	if (write)
		write_byte(fp, offset, byte, count);;

	if (read)
		read_byte(fp, offset, count);;

	if (dump)
		dump_eeprom(fp);

	fclose(fp);
	exit(EX_OK);
}
