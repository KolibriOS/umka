FASM=fasm

all: xfskosfuse

#xfskos: xfskos.o
#	ld *.o -o $@ -m elf_i386 -nostdlib
#	strip $@

xfskos.o: xfskos.asm xfs.inc xfs.asm
	$(FASM) $< $@

xfskosfuse: xfskosfuse.o xfskos.o
	gcc -m32 -Wall $^ -o $@ -D_FILE_OFFSET_BITS=64 -lfuse -g3 -O0

xfskosfuse.o: xfskosfuse.c
	gcc -m32 -Wall -c $< -D_FILE_OFFSET_BITS=64 -g3 -O0

.PHONY: all clean check

clean:
	rm -f *.o xfskos

check: xfskos
	echo ok

