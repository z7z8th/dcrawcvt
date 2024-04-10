#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char * read_file(char *path, size_t *rsize)
{
    FILE *fd = fopen(path, "rb");
    fseek(fd, 0, SEEK_END);
    long size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    printf("path %s size %ld\n", path, size);
    char *buf = calloc(1, size);
    ssize_t nread = fread(buf, size, 1, fd);
    if (nread != 1) {
        fprintf(stderr, "read error %zd\n", nread);
        free(buf);
        return NULL;
    }
    *rsize = size;
    return buf;
}

void write_file(char *path, char *buf, size_t size)
{
    FILE *fd = fopen(path, "w+b");
    if (fwrite(buf, size, 1,fd) != 1) {
        printf("write file error %s\n", strerror(errno));
    }
}

#define RAW(row, col)   ((float)buf[width*row + col])

#ifdef TO_RGB
void bilinear_interpolate_rgb(char *buf, int width, int height, char *rgb_buf)
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
#endif

#ifdef TO_UYVY
void bilinear_interpolate_uyvy(unsigned char *buf, int width, int height, unsigned char *yuv_buf)
{
    printf("bilinear_interpolate_uyvy\n");

    for(int row = 2; row < height-2; row++) {
        for (int col = 2; col < width-2; col++) {
            float sCb = 128, sCr = 128;
            int off = (width*row + col)*2;
            float R, G, B;

            if (!(row & 1)&&!(col & 1)) {  // RGGB R pos
                R = (RAW(row, col) + RAW(row-2,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row,col-2))/5;
                G = (RAW(row,col-1) + RAW(row-1,col) + RAW(row,col+1) + RAW(row+1,col))>>2;
                B = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))>>2;
            }
            else if ((row & 1)&&(col & 1)) {  // B pos
                R = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))>>2;
                G = (RAW(row,col-1) + RAW(row-1,col) + RAW(row,col+1) + RAW(row+1,col))>>2;
                B = (RAW(row, col) + RAW(row-2,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row,col-2))/5;
            } else { // G pos
                G = (RAW(row, col) + RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col+1) + RAW(row+1,col-1))/5;
                if (!(row&1)) {
                    R = (RAW(row,col-1) + RAW(row,col+1))>>1;
                    B = (RAW(row-1,col) + RAW(row+1,col))>>1;
                } else {
                    B = (RAW(row,col-1) + RAW(row,col+1))>>1;
                    R = (RAW(row-1,col) + RAW(row+1,col))>>1;
                }
            }
            // printf("off %d R %f G %f B %f\n", off, R, G, B);
            // exit(0);
            float Y, Cb, Cr;
            #if 0 // 
            //ITU-R BT.709
            Y = 16 + 0.183*R + 0.614*G + 0.062*B;
            Cb = 128 - 0.101*R - 0.339*G + 0.439*B;
            Cr = 128 + (0.439*R - 0.399*G - 0.040*B);
            #elif 1 // Microsoft
             Y = (0.229 * R) + (0.587 * G) + (0.114 * B) + 16;
             Cb = -(0.169 * R) - (0.331 * G) + (0.500 * B) + 128;
             Cr = (0.500 * R) - (0.419 * G) - (0.081 * B) + 128;
            #else //wikipedia
            Y = 16 + 0.2126*R + 0.7152*G + 0.0722*B;
            Cb = 128 -0.09991*R - 0.33609*G + 0.436*B;
            Cr = 128 + 0.615*R - 0.55861*G - 0.05639*B;
            #endif
            // printf("off %d Y %f Cb %f Cr %f\n", off, Y, Cb, Cr);

            if ((off&1) == 0) {
                yuv_buf[off+1] = Y;
                yuv_buf[off] = sCb = Cb;
                yuv_buf[off+2] = sCr = Cr;
            } else {
                yuv_buf[off+1] = Y;
                yuv_buf[off-2] = sCb = (sCb + Cb)/2;
                yuv_buf[off] = sCr = (sCr + Cr)/2;
            }
            // if (off >10000)
            // exit(0);
        }
    }
}

#endif

void bilinear_interpolate_yuyv(unsigned char *buf, int width, int height, unsigned char *yuv_buf)
{
    printf("bilinear_interpolate_yuyv\n");
    

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
            #else //wikipedia
            Y = 16 + 0.2126*R + 0.7152*G + 0.0722*B;
            Cb = 128 -0.09991*R - 0.33609*G + 0.436*B;
            Cr = 128 + 0.615*R - 0.55861*G - 0.05639*B;
            #endif
            // printf("off %d Y %f Cb %f Cr %f\n", off, Y, Cb, Cr);

            int off = (width*(height-1-row) + col)*2;

            if (!(col&1)) {
                yuv_buf[off] = Y;
                yuv_buf[off+1] = sCb = Cb;
                yuv_buf[off+3] = sCr = Cr;
            } else {
                yuv_buf[off] = Y;
                yuv_buf[off-1] = (sCb + Cb)/2;
                yuv_buf[off+1] = (sCr + Cr)/2;
            }
            // if (off >10000)
            // exit(0);
        }
    }
}

// TODO: char * or unsigned char*?
void bilinear_interpolate_color(int debayer_pattern, int width, int height, char *infile, char *outfile)
{
    size_t insize = 0;
    char *buf = read_file(infile, &insize);
    if (insize < width*height) {
        printf("input file is truncated? intput file size %ld wxh %d\n", insize, width*height);
        exit(EXIT_FAILURE);
    }
#ifdef TO_RGB
    char *rgb_buf = malloc(insize * 3);
    memset(rgb_buf, 0xff, insize*3);

    bilinear_interpolate_rgb(buf, width, height, rgb_buf);
    write_file(outfile, rgb_buf, insize * 3);
#else
    char *yuv_buf = malloc(insize * 2);
    memset(yuv_buf, 0, insize*2);
#   if defined(TO_UYVY)
    bilinear_interpolate_uyvy(buf, width, height, yuv_buf);
#   else
    bilinear_interpolate_yuyv(buf, width, height, yuv_buf);
#   endif
    write_file(outfile, yuv_buf, insize * 2);
#endif
}

void parse_geometry(char *gstr, int *width, int *height)
{
    if(sscanf(gstr, "%dx%d", width, height) != 2) {
        printf("parse gemoetry failed\n");
    }
}

void usage(char* argv[])
{
    fprintf(stderr, "Usage: %s [-r raw_bit_width] [-d debayer_pattern] -g WxH [-t to_format] input_file output_file\n"
                    "-r raw_bit_width       8/10/12\n"
                    "-d debayer_pattern     RGGB(default)|BGGR|GRBG|GBRG\n"
                    "-g WxH                 input image geometry width and height\n"
                    "-t to_format           output format: YUV422, default YUV422\n"
                    "",
                        argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;
    int debayer_pattern = 0;
    int width = 0, height = 0;

    while ((opt = getopt(argc, argv, "d:g:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            debayer_pattern = 0;  // TODO: parse pattern
            break;
        case 'g':
            parse_geometry(optarg, &width, &height);
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

    bilinear_interpolate_color(debayer_pattern, width, height, argv[optind], argv[optind+1]);

    exit(EXIT_SUCCESS);
}
