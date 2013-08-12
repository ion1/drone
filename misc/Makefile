targets := i2c-test gps-pps-00A0.dtbo

all : $(targets)

i2c-test : i2c-test.c
	clang -std=gnu99 -Wall -Werror -g -O2 -o $@ $< -lm

gps-pps-00A0.dtbo : gps-pps-00A0.dts
	"$$HOME"/git/dtc/dtc -I dts -O dtb -b 0 -@ -o $@ $<

.PHONY : clean
clean ::
	$(RM) $(targets)
