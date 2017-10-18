FASM=fasm
CC=gcc -m32
CFLAGS=-Wall -g3 -O0 -D_FILE_OFFSET_BITS=64

all: kofu kofuse

kofu: kofu.o kocdecl.o
	$(CC) $^ -o $@

kofuse: kofuse.o kocdecl.o
	$(CC) $^ -o $@ `pkg-config fuse3 --libs`

kocdecl.o: kocdecl.asm xfs.inc xfs.asm
	$(FASM) $< $@

kofu.o: kofu.c
	$(CC) $(CFLAGS) -c $<

kofuse.o: kofuse.c
	$(CC) $(CFLAGS) `pkg-config fuse3 --cflags` -c $<

.PHONY: all clean

clean:
	rm -f *.o kofu kofuse

