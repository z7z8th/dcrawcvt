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
RAWFILE_BASE = $(basename $(notdir $(RAW_FILE)))
FILE_BASE = $(patsubst %.raw,%,$(RAWFILE_BASE))-$(subst $() $(),_,$(MAKECMDGOALS))$(OPT_SUFFIX)
YUV_FILE = $(FILE_BASE).yuv

GEOMETRY = $(shell echo "$(RAWFILE_BASE)" | gawk '{ \
	where = match($$0,"[0-9]{3,4}[x_][0-9]{3,4}"); \
	if (RLENGTH < 0) \
		exit; \
	geom = substr($$0, where, RLENGTH); \
	sub("_", "x", geom); \
	print geom; \
	exit; \
}')
GEOMETRY_SP = $(shell echo "$(GEOMETRY)" | awk -F x '{print $$1/2 "x" $$2/2}')
$(warning geometry $(GEOMETRY))
$(warning geometry sp $(GEOMETRY_SP))
# $(warning $(FILE_BASE))

torgb: dcrawcvt
	$(eval override OPT := $(subst -s,,$(OPT)))
	./dcrawcvt -g $(GEOMETRY) -f RGB $(OPT) $(RAW_FILE) $(FILE_BASE).rgb
	$(MAGICK) convert -depth 8 -size $(GEOMETRY)+0 rgb:$(FILE_BASE).rgb $(FILE_BASE).png
	$(OPEN) $(FILE_BASE).png

yuvtoimg:
ifeq ($(findstring -s,$(OPT)),)
	-time $(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size $(GEOMETRY)+0 pal:$(YUV_FILE) $(FILE_BASE)$(OIMG_EXT)
else
	-time $(MAGICK) convert -depth 8 -colorspace YCbCr -sampling-factor 4:2:2 -size $(GEOMETRY_SP)+0 pal:$(YUV_FILE) $(FILE_BASE)$(OIMG_EXT)
endif
	$(OPEN) $(FILE_BASE)$(OIMG_EXT)

toimg: OFMT=UYVY
toimg: toyuv
toimg: yuvtoimg

topng: OIMG_EXT=.png
topng: toimg
tobmp: OIMG_EXT=.bmp
tobmp: toimg
tojpeg: OIMG_EXT=.jpeg
tojpeg: toimg

toyuv: dcrawcvt
	time ./dcrawcvt -g $(GEOMETRY) -f $(or $(OFMT),UYVY) $(OPT) $(RAW_FILE) $(YUV_FILE)

openyuv:
	$(OPEN) $(YUV_FILE)

touyvy: OFMT=UYVY
touyvy: toyuv openyuv
topal: touyvy

toyuyv: toyuv openyuv
toyuyv: OFMT=YUYV
toyuv422:toyuyv

toyuvp: OFMT=YUVP
toyuvp: toyuv openyuv

toyuvpi: OFMT=YUVPI
toyuvpi: toyuv openyuv

tomjpeg: dcrawcvt
	time ./dcrawcvt -g $(GEOMETRY) -f MJPEG $(OPT) $(RAW_FILE) $(FILE_BASE).mjpeg
	$(OPEN) $(FILE_BASE).mjpeg

perf: dcrawcvt
	perf record -- ./dcrawcvt -g $(GEOMETRY) -f YUYV $(OPT) $(RAW_FILE) $(YUV_FILE)

gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: LDFLAGS += -lgcov --coverage
gcov: dcrawcvt
	./dcrawcvt -g $(GEOMETRY) -f YUYV $(OPT) $(RAW_FILE) $(YUV_FILE)

clean:
	-@rm -vf *.rgb *.yuv *.jpeg *.jpg *.mjpeg *.png *.bmp
	-@rm -vf dcrawcvt
	-@rm -vf *.o *.exe *.list
	-@rm -vf perf.data*
	-@rm -vf *.gcda *.gcno *.gcov

.PHONY: test
