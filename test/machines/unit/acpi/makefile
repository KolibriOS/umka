src := $(addsuffix .aml, $(basename $(wildcard *.asl)))

all: $(src)

%.aml: %.asl
	iasl -we -oa -vw 2173 $<
