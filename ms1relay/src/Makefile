BINS := ms1relay

BIN = $@

all: $(BINS)

CFLAGS+=-Wall -WD -pedantic -std=gnu99 -g 

ms1relay: ms1relay.o ms1rdata.o
	$(CC) $(CFLAGS) -o ms1relay ms1rdata.o ms1relay.o

clean:
	$(RM) *.o $(BINS)
