BINS := cs2toz21 z21emu read-cs2-config_web

BIN = $@

all: $(BINS)

CFLAGS+=-Wall -pedantic -std=gnu99 -g

read-cs2-config_web: geturl.o read-cs2-config.o read-cs2-config_web.o
	$(CC) $(CFLAGS) -o read-cs2-config_web geturl.o read-cs2-config.o read-cs2-config_web.o -lcurl

cs2toz21: read-cs2-config.o utils.o cs2toz21.o
	$(CC) $(CFLAGS) -o cs2toz21 read-cs2-config.o utils.o cs2toz21.o -lsqlite3 -lcurl -luuid

z21emu: cs2-data-functions.o geturl.o read-cs2-config.o subscribe.o utils.o xpn_tty.o z21emu.o
	$(CC) $(CFLAGS) -o z21emu cs2-data-functions.o read-cs2-config.o subscribe.o utils.o xpn_tty.o z21emu.o -lpthread


clean:
	$(RM) *.o $(BINS)
