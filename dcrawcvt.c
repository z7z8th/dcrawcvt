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
    char *buf = malloc(size);
    ssize_t nread = fread(buf, size, 1, fd);
    if (nread != 1) {
        printf("read error %lld\n", nread);
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
        printf("write file error ", strerror(errno));
    }
}

#define RAW(row, col)   buf[w*row + col]

void dcraw_to_rgb(char *buf, int w, int h, char *rgb_buf)
{
    for(int row = 2; row < h-2; row++) {
        for (int col = 2; col < w-2; col++) {
            int off = (w*row + col)*3;
            rgb_buf[off] = (RAW(row,col) + RAW(row,col+2) + RAW(row+2,col) + RAW(row+2,col+2))>>2;
            rgb_buf[off+1] = (RAW(row,col) + RAW(row+1,col) + RAW(row+1,col+2) + RAW(row+2,col+1))>>2;
            rgb_buf[off+2] = (RAW(row-1,col-1) + RAW(row-1,col+1) + RAW(row+1,col-1) + RAW(row+1,col+1))>>2;
        }
    }
}

// TODO: char * or unsigned char*?
void dcraw_to_yuv(int debayer_pattern, int w, int h, char *infile, char *outfile)
{
    size_t insize = 0;
    char *buf = read_file(infile, &insize);
    char *rgb_buf = malloc(insize * 3);
    if (insize < w*h) {
        printf("input file is truncated? intput file size %ld wxh %ld\n", insize, w*h);
        exit(EXIT_FAILURE);
    }
    dcraw_to_rgb(buf, w, h, rgb_buf);
    write_file(outfile, rgb_buf, insize * 3);
    // rgb_to_yuv(rgb_buf, yuv_buf);
}

void parse_geometry(char *gstr, int *w, int *h)
{
    if(sscanf(gstr, "%dx%d", w, h) != 2) {
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
    int w = 0, h = 0;

    while ((opt = getopt(argc, argv, "d:g:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            debayer_pattern = 0;  // TODO: parse pattern
            break;
        case 'g':
            parse_geometry(optarg, &w, &h);
            break;
        default: /* '?' */
            usage(argv);
        }
    }

    printf("debayer pattern %d w %d h %d\n", debayer_pattern, w, h);

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

    dcraw_to_yuv(debayer_pattern, w, h, argv[optind], argv[optind+1]);

    exit(EXIT_SUCCESS);
}
