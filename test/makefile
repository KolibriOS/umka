KOFU=../kofu

xfs_tests    := $(addsuffix .out, $(basename $(wildcard *\#xfs_*.t)))
s05k_tests   := $(addsuffix .out, $(basename $(wildcard *\#s05k_*.t)))
s4k_tests    := $(addsuffix .out, $(basename $(wildcard *\#s4k_*.t)))
f30_tests    := $(addsuffix .out, $(basename $(wildcard *\#f30_*.t)))
f70_tests    := $(addsuffix .out, $(basename $(wildcard *\#f70_*.t)))
f70s0_tests  := $(addsuffix .out, $(basename $(wildcard *\#f70s0_*.t)))
f70s1_tests  := $(addsuffix .out, $(basename $(wildcard *\#f70s1_*.t)))
f70s5_tests  := $(addsuffix .out, $(basename $(wildcard *\#f70s5_*.t)))
lookup_tests := $(addsuffix .out, $(basename $(wildcard *\#lookup_*.t)))
bug_tests    := $(addsuffix .out, $(basename $(wildcard *\#bug_*.t)))

all: xfs s05k s4k f30 f70 f70s0 f70s1 f70s5 lookup bug
	@echo all tests passed

xfs: $(xfs_tests)
s05k: $(s05k_tests)
s4k: $(s4k_tests)
f30: $(f30_tests)
f70: $(f70_tests)
f70s0: $(f70s0_tests)
f70s1: $(f70s1_tests)
f70s5: $(f70s5_tests)
lookup: $(lookup_tests)
bug: $(bug_tests)

%.out: %.ref %.t
	$(KOFU) < $(word 2, $^) > $@
	cmp $@ $<

