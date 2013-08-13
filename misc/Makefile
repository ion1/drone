dts_sources := $(wildcard *.dts)
dtbo_targets := $(dts_sources:%.dts=%.dtbo)

targets := $(dtbo_targets)

all : $(targets)

%.dtbo : %.dts
	"$$HOME"/git/dtc/dtc -I dts -O dtb -b 0 -@ -o $@ $<

.PHONY : clean
clean ::
	$(RM) $(targets)
