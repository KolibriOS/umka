ifndef KOLIBRIOS
        $(error "Set KOLIBRIOS environment variable to KolibriOS root")
endif

FASM_EXE ?= fasm
FASM_FLAGS=-dUEFI=1 -dextended_primary_loader=1 -dUMKA=1 -dHOST=$(HOST) -m 2000000

HOST ?= linux
AR ?= ar
CC ?= gcc
WARNINGS_COMMON=-Wall -Wextra \
         -Wnull-dereference -Wshadow -Wformat=2 -Wswitch -Wswitch-enum \
         -Wpedantic \
         #-Wconversion -Wsign-conversion
NOWARNINGS_COMMON=-Wno-address-of-packed-member

ifneq (,$(findstring gcc,$(CC)))
        WARNINGS=$(WARNINGS_COMMON) -Wduplicated-cond -Wduplicated-branches -Wrestrict -Wlogical-op -Wjump-misses-init
        NOWARNINGS=$(NOWARNINGS_COMMON)
else ifneq (,$(findstring clang,$(CC)))
        WARNINGS=$(WARNINGS_COMMON)
        NOWARNINGS=$(NOWARNINGS_COMMON) -Wno-missing-prototype-for-cc
else
        $(error your compiler is not supported)
endif

CFLAGS=$(WARNINGS) $(NOWARNINGS) -std=c11 -g -O0 -DNDEBUG -masm=intel \
        -D_POSIX_C_SOURCE=200809L -I$(HOST) -I. -fno-pie
CFLAGS_32=$(CFLAGS) -m32 -D_FILE_OFFSET_BITS=64 -D__USE_TIME_BITS64
LDFLAGS=-no-pie
LDFLAGS_32=$(LDFLAGS) -m32

ifeq ($(HOST),linux)
        FASM_INCLUDE=$(KOLIBRIOS)/kernel/trunk;$(KOLIBRIOS)/programs/develop/libraries/libcrash/hash
        FASM=INCLUDE="$(FASM_INCLUDE)" $(FASM_EXE) $(FASM_FLAGS)
else ifeq ($(HOST),windows)
        FASM_INCLUDE=$(KOLIBRIOS)\kernel\trunk;$(KOLIBRIOS)\programs\develop\libraries\libcrash\hash
        FASM=set "INCLUDE=$(FASM_INCLUDE)" && $(FASM_EXE) $(FASM_FLAGS)
else
        $(error your OS is not supported)
endif

ifeq ($(HOST),linux)
all: umka_shell umka_fuse umka_os umka_gen_devices_dat umka.sym umka.prp \
     umka.lst tags default.skn skin.skn
else ifeq ($(HOST),windows)
all: umka_shell umka.sym umka.prp \
     umka.lst default.skn skin.skn
else
        $(error your OS is not supported)
endif

.PHONY: test

test: umka_shell
	@cd test && make clean all && cd ../

umka_shell: umka_shell.o umka.o shell.o trace.o trace_lbr.o vdisk.o \
            vdisk/raw.o vdisk/qcow2.o vdisk/miniz/miniz.a vnet.o lodepng.o \
            $(HOST)/pci.o $(HOST)/thread.o io.o $(HOST)/io_async.o util.o \
            optparse.o bestline.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -T umka.ld

umka_fuse: umka_fuse.o umka.o trace.o trace_lbr.o vdisk.o vdisk/raw.o \
           vdisk/qcow2.o vdisk/miniz/miniz.a $(HOST)/pci.o $(HOST)/thread.o \
           io.o $(HOST)/io_async.o
	$(CC) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs` -T umka.ld

umka_os: umka_os.o umka.o shell.o lodepng.o vdisk.o vdisk/raw.o vdisk/qcow2.o \
         vdisk/miniz/miniz.a vnet.o trace.o trace_lbr.o $(HOST)/pci.o \
         $(HOST)/thread.o io.o $(HOST)/io_async.o util.o bestline.o optparse.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -T umka.ld

umka_gen_devices_dat: umka_gen_devices_dat.o umka.o $(HOST)/pci.o \
                      $(HOST)/thread.o util.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -T umka.ld

umka.o umka.fas: umka.asm
	$(FASM) $< umka.o -s umka.fas

shell.o: shell.c lodepng.h
	$(CC) $(CFLAGS_32) -c $<

io.o: io.c io.h
	$(CC) $(CFLAGS_32) -D_DEFAULT_SOURCE -c $< -o $@

$(HOST)/io_async.o: $(HOST)/io_async.c $(HOST)/io_async.h
	$(CC) $(CFLAGS_32) -D_DEFAULT_SOURCE -c $< -o $@

$(HOST)/thread.o: $(HOST)/thread.c
	$(CC) $(CFLAGS_32) -c $< -o $@

$(HOST)/pci.o: $(HOST)/pci.c
	$(CC) $(CFLAGS_32) -std=gnu11 -c $< -o $@

lodepng.o: lodepng.c lodepng.h
	$(CC) $(CFLAGS_32) -c $<

bestline.o: bestline.c bestline.h
	$(CC) $(CFLAGS_32) -U_POSIX_C_SOURCE -Wno-logical-op -Wno-switch-enum -c $<

optparse.o: optparse.c optparse.h
	$(CC) $(CFLAGS_32) -c $<

util.o: util.c util.h umka.h
	$(CC) $(CFLAGS_32) -c $<

default.skn: $(KOLIBRIOS)/skins/Leency/Shkvorka/default.asm colors.dtp
	$(FASM) $< $@

colors.dtp: $(KOLIBRIOS)/skins/Leency/Shkvorka/colors.dtp.asm
	$(FASM) $< $@

skin.skn: $(KOLIBRIOS)/skins/Leency/Octo_flat/default.asm
	$(FASM) "$<" $@

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

vdisk.o: vdisk.c vdisk/raw.h vdisk/qcow2.h
	$(CC) $(CFLAGS_32) -c $<

vdisk/raw.o: vdisk/raw.c vdisk/raw.h
	$(CC) $(CFLAGS_32) -c $< -o $@

vdisk/qcow2.o: vdisk/qcow2.c vdisk/qcow2.h
	$(CC) $(CFLAGS_32) -c $< -o $@

vdisk/miniz/miniz_tinfl.o: vdisk/miniz/miniz_tinfl.c vdisk/miniz/miniz_tinfl.h
	$(CC) $(CFLAGS_32) -c $< -o $@

vdisk/miniz/miniz_tdef.o: vdisk/miniz/miniz_tdef.c vdisk/miniz/miniz_tdef.h
	$(CC) $(CFLAGS_32) -c $< -o $@

vdisk/miniz/miniz.o: vdisk/miniz/miniz.c vdisk/miniz/miniz.h
	$(CC) $(CFLAGS_32) -DMINIZ_NO_ARCHIVE_APIS -Wno-type-limits -c $< -o $@

vdisk/miniz/miniz.a: vdisk/miniz/miniz.o vdisk/miniz/miniz_tinfl.o vdisk/miniz/miniz_tdef.o
	$(AR) --target=elf32-i386 r $@ $^

vnet.o: vnet.c
	$(CC) $(CFLAGS_32) -c $<

umka_shell.o: umka_shell.c umka.h trace.h
	$(CC) $(CFLAGS_32) -c $<

umka_fuse.o: umka_fuse.c umka.h
	$(CC) $(CFLAGS_32) `pkg-config fuse3 --cflags` -c $<

umka_os.o: umka_os.c umka.h
	$(CC) $(CFLAGS_32) -c $<

umka_gen_devices_dat.o: umka_gen_devices_dat.c umka.h
	$(CC) $(CFLAGS_32) -c $<

.PHONY: all clean

clean:
	rm -f umka_shell umka_fuse umka_os umka_gen_devices_dat umka.fas \
          umka.sym umka.lst umka.prp umka.cov coverage *.skn colors.dtp
	find . -name "*.o" -delete
	find . -name "*.a" -delete
