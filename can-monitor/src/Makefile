BINS := can-monitor

PREFIX=/usr/sbin

BIN = $@

OBJS = crc-ccitt.o lib.o decoder-can-cdb.o decoder-can-cs1.o decoder-can-cs2.o decoder-can-maecan.o decoder-z21.o tools.o can-monitor.o
all:	$(BINS)

# CFLAGS+=-Wall -Wextra -Wmaybe-uninitialized -pedantic-errors -std=gnu99 -g
CFLAGS+=-g -O2 -fstack-protector-strong -Wformat -Werror=format-security -Wall -Wextra -Wmaybe-uninitialized -std=gnu99 -pedantic-errors 

can-monitor: ${OBJS}
	$(CC) $(CFLAGS) -o can-monitor ${OBJS} -lpcap -lz

clean:
	$(RM) ${OBJS} $(BINS)
