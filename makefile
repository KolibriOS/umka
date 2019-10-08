FASM=fasm
CC=gcc
CFLAGS=-Wall -Wextra -g -O0 -D_LARGEFILE64_SOURCE
CFLAGS_32=-m32 -D_FILE_OFFSET_BITS=64
LDFLAGS=
LDFLAGS_32=-m32

all: kofu kofuse kolibri.sym kolibri.lst tools/mkdirrange tools/mkfilepattern

kofu: kofu.o kolibri.o trace.o trace_lbr.o trace_lwp.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ -static

kofuse: kofuse.o kolibri.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs`

kolibri.o kolibri.fas: kolibri.asm kolibri.h
	INCLUDE="$(KOLIBRI_TRUNK)" $(FASM) $< $@ -s kolibri.fas

kolibri.sym: kolibri.fas
	symbols kolibri.fas kolibri.sym

kolibri.lst: kolibri.fas
	listing kolibri.fas kolibri.lst

trace.o: trace.c trace.h trace_lbr.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

trace_lbr.o: trace_lbr.c trace_lbr.h kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

trace_lwp.o: trace_lwp.c trace_lwp.h kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

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
	rm -f *.o kofu kofuse kolibri.fas kolibri.sym kolibri.lst coverage tools/mkdirrange tools/mkfilepattern

