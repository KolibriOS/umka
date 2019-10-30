KOFU=../kofu

f30_tests := $(addsuffix .out, $(basename $(wildcard ???_f30_*.t)))

f70s0_tests := $(addsuffix .out, $(basename $(wildcard ???_f70s0_*.t)))
f70s1_tests := $(addsuffix .out, $(basename $(wildcard ???_f70s1_*.t)))
f70s5_tests := $(addsuffix .out, $(basename $(wildcard ???_f70s5_*.t)))

all: f30 f70
	@echo all tests passed

f30: $(f30_tests)

f70: f70s0 f70s1 f70s5
f70s0: $(f70s0_tests)
f70s1: $(f70s1_tests)
f70s5: $(f70s5_tests)

%.out: %.ref %.t
	$(KOFU) < $(word 2, $^) > $@
	cmp $@ $<

