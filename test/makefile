ifndef HOST
        $(error "Set HOST variable to linux/windows")
endif

ifndef COVERAGE
UMKA_SHELL ?= ../umka_shell
else
UMKA_SHELL ?= sudo taskset 1 ../umka_shell -c
endif

xfs_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#xfs_*.t)))
exfat_tests    := $(addsuffix .out.log, $(basename $(wildcard *\#exfat_*.t)))
s05k_tests     := $(addsuffix .out.log, $(basename $(wildcard *\#s05k_*.t)))
s4k_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#s4k_*.t)))
f30_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#f30_*.t)))
f70_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#f70_*.t)))
f70s0_tests    := $(addsuffix .out.log, $(basename $(wildcard *\#f70s0_*.t)))
f70s1_tests    := $(addsuffix .out.log, $(basename $(wildcard *\#f70s1_*.t)))
f70s5_tests    := $(addsuffix .out.log, $(basename $(wildcard *\#f70s5_*.t)))
lookup_tests   := $(addsuffix .out.log, $(basename $(wildcard *\#lookup_*.t)))
bug_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#bug_*.t)))
xattr_tests    := $(addsuffix .out.log, $(basename $(wildcard *\#xattr_*.t)))
unicode_tests  := $(addsuffix .out.log, $(basename $(wildcard *\#unicode_*.t)))
v5_tests       := $(addsuffix .out.log, $(basename $(wildcard *\#v5_*.t)))
draw_tests     := $(addsuffix .out.log, $(basename $(wildcard *\#draw_*.t)))
coverage_tests := $(addsuffix .out.log, $(basename $(wildcard *\#coverage_*.t)))
i40_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#i40_*.t)))
net_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#net_*.t)))
arp_tests      := $(addsuffix .out.log, $(basename $(wildcard *\#arp_*.t)))
acpi_tests     := $(addsuffix .out.log, $(basename $(wildcard *\#acpi_*.t)))
input_tests    := $(addsuffix .out.log, $(basename $(wildcard *\#input_*.t)))

all: xfs exfat s05k s4k f30 f70 f70s0 f70s1 f70s5 lookup bug xattr unicode v5 \
     draw coverage i40 net arp acpi input
	@echo all tests passed

xfs: $(xfs_tests)
exfat: $(exfat_tests)
s05k: $(s05k_tests)
s4k: $(s4k_tests)
f30: $(f30_tests)
f70: $(f70_tests)
f70s0: $(f70s0_tests)
f70s1: $(f70s1_tests)
f70s5: $(f70s5_tests)
lookup: $(lookup_tests)
bug: $(bug_tests)
xattr: $(xattr_tests)
unicode: $(unicode_tests)
v5: $(v5_tests)
draw: $(draw_tests)
coverage: $(coverage_tests)
i40: $(i40_tests)
net: $(net_tests)
arp: $(arp_tests)
acpi: $(acpi_tests)
input: $(input_tests)

%.out.log: %.t
	$(UMKA_SHELL) -ri $*.t -o $@
ifeq ($(HOST),linux)
	@ cmp $*.out.log $*.ref.log
	@ if [ -f "$*.ref.png" ]; then cmp $*.out.png $*.ref.png; fi
else
	@ comp /M $*.out.log $*.ref.log
#	@ if [ -f "$*.ref.png" ]; then cmp $*.out.png $*.ref.png; fi
endif

collect_coverage:
	../tools/covpreproc ../umka.lst coverage.* > ../umka.cov

clean:
	rm -f *.out.log *.out.png coverage.*

update_logs:
	for f in *ref.log; do cp $$(basename $$f .ref.log).out.log $$f; done

.PHONY: clean collect_coverage update_logs
