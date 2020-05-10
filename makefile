FASM=fasm
CC=gcc
WARNINGS=-Wall -Wextra -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wshadow -Wformat=2 -Wswitch -Wswitch-enum #-Wconversion -Wsign-conversion
CFLAGS=$(WARNINGS) -std=c99 -g -O0 -D_FILE_OFFSET_BITS=64 -Wno-address-of-packed-member -DNDEBUG -masm=intel -D_POSIX_C_SOURCE=200809L -Ilinux
CFLAGS_32=$(CFLAGS) -m32
LDFLAGS=
LDFLAGS_32=$(LDFLAGS) -m32

all: umka_shell umka_fuse umka_os umka.sym umka.prp umka.lst tags tools/mkdirrange tools/mkfilepattern covpreproc default.skn skin.skn

covpreproc: covpreproc.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

umka_shell: umka_shell.o umka.o shell.o trace.o trace_lbr.o vdisk.o vnet.o lodepng.o pci.o thread.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -static

umka_fuse: umka_fuse.o umka.o trace.o trace_lbr.o vdisk.o pci.o thread.o
	$(CC) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs`

umka_os: umka_os.o umka.o shell.o lodepng.o vdisk.o vnet.o trace.o trace_lbr.o vdisk.o pci.o thread.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -static

umka.o umka.fas: umka.asm
	INCLUDE="$(KOLIBRI)/kernel/trunk;$(KOLIBRI)/programs/develop/libraries/libcrash/trunk" $(FASM) $< umka.o -s umka.fas -m 1234567

shell.o: shell.c
	$(CC) $(CFLAGS_32) -c $<

thread.o: linux/thread.c
	$(CC) $(CFLAGS_32) -c $<

pci.o: linux/pci.c
	$(CC) $(CFLAGS_32) -c $<

lodepng.o: lodepng.c lodepng.h
	$(CC) $(CFLAGS_32) -c $<

default.skn: $(KOLIBRI)/skins/Leency/Shkvorka/default.asm
	$(FASM) $< $@

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
	$(CC) $(CFLAGS_32) -c $<

trace_lbr.o: trace_lbr.c trace_lbr.h umka.h
	$(CC) $(CFLAGS_32) -c $<

vdisk.o: vdisk.c
	$(CC) $(CFLAGS_32) -c $<

vnet.o: vnet.c
	$(CC) $(CFLAGS_32) -c $<

umka_shell.o: umka_shell.c umka.h trace.h
	$(CC) $(CFLAGS_32) -c $<

umka_fuse.o: umka_fuse.c umka.h
	$(CC) $(CFLAGS_32) `pkg-config fuse3 --cflags` -c $<

umka_os.o: umka_os.c umka.h
	$(CC) $(CFLAGS_32) -c $<

tools/mkdirrange: tools/mkdirrange.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

tools/mkfilepattern: tools/mkfilepattern.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: all clean

clean:
	rm -f *.o umka_shell umka_fuse umka_os umka.fas umka.sym umka.lst umka.prp coverage tools/mkdirrange tools/mkfilepattern

