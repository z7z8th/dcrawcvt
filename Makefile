RAW_FILE=../1920_1080_raw8_r12_0x1fa400_fixed.raw
# RAW_FILE=../1920_1080_raw8_r12_0x1fa400.raw
# RAW_FILE=../1920_1080_raw8_pg_P2_0x1fa400.raw

CFLAGS += -g -O0 -Wall

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

dcrawcvt: dcrawcvt.o
#	gcc -g -O2 -o $@ $<

torgb: dcrawcvt
	./dcrawcvt -g 1920x1080 -f RGB $(and $(FIX_ENDIAN),-e) $(RAW_FILE) a.rgb
	convert -depth 8 -size 1920x1080+0 rgb:a.rgb a.png
	open a.png

touyvy: dcrawcvt
	./dcrawcvt -g 1920x1080 -f UYVY $(RAW_FILE) a.yuv
	convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 pal:a.yuv a.png
	open a.png
topal: touyvy

toyuyv: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUYV $(RAW_FILE) a.yuv
	open a.yuv
toyuv422:toyuyv

toyuvp: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVP $(RAW_FILE) a.yuv
	open a.yuv

toyuvpi: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVPI $(RAW_FILE) a.yuv
	open a.yuv

clean:
	-@rm -vf a.yuv
	-@rm -vf a.rgb
	-@rm -vf a.png
	-@rm -vf dcrawcvt
	-@rm -vf *.o

.PHONY: test
