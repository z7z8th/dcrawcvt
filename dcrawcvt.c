#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

#define FMT_PLANAR 0x80
#define FMT_PLANAR_UV_INTERLEAVED 0x40

#define FMT_YUYV   0
#define FMT_UYVY   1
#define FMT_RGB    4
#define FMT_YUVP   FMT_PLANAR
#define FMT_YUVPI  (FMT_PLANAR | FMT_PLANAR_UV_INTERLEAVED)
#define FMT_YUV422  FMT_YUYV
#define FMT_PAL     FMT_UYVY

int endian_fix = 0;

unsigned char * read_file(char *path, size_t *rsize)
{
    FILE *fd = fopen(path, "rb");
    fseek(fd, 0, SEEK_END);
    long size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    printf("path %s size %ld\n", path, size);
    unsigned char *buf = calloc(1, size);
    ssize_t nread = fread(buf, size, 1, fd);
    if (nread != 1) {
        fprintf(stderr, "read error %zd\n", nread);
        free(buf);
        return NULL;
    }
    *rsize = size;
    return buf;
}

void fix_endian(unsigned char *buf, int width, int height)
{
    unsigned char temp[8];
    for(int off = 0; off < width*height; off+=8) {
        memcpy(temp, buf+off, 8);
        for (int i=0; i<8; i++){
            buf[off+i] = temp[7-i];
        }
    }
}

void write_file(char *path, unsigned char *buf, size_t size)
{
    FILE *fd = fopen(path, "w+b");
    if (fwrite(buf, size, 1,fd) != 1) {
        printf("write file error %s\n", strerror(errno));
    }
}

// WARN: don not miss the () around row!!!
#define RAW(row, col)   ((float)buf[width*(row) + (col)])

void bilinear_interpolate_rgb(unsigned char *buf, int width, int height, unsigned char *rgb_buf)
{
    printf("bilinear_interpolate_rgb\n");

    for(int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            float R, G, B;
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

void bilinear_interpolate_yuyv(unsigned char *buf, int width, int height, unsigned char *yuv_buf, int ofmt)
{
    printf("bilinear_interpolate_yuyv\n");

    int planar_u_base, planar_v_base;
    if (ofmt & FMT_PLANAR) {
        planar_u_base = width * height;
        planar_v_base = planar_u_base*3/2;
    }
    
    for(int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            float sCb = 128, sCr = 128;

            float R, G, B;
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

            float Y, Cb, Cr;
            #if 1 // 
            //ITU-R BT.709
            Y = 16 + 0.183*R + 0.614*G + 0.062*B;
            Cb = 128 - 0.101*R - 0.339*G + 0.439*B;
            Cr = 128 + (0.439*R - 0.399*G - 0.040*B);
            #elif 0 // Microsoft
            Y = (0.229 * R) + (0.587 * G) + (0.114 * B) + 16;
            Cb = -(0.169 * R) - (0.331 * G) + (0.500 * B) + 128;
            Cr = (0.500 * R) - (0.419 * G) - (0.081 * B) + 128;
            #else // wikipedia, white may show as black
            Y = 16 + 0.2126*R + 0.7152*G + 0.0722*B, 255;
            Cb = 128 -0.09991*R - 0.33609*G + 0.436*B, 255;
            Cr = 128 + 0.615*R - 0.55861*G - 0.05639*B, 255;
            #endif
            // printf("off %d Y %f Cb %f Cr %f\n", off, Y, Cb, Cr);
            Y = LIM(Y, 0, 255);
            Cb = LIM(Cb, 0, 255);
            Cr = LIM(Cr, 0, 255);

            if (!(ofmt & FMT_PLANAR)) {
                int off = (width*(height-1-row) + col)*2;
                if (ofmt == FMT_YUYV) {
                    yuv_buf[off] = Y;
                    yuv_buf[off-1+!(col&1)*2] = (sCb + Cb)/2;
                    yuv_buf[off+1+!(col&1)*2] = (sCr + Cr)/2;
                } else if (ofmt == FMT_UYVY) {
                    yuv_buf[off+1] = Y;
                    yuv_buf[off-2+!(col&1)*2] = (sCb + Cb)/2;
                    yuv_buf[off+!(col&1)*2] = (sCr + Cr)/2;
                } else {
                    printf("unrecognized format 0x%x\n", ofmt);
                    exit(EXIT_FAILURE);
                }
            } else {
                int off = (width*(height-1-row) + col);
                yuv_buf[off] = Y;
                if (!(ofmt & FMT_PLANAR_UV_INTERLEAVED)) {
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
}

// TODO: char * or unsigned char*?
void bilinear_interpolate_color(int debayer_pattern, int width, int height, char *infile, char *outfile, int ofmt)
{
    size_t insize = 0;
    unsigned char *buf = read_file(infile, &insize);
    if (insize < width*height) {
        printf("input file is truncated? intput file size %ld wxh %d\n", insize, width*height);
        exit(EXIT_FAILURE);
    }
    if(endian_fix)
        fix_endian(buf, width, height);
    if (ofmt == FMT_RGB) {
        unsigned char *rgb_buf = malloc(insize * 3);
        memset(rgb_buf, 0xff, insize*3);

        bilinear_interpolate_rgb(buf, width, height, rgb_buf);
        write_file(outfile, rgb_buf, insize * 3);
    } else {
        unsigned char *yuv_buf = malloc(insize * 2);
        memset(yuv_buf, 0, insize*2);
        bilinear_interpolate_yuyv(buf, width, height, yuv_buf, ofmt);
        write_file(outfile, yuv_buf, insize * 2);
    }
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
    printf("unknown format %s, default to YUYV/YUV422\n", str);
    return FMT_YUYV;
}

void usage(char* argv[])
{
    fprintf(stderr, "Usage: %s [-r raw_bit_width] [-d debayer_pattern] -g WxH [-f output_format] [-e] input_file output_file\n"
                    "-r raw_bit_width       8/10/12\n"
                    "-d debayer_pattern     RGGB(default)|BGGR|GRBG|GBRG\n"
                    "-g WxH                 input image geometry width and height\n"
                    "-f output_format       output format: YUV422/YUYV/YUY2(default,packed), YUVP(planar), YUVPI(planar uv interleaved), UYVY/PAL, RGB(24 bits)\n"
                    "-e                     endian fix, reverse every 8 bytes",
                        argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;
    int raw_bit_width = 8;
    int debayer_pattern = 0;
    int width = 0, height = 0;
    int output_format = 0;

    while ((opt = getopt(argc, argv, "r:d:g:f:e")) != -1)
    {
        switch (opt)
        {
        case 'r':
            raw_bit_width = atoi(optarg);
            break;
        case 'd':
            debayer_pattern = 0;  // TODO: parse pattern
            break;
        case 'g':
            parse_geometry(optarg, &width, &height);
            break;
        case 'f':
            output_format = parse_format(optarg);
            printf("output format %d\n", output_format);
            break;
        case 'e':
            endian_fix = 1;
            break;
        default: /* '?' */
            usage(argv);
        }
    }

    printf("debayer pattern %d w %d h %d\n", debayer_pattern, width, height);

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

    bilinear_interpolate_color(debayer_pattern, width, height, argv[optind], argv[optind+1], output_format);

    exit(EXIT_SUCCESS);
}
