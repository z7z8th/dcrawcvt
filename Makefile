RAW_FILE=../1920_1080_raw8_r12_0x1fa400_fixed.raw
# RAW_FILE=../1920_1080_raw8_r12_0x1fa400.raw
# RAW_FILE=../1920_1080_raw8_pg_P2_0x1fa400.raw

CFLAGS += -g -O0 -Wall

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

dcrawcvt: dcrawcvt.o
#	gcc -g -O2 -o $@ $<

torgb: CFLAGS+=-DTO_RGB
torgb: dcrawcvt
	./dcrawcvt -g 1920x1080 $(RAW_FILE) a.rgb
	convert -depth 8 -size 1920x1080+0 rgb:a.rgb a.png
	open a.png

toyuv: dcrawcvt
	./dcrawcvt -g 1920x1080 $(RAW_FILE) a.yuv
	#convert -depth 8 -colorspace YUV -sampling-factor 4:2:2 -size 1920x1080+0 pal:a.yuv a.png
	open a.yuv

clean:
	-@rm -vf a.yuv
	-@rm -vf a.rgb
	-@rm -vf a.png
	-@rm -vf dcrawcvt
	-@rm -vf *.o
.PHONY: test
