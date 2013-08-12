dts_sources := $(wildcard *.dts)
dtbo_targets := $(dts_sources:%.dts=%.dtbo)

targets := i2c-test $(dtbo_targets)

all : $(targets)

i2c-test : i2c-test.c
	clang -std=gnu99 -Wall -Werror -g -O2 -o $@ $< -lm

%.dtbo : %.dts
	"$$HOME"/git/dtc/dtc -I dts -O dtb -b 0 -@ -o $@ $<

.PHONY : clean
clean ::
	$(RM) $(targets)
