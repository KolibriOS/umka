KOFU=../kofu

f30_tests := $(addsuffix .out, $(basename $(wildcard ???_f30_*.t)))

sf700_tests := $(addsuffix .out, $(basename $(wildcard ???_sf700_*.t)))
sf701_tests := $(addsuffix .out, $(basename $(wildcard ???_sf701_*.t)))
sf705_tests := $(addsuffix .out, $(basename $(wildcard ???_sf705_*.t)))

all: f30 f70
	@echo all test passed

f30: $(f30_tests)

f70: sf700 sf701 sf705
sf700: $(sf700_tests)
sf701: $(sf701_tests)
sf705: $(sf705_tests)

%.out: %.ref %.t
	$(KOFU) < $(word 2, $^) > $@
	cmp $@ $<
	
