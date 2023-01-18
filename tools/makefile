CC=gcc
WARNINGS=-Wall -Wextra -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
         -Wrestrict -Wnull-dereference -Wjump-misses-init -Wshadow -Wformat=2 \
         -Wswitch -Wswitch-enum -Wpedantic
NOWARNINGS=-Wno-address-of-packed-member
CFLAGS=$(WARNINGS) $(NOWARNINGS) -std=c11 -O2 \
       -DNDEBUG -D_POSIX_C_SOURCE=200809L -fno-pie
LDFLAGS=-no-pie

all: mkdirrange mkfilepattern lfbviewx randdir covpreproc mkdoubledirs \
     gensamehash mksamehash lfbview

gensamehash: gensamehash.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

mkdoubledirs: mkdoubledirs.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

mksamehash: mksamehash.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

mkdirrange: mkdirrange.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

mkfilepattern: mkfilepattern.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

lfbviewx: lfbviewx.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ -lX11 -lXext -D_GNU_SOURCE

lfbview: lfbview.c
	$(CC) `sdl2-config --cflags --libs` -o $@ $< -D_GNU_SOURCE
randdir: randdir.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

covpreproc: covpreproc.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

.PHONY: all clean

clean:
	rm -f *.o mkdirrange mkfilepattern lfbviewx randdir covpreproc \
          mkdoubledirs gensamehash mksamehash lfbview
