FASM=fasm
CC=gcc
CFLAGS=-m32 -Wall -Wextra -g -O0 -D_FILE_OFFSET_BITS=64
LDFLAGS=-m32

all: kofu kofuse

kofu: kofu.o kocdecl.o
	$(CC) $(LDFLAGS) $^ -o $@

kofuse: kofuse.o kocdecl.o
	$(CC) $(LDFLAGS) $^ -o $@ `pkg-config fuse3 --libs`

kocdecl.o: kocdecl.asm $(KERNEL_TRUNK)/fs/xfs.inc $(KERNEL_TRUNK)/fs/xfs.asm
	INCLUDE="$(KERNEL_TRUNK);$(KERNEL_TRUNK)/fs" $(FASM) $< $@

kofu.o: kofu.c kocdecl.h
	$(CC) $(CFLAGS) -c $<

kofuse.o: kofuse.c kocdecl.h
	$(CC) $(CFLAGS) `pkg-config fuse3 --cflags` -c $<

.PHONY: all clean

clean:
	rm -f *.o kofu kofuse

