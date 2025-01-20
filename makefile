ifndef KOLIBRIOS
        $(error "Set KOLIBRIOS environment variable to KolibriOS root")
endif

FASM_EXE ?= fasm
FASM_FLAGS=-dUEFI=1 -dextended_primary_loader=1 -dUMKA=1 -dHOST=$(HOST) -m 2000000 -dlang=en_US

HOST ?= linux
CC ?= gcc
WARNINGS_COMMON=-Wall -Wextra \
         -Wnull-dereference -Wshadow -Wformat=2 -Wswitch -Wswitch-enum \
         -Wpedantic -Wstrict-prototypes -Wunused -Wformat-nonliteral \
         #-Wconversion -Wsign-conversion
NOWARNINGS_COMMON=-Wno-address-of-packed-member

CFLAGS_ISOCLINE_COMMON=-D__MINGW_USE_VC2005_COMPAT
ifeq (,$(findstring gcc,$(CC)))
        WARNINGS=$(WARNINGS_COMMON)
        NOWARNINGS=$(NOWARNINGS_COMMON) -Wno-missing-prototype-for-cc
        CFLAGS_ISOCLINE=$(CFLAGS_ISOCLINE_COMMON) -Wno-format-nonliteral
else ifeq (,$(findstring clang,$(CC)))
        WARNINGS=$(WARNINGS_COMMON) -Wduplicated-cond -Wduplicated-branches \
                 -Wrestrict -Wlogical-op -Wjump-misses-init
        NOWARNINGS=$(NOWARNINGS_COMMON)
        CFLAGS_ISOCLINE=$(CFLAGS_ISOCLINE_COMMON) -Wno-duplicated-branches
else
        $(error your compiler is not supported)
endif

CFLAGS=$(WARNINGS) $(NOWARNINGS) -std=c11 -g -O0 -DNDEBUG -masm=intel \
        -D_POSIX_C_SOURCE=200809L -I$(HOST) -Ideps -I. -fno-pie -D_POSIX \
        -fno-common
CFLAGS_32=$(CFLAGS) -m32 -D_FILE_OFFSET_BITS=64 -D__USE_TIME_BITS64
LDFLAGS=-no-pie
LDFLAGS_32=$(LDFLAGS) -m32
LIBS_COMMON=-lpthread

ifeq ($(HOST),linux)
        LIBS=$(LIBS_COMMON)
else ifeq ($(HOST),windows)
        LIBS=$(LIBS_COMMON) -lws2_32
else
        $(error your HOST is not supported)
endif

ifeq ($(HOST),linux)
        FASM_INCLUDE=$(KOLIBRIOS)/kernel/trunk;$(KOLIBRIOS)/programs/develop/libraries/libcrash/hash
        FASM=INCLUDE="$(FASM_INCLUDE)" $(FASM_EXE) $(FASM_FLAGS)
else ifeq ($(HOST),windows)
        FASM_INCLUDE=$(KOLIBRIOS)\kernel\trunk;$(KOLIBRIOS)\programs\develop\libraries\libcrash\hash
        FASM=set "INCLUDE=$(FASM_INCLUDE)" && $(FASM_EXE) $(FASM_FLAGS)
else
        $(error your HOST is not supported)
endif

ifeq ($(HOST),linux)
all: umka_shell umka_fuse umka_os umka_gen_devices_dat umka.sym umka.prp \
     umka.lst tags default.skn skin.skn runtests
else ifeq ($(HOST),windows)
all: umka_shell umka.sym umka.prp umka.lst default.skn skin.skn runtests
else
        $(error your HOST is not supported)
endif

.PHONY: test

test: umka_shell
	@cd test && make clean all && cd ../

umka_shell: umka_shell.o umka.o shell.o trace.o trace_lbr.o vdisk.o \
            vdisk/raw.o vdisk/qcow2.o deps/em_inflate/em_inflate.o vnet.o \
            $(HOST)/vnet/tap.o vnet/file.o vnet/null.o deps/lodepng/lodepng.o \
            $(HOST)/pci.o $(HOST)/thread.o umkaio.o umkart.o \
            deps/optparse/optparse.o deps/isocline/src/isocline.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -T umka.ld $(LIBS)

umka_fuse: umka_fuse.o umka.o trace.o trace_lbr.o vdisk.o vdisk/raw.o \
           vdisk/qcow2.o deps/em_inflate/em_inflate.o $(HOST)/pci.o \
           $(HOST)/thread.o umkaio.o
	$(CC) $(LDFLAGS_32) $^ -o $@ `pkg-config fuse3 --libs` -T umka.ld

umka_os: umka_os.o umka.o shell.o deps/lodepng/lodepng.o vdisk.o vdisk/raw.o \
         vdisk/qcow2.o deps/em_inflate/em_inflate.o vnet.o $(HOST)/vnet/tap.o \
         vnet/file.o vnet/null.o trace.o trace_lbr.o $(HOST)/pci.o \
         $(HOST)/thread.o umkaio.o umkart.o deps/isocline/src/isocline.o \
         deps/optparse/optparse.o
	$(CC) $(LDFLAGS_32) $^ `sdl2-config --libs` -o $@ -T umka.ld

umka_gen_devices_dat: umka_gen_devices_dat.o umka.o $(HOST)/pci.o \
                      $(HOST)/thread.o umkart.o
	$(CC) $(LDFLAGS_32) $^ -o $@ -T umka.ld

umka.o umka.fas: umka.asm
	$(FASM) $< umka.o -s umka.fas

shell.o: shell.c deps/lodepng/lodepng.h
	$(CC) $(CFLAGS_32) -c $<

umkaio.o: umkaio.c umkaio.h
	$(CC) $(CFLAGS_32) -D_DEFAULT_SOURCE -c $< -o $@

$(HOST)/thread.o: $(HOST)/thread.c
	$(CC) $(CFLAGS_32) -c $< -o $@

$(HOST)/pci.o: $(HOST)/pci.c
	$(CC) $(CFLAGS_32) -std=gnu11 -c $< -o $@

deps/lodepng/lodepng.o: deps/lodepng/lodepng.c deps/lodepng/lodepng.h
	$(CC) $(CFLAGS_32) -c $< -o $@ -DLODEPNG_NO_COMPILE_DECODER \
                -DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS

deps/isocline/src/isocline.o: deps/isocline/src/isocline.c \
        deps/isocline/include/isocline.h
	$(CC) $(CFLAGS_32) $(CFLAGS_ISOCLINE) -c $< -o $@ -Wno-shadow \
                -Wno-unused-function

deps/optparse/optparse.o: deps/optparse/optparse.c deps/optparse/optparse.h
	$(CC) $(CFLAGS_32) -c $< -o $@

umkart.o: umkart.c umkart.h umka.h
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

deps/em_inflate/em_inflate.o: deps/em_inflate/em_inflate.c deps/em_inflate/em_inflate.h
	$(CC) $(CFLAGS_32) -c $< -o $@ -Wno-sign-compare -Wno-unused-parameter \
                -Wno-switch-enum -Wno-unused-function

vnet.o: vnet.c vnet.h
	$(CC) $(CFLAGS_32) -c $<

$(HOST)/vnet/tap.o: $(HOST)/vnet/tap.c vnet/tap.h
	$(CC) $(CFLAGS_32) -c $< -o $@

vnet/file.o: vnet/file.c vnet/file.h
	$(CC) $(CFLAGS_32) -c $< -o $@

vnet/null.o: vnet/null.c vnet/null.h
	$(CC) $(CFLAGS_32) -c $< -o $@

umka_shell.o: umka_shell.c umka.h trace.h
	$(CC) $(CFLAGS_32) -c $<

umka_fuse.o: umka_fuse.c umka.h
	$(CC) $(CFLAGS_32) `pkg-config fuse3 --cflags` -c $<

umka_os.o: umka_os.c umka.h
	$(CC) $(CFLAGS_32) `sdl2-config --cflags` -c $<

umka_gen_devices_dat.o: umka_gen_devices_dat.c umka.h
	$(CC) $(CFLAGS_32) -c $<

runtests: runtests.o deps/optparse/optparse.o
	$(CC) $(LDFLAGS_32) -o $@ $^ $(LIBS)

runtests.o: runtests.c
	$(CC) $(CFLAGS_32) -c $< -o $@ -Wno-deprecated-declarations


.PHONY: all clean

clean:
	rm -f umka_shell umka_fuse umka_os umka_gen_devices_dat umka.fas \
          umka.sym umka.lst umka.prp umka.cov coverage *.skn colors.dtp \
          runtests
	find . -name "*.o" -delete
	find . -name "*.a" -delete
