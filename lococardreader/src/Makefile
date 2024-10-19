BINS := lococardreader

BIN = $@

LCRD_OBJS = lococardreader.o

all: $(BINS)

CFLAGS+=-Wall -pedantic -std=gnu99 -g -I/usr/include/PCSC
LFLAGS+=$(shell pkg-config --cflags libpcsclite)

lococardreader: ${LCRD_OBJS}
	$(CC) $(CFLAGS) -o lococardreader ${LCRD_OBJS} -lpcsclite

clean:
	$(RM) *.o $(BINS)
