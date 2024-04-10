# dcrawcvt - convert DCRAW to YUV or RGB

## usage

```sh
# dependency
apt install imagemagick yuview

make; ./dcrawcvt -g 1920x1080 -f YUYV -b RG /path/to/file.raw a.yuv; open a.yuv
# or
make clean; make -B torgb RAW_FILE=/path/to/file.raw
# or
make clean; make -B toyuyv RAW_FILE=/path/to/file.raw
```
