# dcrawcvt - convert DCRAW to YUV or RGB

## Usage

```sh
Usage: ./dcrawcvt [-r raw_bit_width] [-b bayer_pattern] -g WxH [-f output_format] [-e] input_file output_file
-r raw_bit_width   8/10/12
-b bayer_pattern   RG(default)|GB|GR|BG
-g WxH             input image geometry width and height
-f output_format   output format:
                   YUV422/YUYV/YUY2(default,packed), YUVP(planar), YUVPI(planar uv
                   interleaved), UYVY/PAL(packed), RGB(24 bits), MJPEG
-e                 endian fix, reverse every 8 bytes
-s                 super pixel, no interpolation, output geometry is W/2xH/2
-n                 no auto whitebalance
```

## Test

```sh
# dependency
apt install imagemagick yuview

make; ./dcrawcvt -g 1920x1080 -f YUYV -b RG /path/to/file.raw a.yuv; open a.yuv
# or
make clean; make -B torgb RAW_FILE=/path/to/file.raw
# or
make clean; make -B toyuyv RAW_FILE=/path/to/file.raw
```
