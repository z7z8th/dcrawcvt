


%.o: %.c
	gcc -g -O2 -c -o $@ $<

dcrawcvt: dcrawcvt.o
#	gcc -g -O2 -o $@ $<

test: dcrawcvt
	./dcrawcvt -g 1920x1080 ../1920_1080_raw8_r12_0x1fa400.bin ../a.rgb

.PHONY: test
