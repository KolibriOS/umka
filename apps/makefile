FASM_EXE ?= fasm
FASM_FLAGS=-dUMKA=1

ifeq ($(HOST),linux)
        FASM_INCLUDE=$(KOLIBRIOS)/programs
        FASM=INCLUDE="$(FASM_INCLUDE)" $(FASM_EXE) $(FASM_FLAGS)
else ifeq ($(HOST),windows)
        FASM_INCLUDE=$(KOLIBRIOS)\programs
        FASM=set "INCLUDE=$(FASM_INCLUDE)" && $(FASM_EXE) $(FASM_FLAGS)
else
        $(error your HOST is not supported)
endif

all: board_hello board_cycle readdir loader justawindow

%: %.asm
	$(FASM) $< $@


.PHONY: all clean

clean:
	rm -f board_hello board_cycle readdir loader justawindow
