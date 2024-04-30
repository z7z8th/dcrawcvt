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

OPT_SUFFIX = $(subst $() $(),_,$(OPT))
FILE_BASE = $(patsubst %.raw,%,$(basename $(notdir $(RAW_FILE))))-$(subst $() $(),_,$(MAKECMDGOALS))$(OPT_SUFFIX)
YUV_FILE = $(FILE_BASE).yuv

# $(warning $(FILE_BASE))

torgb: dcrawcvt
	$(eval override OPT := $(subst -s,,$(OPT)))
	./dcrawcvt -g 1920x1080 -f RGB $(OPT) $(RAW_FILE) $(FILE_BASE).rgb
	$(MAGICK) convert -depth 8 -size 1920x1080+0 rgb:$(FILE_BASE).rgb $(FILE_BASE).png
	$(OPEN) $(FILE_BASE).png

tobmp: dcrawcvt
	time ./dcrawcvt -g 1920x1080 -f UYVY $(OPT) $(RAW_FILE) $(YUV_FILE)
ifeq ($(findstring -s,$(OPT)),)
	$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 pal:$(YUV_FILE) $(FILE_BASE).bmp
else
	-$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 960x540+0 pal:$(YUV_FILE) $(FILE_BASE).bmp
endif
	$(OPEN) $(FILE_BASE).bmp

touyvy: dcrawcvt
	./dcrawcvt -g 1920x1080 -f UYVY $(OPT) $(RAW_FILE) $(YUV_FILE)
ifeq ($(findstring -s,$(OPT)),)
	-$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 1920x1080+0 pal:$(YUV_FILE) $(FILE_BASE).png
else
	-$(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size 960x540+0 pal:$(YUV_FILE) $(FILE_BASE).png
endif
	$(OPEN) $(FILE_BASE).png
topal: touyvy

toyuyv: dcrawcvt
	time ./dcrawcvt -g 1920x1080 -f YUYV $(OPT) $(RAW_FILE) $(YUV_FILE)
	$(OPEN) $(YUV_FILE)
toyuv422:toyuyv

toyuvp: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVP $(OPT) $(RAW_FILE) $(YUV_FILE)
	$(OPEN) $(YUV_FILE)

toyuvpi: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUVPI $(OPT) $(RAW_FILE) $(YUV_FILE)
	$(OPEN) $(YUV_FILE)

tomjpeg: dcrawcvt
	time ./dcrawcvt -g 1920x1080 -f MJPEG $(OPT) $(RAW_FILE) $(FILE_BASE).jpeg
	$(OPEN) $(FILE_BASE).jpeg

perf: dcrawcvt
	perf record -- ./dcrawcvt -g 1920x1080 -f YUYV $(OPT) $(RAW_FILE) $(YUV_FILE)

gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: LDFLAGS += -lgcov --coverage
gcov: dcrawcvt
	./dcrawcvt -g 1920x1080 -f YUYV $(OPT) $(RAW_FILE) $(YUV_FILE)

clean:
	-@rm -vf *.rgb *.yuv *.jpeg *.jpg *.png *.bmp
	-@rm -vf dcrawcvt
	-@rm -vf *.o *.exe *.list
	-@rm -vf perf.data*
	-@rm -vf *.gcda *.gcno *.gcov

.PHONY: test
