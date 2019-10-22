KOFU=../kofu

sf700_tests := $(addsuffix .out, $(basename $(wildcard *_700_*.t)))
sf701_tests := $(addsuffix .out, $(basename $(wildcard *_701_*.t)))
sf705_tests := $(addsuffix .out, $(basename $(wildcard *_705_*.t)))

all: f70
	@echo all test passed

f70: sf700 sf701 sf705
sf700: $(sf700_tests)
sf701: $(sf701_tests)
sf705: $(sf705_tests)

%.out: %.ref %.t
	$(KOFU) < $(word 2, $^) > $@
	cmp $@ $<
	
