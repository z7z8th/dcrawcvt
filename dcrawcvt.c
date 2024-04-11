#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"

#define SQR(x) ((x) * (x))
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LIM(x, min, max) MAX(min, MIN(x, max))
#define ULIM(x, y, z) ((y) < (z) ? LIM(x, y, z) : LIM(x, z, y))
#define CLIP(x) LIM((int)(x), 0, 65535)
#define SWAP(a, b) \
    {              \
        a = a + b; \
        b = a - b; \
        a = a - b; \
    }

#define FMT_YUV                       0x8000
#define FMT_YUV_PLANAR                0x4000
#define FMT_YUV_PLANAR_INTERLEAVED    0x2000

#define FMT_YUYV   (FMT_YUV | 0)
#define FMT_UYVY   (FMT_YUV | 1)
#define FMT_RGB    4
#define FMT_MJPEG  5
#define FMT_YUVP   (FMT_YUV | FMT_YUV_PLANAR)
#define FMT_YUVPI  (FMT_YUV | FMT_YUV_PLANAR | FMT_YUV_PLANAR_INTERLEAVED)
#define FMT_YUV422  FMT_YUYV
#define FMT_PAL     FMT_UYVY

// WARN: don not miss the () around row!!!
#define RAW(row, col)   ((int)raw_buf[width*(row) + (col)])

// simplified from dcraw code
// 0/1/2/3 = R/G1/B/G2, to simplify G1=G2=1
#define FILTERS_RG   0x94
#define FILTERS_GB   0x49
#define FILTERS_GR   0x61
#define FILTERS_BG   0x16

unsigned filters_map[4] = {FILTERS_RG, FILTERS_GB, FILTERS_GR, FILTERS_BG};

#define RAWC_RED    0
#define RAWC_GREEN  1
#define RAWC_BLUE   2

#define FC(row, col)  (filters >> (((((row) & 1) << 1) + ((col) & 1)) << 1) & 3)

int endian_fix = 0;

unsigned char * read_file(char *path, size_t *rsize)
{
    FILE *fd = fopen(path, "rb");
    fseek(fd, 0, SEEK_END);
    long size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    printf("path %s size %ld\n", path, size);
    unsigned char *raw_buf = calloc(1, size);
    ssize_t nread = fread(raw_buf, size, 1, fd);
    if (nread != 1) {
        fprintf(stderr, "read error %zd\n", nread);
        free(raw_buf);
        return NULL;
    }
    *rsize = size;
    return raw_buf;
}

void fix_endian(unsigned char *raw_buf, int width, int height)
{
    unsigned char temp[8];
    for(int off = 0; off < width*height; off+=8) {
        memcpy(temp, raw_buf+off, 8);
        for (int i=0; i<8; i++){
            raw_buf[off+i] = temp[7-i];
        }
    }
}

void write_file(char *path, unsigned char *raw_buf, size_t size)
{
    FILE *fd = fopen(path, "w+b");
    if (fwrite(raw_buf, size, 1,fd) != 1) {
        printf("write file error %s\n", strerror(errno));
    }
}

void bilinear_interpolate_rgb(unsigned char *raw_buf, int width, int height, unsigned char *rgb_buf)
{
    printf("bilinear_interpolate_rgb\n");

    for(int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int R, G, B;
            R = G = B = 128;

            if (row > 1 && row < height-2 && col > 1 && col < width - 2) {
                if (!(row & 1) && !(col & 1)) {  // RGGB R pos
                    R = (RAW(row, col)*2 + RAW(row-2,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row,col-2))/6;
                    G = (RAW(row,col-1) + RAW(row-1,col) + RAW(row,col+1) + RAW(row+1,col))/4;
                    B = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))/4;
                } else if ((row & 1) && (col & 1)) {  // B pos
                    R = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))/4;
                    G = (RAW(row,col-1) + RAW(row-1,col) + RAW(row,col+1) + RAW(row+1,col))/4;
                    B = (RAW(row, col)*2 + RAW(row-2,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row,col-2))/6;
                } else { // G pos
                    G = (RAW(row, col)*2 + RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col+1) + RAW(row+1,col-1))/6;
                    if (!(row&1)) {
                        R = (RAW(row,col-1) + RAW(row,col+1))/2;
                        B = (RAW(row-1,col) + RAW(row+1,col))/2;
                    } else {
                        B = (RAW(row,col-1) + RAW(row,col+1))/2;
                        R = (RAW(row-1,col) + RAW(row+1,col))/2;
                    }
                }
            } else {
                R = G = B = 128;
            }
            R = LIM(R, 0, 255);
            G = LIM(G, 0, 255);
            B = LIM(B, 0, 255);

            // if (!(row & 1))
            //     printf("[%d,%d] R %f G %f B %f\n", row, col, R, G, B);

            int off = (width*(height-1-row) + col)*3;
            // printf("off %d r %f g %f b %f\n", off, R, G, B);
            rgb_buf[off] = R;
            rgb_buf[off+1] = G;
            rgb_buf[off+2] = B;
        }
    }
}

// #define NO_BORDER_INTERPOLATE

#ifndef NO_BORDER_INTERPOLATE
void border_interpolate(unsigned char *raw_buf, int width, int height, unsigned filters, unsigned char (*hbuf)[4], unsigned char (*vbuf)[4])
{
    // top and bottom border
    int hrow = 0;
    for (int row = 0; row < height; row+=2, hrow+=2) {
        if (row == 4) {
            row = height - 3;
            hrow = 3;
        }
        // printf("row %d hrow %d\n", row, hrow);
        for (int col = 0; col < width; col+=2) {
            int base_row = row & ~1;
            int base_col = col & ~1;
            unsigned char *pix = hbuf[hrow*width + col];
            for (int i=0; i<4; i++) {
                int c = filters>>i & 3;
                pix[c] = RAW(base_row+(i>>1), base_col+(i&1));
                // printf("pix[%d] = %d\n", c, pix[c]);
            }
        }
    }
    for (hrow=0; hrow < 6; hrow++) {
        if (hrow != 1 && hrow != 4) {
            for (int col = 1; col < width; col+=2) {
                for(int c=0; c<4; c++)
                    hbuf[hrow*width + col][c] = LIM(((int)hbuf[hrow*width + col + 1][c] + (int)hbuf[hrow*width + col - 1][c])/2, 0, 255);
            }
        }
    }
    for (hrow=1; hrow < 6; hrow+=3) {
        for (int col = 0; col < width; col+=1) {
            for(int c=0; c<4; c++)
                hbuf[hrow*width + col][c] = LIM(((int)hbuf[(hrow-1)*width + col][c] + (int)hbuf[(hrow+1)*width + col][c])/2, 0, 255);
        }
    }

    // left and right border
    int hcol = 0;
    for (int col = 0; col < width; col+=2, hcol+=2) {
        if (col == 4) {
            col = width - 3;
            hcol = 3;
        }
        for (int row = 0; row < height; row+=2) {
            int base_row = row & ~1;
            int base_col = col & ~1;
            unsigned char *pix = vbuf[row + hcol*height];
            for (int i=0; i<4; i++) {
                int c = filters>>i & 3;
                pix[c] = RAW(base_row+(i>>1), base_col+(i&1));
            }
        }
    }
    for (hcol=0; hcol < 6; hcol++) {
        if (hcol != 1 && hcol != 4) {
            for (int row = 1; row < height; row+=2) {
                for(int c=0; c<4; c++)
                    vbuf[hcol*height + row][c] = LIM(((int)vbuf[hcol*height + row + 1][c] + (int)vbuf[hcol*height + row - 1][c])/2, 0, 255);
            }
        }
    }
    for (hcol=1; hcol < 6; hcol+=3) {
        for (int row = 0; row < height; row+=1) {
            for(int c=0; c<4; c++)
                vbuf[hcol*height + row][c] = LIM(((int)vbuf[(hcol-1)*height + row][c] + (int)vbuf[(hcol+1)*height + row][c])/2, 0, 255);
        }
    }
    // /border
    
}
#endif

void bilinear_interpolate_yuyv(unsigned char *raw_buf, int width, int height, unsigned filters, unsigned char *yuv_buf, int ofmt)
{
    printf("bilinear_interpolate_yuyv\n");

    int planar_u_base, planar_v_base;
    if (ofmt & FMT_YUV_PLANAR) {
        planar_u_base = width * height;
        planar_v_base = planar_u_base*3/2;
    }

#ifndef NO_BORDER_INTERPOLATE
    // gen border bayer image
    unsigned char (*hbuf)[4] = calloc(6, width*4);
    unsigned char (*vbuf)[4] = calloc(6, height*4);

    border_interpolate(raw_buf, width, height, filters, hbuf, vbuf);
#endif
    // main area
    for(int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int sCb = 128, sCr = 128;

            int R, G, B;

            if (row > 1 && row < height-2 && col > 1 && col < width - 2) {
                int fc = FC(row, col);
                if (fc == RAWC_RED) {  // RGGB R pos
                    R = ((RAW(row, col)<<2) + RAW(row-2,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row,col-2))>>3;
                    G = (RAW(row,col-1) + RAW(row-1,col) + RAW(row,col+1) + RAW(row+1,col))/4;
                    B = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))/4;
                } else if (fc == RAWC_BLUE) {  // RGGB B pos
                    R = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))/4;
                    G = (RAW(row,col-1) + RAW(row-1,col) + RAW(row,col+1) + RAW(row+1,col))/4;
                    B = ((RAW(row, col)<<2) + RAW(row-2,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row,col-2))>>3;
                } else { // RGGB G pos
                    G = ((RAW(row, col)<<2) + RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col+1) + RAW(row+1,col-1))>>3;
                    if (!(row&1)) {
                        R = (RAW(row,col-1) + RAW(row,col+1))/2;
                        B = (RAW(row-1,col) + RAW(row+1,col))/2;
                    } else {
                        B = (RAW(row,col-1) + RAW(row,col+1))/2;
                        R = (RAW(row-1,col) + RAW(row+1,col))/2;
                    }
                }
            } else {
                // border
#ifndef NO_BORDER_INTERPOLATE
                unsigned char *pix;
                if (row < 2) {
                    pix = hbuf[row*width + col];
                } else if (row >= height - 2) {
                    pix = hbuf[(row - height + 6)*width + col];
                } else if (col < 2) {
                    pix = vbuf[row + col*height];
                } else if (col >= width -  2) {
                    pix = vbuf[row + (col - width + 6)*height];
                }

                R = pix[RAWC_RED];
                G = pix[RAWC_GREEN];
                B = pix[RAWC_BLUE];
#else
                R = G = B = 128;
#endif
                // if (row < 1)
                    // printf("[%d,%d] R %f G %f B %f\n", row, col, R, G, B);
                // exit(0);
            }
            R = LIM(R, 0, 255);
            G = LIM(G, 0, 255);
            B = LIM(B, 0, 255);

            // if (!(row & 1))
            //     printf("[%d,%d] R %f G %f B %f\n", row, col, R, G, B);

            int Y, Cb, Cr;
            #if 0 // 
            //ITU-R BT.709
            Y = 16 + 0.183*R + 0.614*G + 0.062*B;
            Cb = 128 - 0.101*R - 0.339*G + 0.439*B;
            Cr = 128 + (0.439*R - 0.399*G - 0.040*B);
            #elif 1 // Microsoft
            Y = ((234 * R + 601 * G + 117 * B)>>10) + 16;
            Cb = (-173 * R - 339 * G + 512 * B + 128 * 1024) >> 10;
            Cr = (512 * R - 429 * G - 83 * B + 128 * 1024) >> 10;
            #else // wikipedia, white may show as black
            Y = 16 + 0.2126*R + 0.7152*G + 0.0722*B, 255;
            Cb = 128 -0.09991*R - 0.33609*G + 0.436*B, 255;
            Cr = 128 + 0.615*R - 0.55861*G - 0.05639*B, 255;
            #endif
            Y = LIM(Y, 0, 255);
            Cb = LIM(Cb, 0, 255);
            Cr = LIM(Cr, 0, 255);
            // if (row > 1917)
            //     printf("[%d,%d] Y %f Cb %f Cr %f\n", row, col, Y, Cb, Cr);

            if (!(ofmt & FMT_YUV_PLANAR)) {
                int off = (width*(height-1-row) + col)*2;
                if (ofmt == FMT_YUYV) {
                    yuv_buf[off] = Y;
                    if (!(col&1)) {
                        yuv_buf[off+1] = sCb = Cb;
                        yuv_buf[off+3] = sCr = Cr;
                    } else {
                        yuv_buf[off-1] = (sCb + Cb)/2;
                        yuv_buf[off+1] = (sCr + Cr)/2;
                    }
                } else if (ofmt == FMT_UYVY) {
                    yuv_buf[off+1] = Y;
                    if (!(col&1)) {
                        yuv_buf[off] = sCb = Cb;
                        yuv_buf[off+2] = sCr = Cr;
                    } else {
                        yuv_buf[off-2] = (sCb + Cb)/2;
                        yuv_buf[off] = (sCr + Cr)/2;
                    }
                } else {
                    printf("unrecognized format 0x%x\n", ofmt);
                    exit(EXIT_FAILURE);
                }
            } else {
                int off = (width*(height-1-row) + col);
                yuv_buf[off] = Y;
                if (!(ofmt & FMT_YUV_PLANAR_INTERLEAVED)) {
                    if (!(col&1)) {
                        yuv_buf[planar_u_base+off/2] = sCb = Cb;
                        yuv_buf[planar_v_base+off/2] = sCr = Cr;
                    } else {
                        yuv_buf[planar_u_base+off/2] = (sCb + Cb)/2;
                        yuv_buf[planar_v_base+off/2] = (sCr + Cr)/2;
                    }
                } else {
                    if (!(col&1)) {
                        yuv_buf[planar_u_base+off] = sCb = Cb;
                        yuv_buf[planar_u_base+off+1] = sCr = Cr;
                    } else {
                        yuv_buf[planar_u_base+off-1] = (sCb + Cb)/2;
                        yuv_buf[planar_u_base+off] = (sCr + Cr)/2;
                    }
                }
                
            }
        }
    }

#ifndef NO_BORDER_INTERPOLATE
    free(hbuf);
    free(vbuf);
#endif
}

typedef struct mybuf {
    void * buffer;
    int offset;
    size_t cap;
} mybuf;

void mybuf_alloc(mybuf *buf, size_t size)
{
    buf->buffer = malloc(size);
    if (buf->buffer)
        buf->cap = size;
    buf->offset = 0;
}

void mybuf_append(void *ctx, void *indata, int insize)
{
    mybuf *buf = (mybuf *)ctx;
    if (insize > buf->cap - buf->offset) {
        printf("mybuf not enough space to append data\n");
        return;
    }
    memcpy(buf->buffer + buf->offset, indata, insize);
    buf->offset += insize;
}

void mybuf_free(mybuf *buf, size_t size)
{
    free(buf->buffer);
    buf->buffer = NULL;
    buf->cap = buf->offset = 0;
}

void* mybuf_get(mybuf *buf) {
    return buf->buffer;
}

size_t mybuf_size(mybuf *buf) {
    return buf->offset;
}

size_t mybuf_capacity(mybuf *buf) {
    return buf->cap;
}

void bilinear_interpolate_color(int bayer_pattern, int width, int height, char *infile, char *outfile, int ofmt)
{
    size_t insize = 0;
    unsigned char *raw_buf = read_file(infile, &insize);
    if (insize < width*height) {
        printf("input file is truncated? intput file size %ld wxh %d\n", insize, width*height);
        exit(EXIT_FAILURE);
    }

    if(endian_fix)
        fix_endian(raw_buf, width, height);

    clock_t start = clock();
    if (ofmt == FMT_RGB) {
        unsigned char *rgb_buf = malloc(insize * 3);
        memset(rgb_buf, 0xff, insize*3);
        bilinear_interpolate_rgb(raw_buf, width, height, rgb_buf);
        write_file(outfile, rgb_buf, insize * 3);
        free(rgb_buf);
    } else {
        unsigned char *yuv_buf = malloc(insize * 2);
        memset(yuv_buf, 0, insize*2);
        int tmp_fmt = ofmt == FMT_MJPEG ? FMT_YUYV : ofmt;
        bilinear_interpolate_yuyv(raw_buf, width, height, filters_map[bayer_pattern], yuv_buf, tmp_fmt);
        if (ofmt != FMT_MJPEG) {
            write_file(outfile, yuv_buf, insize * 2);
        } else {
            // write_file("a.yuv", yuv_buf, insize * 2);
            // tje_encode_to_file(outfile, width, height, tmp_fmt, yuv_buf);
            struct mybuf mjpegbuf;
            mybuf_alloc(&mjpegbuf, insize);
            int result = tje_encode_with_func(mybuf_append, &mjpegbuf,
                                    3, width, height, tmp_fmt, yuv_buf);
            printf("to jpeg result %s\n", result ? "ok" : "fail");
            write_file(outfile, mybuf_get(&mjpegbuf), mybuf_size(&mjpegbuf));
        }
        
        free(yuv_buf);
    }
    clock_t end = clock() ;
    double elapsed_time = (end-start)/(double)CLOCKS_PER_SEC;
    printf("bilinear interpolate time %fs\n", elapsed_time);

    free(raw_buf);
}

int parse_bayer_pattern(char *str)
{
    if (strlen(str) < 2)
        goto err;
    if (str[0] == 'R')  // RG
        return 0;
    if (str[0] == 'B')  // BG
        return 3;
    if (str[1] == 'B')  // GB
        return 1;
    if (str[1] == 'R')  // GR
        return 2;

err:
    printf("parse bayer pattern %s error, use default\n", str);
    return 0;
}

void parse_geometry(char *gstr, int *width, int *height)
{
    if(sscanf(gstr, "%dx%d", width, height) != 2) {
        printf("parse gemoetry failed\n");
    }
}

int parse_format(char *str)
{
    if (!strcmp(str, "YUYV") || !strcmp(str, "YUV422") || !strcmp(str, "YUY2")) {
        return FMT_YUYV;
    }
    if (!strcmp(str, "YUVP")) {
        return FMT_YUVP;
    }
    if (!strcmp(str, "YUVPI")) {
        return FMT_YUVPI;
    }
    if (!strcmp(str, "UYVY") || !strcmp(str, "PAL")) {
        return FMT_UYVY;
    }
    if (!strcmp(str, "RGB")) {
        return FMT_RGB;
    }
    if (!strcmp(str, "MJPEG")) {
        return FMT_MJPEG;
    }
    printf("unknown format %s, default to YUYV/YUV422\n", str);
    return FMT_YUYV;
}

void usage(char* argv[])
{
    fprintf(stderr, "Usage: %s [-r raw_bit_width] [-b bayer_pattern] -g WxH [-f output_format] [-e] input_file output_file\n"
                    "-r raw_bit_width       8/10/12\n"
                    "-b bayer_pattern       RG(default)|GB|GR|BG\n"
                    "-g WxH                 input image geometry width and height\n"
                    "-f output_format       output format: YUV422/YUYV/YUY2(default,packed), YUVP(planar), YUVPI(planar uv interleaved), UYVY/PAL(packed), RGB(24 bits), MJPEG\n"
                    "-e                     endian fix, reverse every 8 bytes",
                    argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;
    int raw_bit_width = 8;
    int bayer_pattern = 0;
    int width = 0, height = 0;
    int output_format = 0;

    while ((opt = getopt(argc, argv, "r:b:g:f:e")) != -1)
    {
        switch (opt)
        {
        case 'r':
        #warning only 8 bit width implemented, TODO: 10, 12, 14, 16 bit
            raw_bit_width = atoi(optarg);
            break;
        case 'b':
            bayer_pattern = parse_bayer_pattern(optarg);
            break;
        case 'g':
            parse_geometry(optarg, &width, &height);
            break;
        case 'f':
            output_format = parse_format(optarg);
            break;
        case 'e':
            endian_fix = 1;
            break;
        default: /* '?' */
            usage(argv);
        }
    }

    printf("debayer pattern %d w\n", bayer_pattern);
    printf("w %d h %d\n", width, height);
    printf("output format %d\n", output_format);

    if (optind >= argc)
    {
        fprintf(stderr, "Expected input file after options\n");
        exit(EXIT_FAILURE);
    }
    printf("input file = %s\n", argv[optind]);

    if (optind + 1 >= argc)
    {
        fprintf(stderr, "Expected output file after options\n");
        exit(EXIT_FAILURE);
    }
    printf("output file = %s\n", argv[optind+1]);

    bilinear_interpolate_color(bayer_pattern, width, height, argv[optind], argv[optind+1], output_format);

    exit(EXIT_SUCCESS);
}
