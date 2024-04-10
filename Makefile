RAW_FILE ?= ../1920_1080_raw8_r12_0x1fa400_fixed.raw
# RAW_FILE=../1920_1080_raw8_r12_0x1fa400.raw
# RAW_FILE=../1920_1080_raw8_pg_P2_0x1fa400.raw

CFLAGS += -g -O0 -Wall

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

dcrawcvt: dcrawcvt.o
#	gcc -g -O2 -o $@ $<

ifeq ($(OS),Windows_NT)
MAGICK = magick
OPEN = start
else
OPEN = open
endif

torgb: dcrawcvt
	./dcrawcvt -g 1920x1080 -f RGB -b $(or $(BAYER),RG) $(and $(FIX_ENDIAN),-e) $(RAW_FILE) a.rgb
	$(MAGICK) convert -depth 8 -size 1920x1080+0 rgb:a.rgb a.png
	$(OPEN) a.png

touyvy: dcrawcvt
	./dcrawcvt -g 1920x1080 -f UYVY -b $(or $(BAYER),RG) $(RAW_FILE) a.yuv
	$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 pal:a.yuv a.png
	$(OPEN) a.png
topal: touyvy

toyuyv: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUYV -b $(or $(BAYER),RG) $(RAW_FILE) a.yuv
	$(OPEN) a.yuv
toyuv422:toyuyv

toyuvp: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVP -b $(or $(BAYER),RG) $(RAW_FILE) a.yuv
	$(OPEN) a.yuv

toyuvpi: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVPI -b $(or $(BAYER),RG) $(RAW_FILE) a.yuv
	$(OPEN) a.yuv

clean:
	-@rm -vf a.yuv
	-@rm -vf a.rgb
	-@rm -vf a.png
	-@rm -vf dcrawcvt
	-@rm -vf *.o

.PHONY: test
