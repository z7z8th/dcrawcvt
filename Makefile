SHELL = /bin/bash

RAW_FILE ?= ../1920_1080_raw8_r12_0x1fa400_fixed.raw
# RAW_FILE=../1920_1080_raw8_r12_0x1fa400.raw
# RAW_FILE=../1920_1080_raw8_pg_P2_0x1fa400.raw

$(warning SHELL $(SHELL))

CC = gcc
CFLAGS += -g -O0 -Wall
LDFLAGS += -lm

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

dcrawcvt: dcrawcvt.o
	gcc $< $(LDFLAGS) -o $@

ifeq ($(DEBUG),0)
CFLAGS += -O2
endif

ifeq ($(OS),Windows_NT)
MAGICK = magick
OPEN = start
else
OPEN = open
endif

torgb: dcrawcvt
	./dcrawcvt -g 1920x1080 -f RGB $(OPTIONS) $(and $(FIX_ENDIAN),-e) $(RAW_FILE) a.rgb
	$(MAGICK) convert -depth 8 -size 1920x1080+0 rgb:a.rgb a.png
	$(OPEN) a.png

tobmp: dcrawcvt
	time ./dcrawcvt -g 1920x1080 -f UYVY $(OPTIONS) $(RAW_FILE) a.yuv
	$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 pal:a.yuv a.bmp
	$(OPEN) a.bmp

touyvy: dcrawcvt
	./dcrawcvt -g 1920x1080 -f UYVY $(OPTIONS) $(RAW_FILE) a.yuv
	-$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 pal:a.yuv a.png
	-$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 960x540+0 pal:a.yuv a.png
	$(OPEN) a.png
topal: touyvy

toyuyv: dcrawcvt
	time ./dcrawcvt -g 1920x1080 -f YUYV $(OPTIONS) $(RAW_FILE) a.yuv
	$(OPEN) a.yuv
toyuv422:toyuyv

toyuvp: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVP $(OPTIONS) $(RAW_FILE) a.yuv
	$(OPEN) a.yuv

toyuvpi: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVPI $(OPTIONS) $(RAW_FILE) a.yuv
	$(OPEN) a.yuv

tomjpeg: dcrawcvt
	time ./dcrawcvt -g 1920x1080 -f MJPEG $(OPTIONS) $(RAW_FILE) a.jpeg
	$(OPEN) a.jpeg

perf: dcrawcvt
	perf record -- ./dcrawcvt -g 1920x1080 -f YUYV $(OPTIONS) $(RAW_FILE) a.yuv

gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: LDFLAGS += -lgcov --coverage
gcov: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUYV $(OPTIONS) $(RAW_FILE) a.yuv

clean:
	-@rm -vf a.rgb a.yuv a.jpeg a.jpg a.png a.bmp
	-@rm -vf dcrawcvt
	-@rm -vf *.o *.exe
	-@rm -vf perf.data*
	-@rm -vf *.gcda *.gcno *.gcov

.PHONY: test
