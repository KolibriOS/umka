FASM=fasm
CC=gcc
WARNINGS=-Wall -Wextra -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wshadow -Wformat=2 -Wswitch -Wswitch-enum #-Wconversion -Wsign-conversion
CFLAGS=$(WARNINGS) -g -O0 -D_FILE_OFFSET_BITS=64 -Wno-address-of-packed-member -DNDEBUG -masm=intel
CFLAGS_32=-m32
LDFLAGS=
LDFLAGS_32=-m32

all: kofu kofuse kolibri.sym kolibri.prp kolibri.lst tags tools/mkdirrange tools/mkfilepattern covpreproc

covpreproc: covpreproc.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

kofu: kofu.o kolibri.o trace.o trace_lbr.o trace_lwp.o cio.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ -static

kofuse: kofuse.o kolibri.o cio.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs`

kolibri.o kolibri.fas: kolibri.asm kolibri.h skin.skn
	INCLUDE="$(KOLIBRI)/kernel/trunk;$(KOLIBRI)/programs/develop/libraries/libcrash/trunk" $(FASM) $< kolibri.o -s kolibri.fas -m 1234567

skin.skn: $(KOLIBRI)/skins/Leency/Octo_flat/default.asm
	$(FASM) $< $@

kolibri.prp: kolibri.fas
	prepsrc kolibri.fas kolibri.prp

kolibri.sym: kolibri.fas
	symbols kolibri.fas kolibri.sym

kolibri.lst: kolibri.fas
	listing kolibri.fas kolibri.lst

tags: kolibri.sym
	fasmtags.py $<

trace.o: trace.c trace.h trace_lbr.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

trace_lbr.o: trace_lbr.c trace_lbr.h kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

trace_lwp.o: trace_lwp.c trace_lwp.h kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

cio.o: cio.c
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

kofu.o: kofu.c kolibri.h trace.h syscalls.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $< -std=c99 -D_POSIX_C_SOURCE

kofuse.o: kofuse.c kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) `pkg-config fuse3 --cflags` -c $< -std=gnu99

tools/mkdirrange: tools/mkdirrange.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

tools/mkfilepattern: tools/mkfilepattern.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: all clean

clean:
	rm -f *.o kofu kofuse kolibri.fas kolibri.sym kolibri.lst kolibri.prp coverage tools/mkdirrange tools/mkfilepattern

