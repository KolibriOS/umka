FASM=fasm
CC=gcc
WARNINGS=-Wall -Wextra -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wshadow -Wformat=2 -Wswitch -Wswitch-enum #-Wconversion -Wsign-conversion
CFLAGS=$(WARNINGS) -g -O0 -D_FILE_OFFSET_BITS=64 -Wno-address-of-packed-member -DNDEBUG -masm=intel
CFLAGS_32=$(CFLAGS) -m32
LDFLAGS=
LDFLAGS_32=-m32

all: umka_shell umka_fuse umka.sym umka.prp umka.lst tags tools/mkdirrange tools/mkfilepattern covpreproc

covpreproc: covpreproc.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

umka_shell: umka_shell.o umka.o trace.o trace_lbr.o cio.o lodepng.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ -static

umka_fuse: umka_fuse.o umka.o cio.o
	$(CC) $(LDFLAGS) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs`

umka.o umka.fas: umka.asm skin.skn
	INCLUDE="$(KOLIBRI)/kernel/trunk;$(KOLIBRI)/programs/develop/libraries/libcrash/trunk" $(FASM) $< umka.o -s umka.fas -m 1234567

lodepng.o: lodepng.c lodepng.h
	$(CC) $(CFLAGS_32) -c $<

skin.skn: $(KOLIBRI)/skins/Leency/Octo_flat/default.asm
	$(FASM) $< $@

umka.prp: umka.fas
	prepsrc $< $@

umka.sym: umka.fas
	symbols $< $@

umka.lst: umka.fas
	listing $< $@

tags: umka.sym
	fasmtags.py $<

trace.o: trace.c trace.h trace_lbr.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

trace_lbr.o: trace_lbr.c trace_lbr.h kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

cio.o: cio.c
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $<

umka_shell.o: umka_shell.c kolibri.h trace.h syscalls.h
	$(CC) $(CFLAGS) $(CFLAGS_32) -c $< -std=c99 -D_POSIX_C_SOURCE=2

umka_fuse.o: umka_fuse.c kolibri.h
	$(CC) $(CFLAGS) $(CFLAGS_32) `pkg-config fuse3 --cflags` -c $< -std=gnu99

tools/mkdirrange: tools/mkdirrange.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

tools/mkfilepattern: tools/mkfilepattern.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: all clean

clean:
	rm -f *.o umka_shell umka_fuse umka.fas umka.sym umka.lst umka.prp coverage tools/mkdirrange tools/mkfilepattern

