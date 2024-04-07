


%.o: %.c
	gcc -g -O2 -c -o $@ $<

dcrawcvt: dcrawcvt.o
#	gcc -g -O2 -o $@ $<

torgb: clean dcrawcvt
	./dcrawcvt -g 1920x1080 ../1920_1080_raw8_r12_0x1fa400.bin a.rgb
	convert -depth 8 -size 1920x1080+0 rgb:a.rgb a.png
	open a.png

toyuv: clean dcrawcvt
	./dcrawcvt -g 1920x1080 ../1920_1080_raw8_r12_0x1fa400.bin a.yuv
	convert  -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 uyvy:a.yuv a.png
	open a.png

clean:
	-rm a.yuv
	-rm a.rgb
	-rm a.png
.PHONY: test
