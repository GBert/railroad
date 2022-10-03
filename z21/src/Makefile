BINS := cs2toz21 z21emu

BIN = $@

all: $(BINS)

CFLAGS+=-Wall -pedantic -std=gnu99 -g

cs2toz21: read-cs2-config.o cs2toz21.o
	$(CC) $(CFLAGS) -o cs2toz21 read-cs2-config.o cs2toz21.o -lsqlite3

z21emu: crc-ccitt.o cs2-data-functions.o read-cs2-config.o subscribe.o utils.o xpn_tty.o z21emu.o
	$(CC) $(CFLAGS) -o z21emu crc-ccitt.o cs2-data-functions.o read-cs2-config.o subscribe.o utils.o xpn_tty.o z21emu.o -lpthread -lz


clean:
	$(RM) *.o $(BINS)
