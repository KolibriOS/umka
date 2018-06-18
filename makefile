FASM=fasm
CC=gcc
CFLAGS=-Wall -Wextra -g -O0 -D_LARGEFILE64_SOURCE
CFLAGS_32=-m32 -D_FILE_OFFSET_BITS=64
LDFLAGS=
LDFLAGS_32=-m32

all: kofu kofuse tools/mkdirrange tools/mkfilepattern

kofu: kofu.o kolibri.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@

kofuse: kofuse.o kolibri.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs`

kolibri.o: kolibri.asm kolibri.h
	INCLUDE="$(KOLIBRI_TRUNK)" $(FASM) $< $@

kofu.o: kofu.c kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

kofuse.o: kofuse.c kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) `pkg-config fuse3 --cflags` -c $<

tools/mkdirrange: tools/mkdirrange.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

tools/mkfilepattern: tools/mkfilepattern.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: all clean

clean:
	rm -f *.o kofu kofuse tools/mkdirrange tools/mkfilepattern

