#include <stdio.h>
#include <stdlib.h>

#include "decoder.h"

int main(int argc, char *argv[])
{
        if (argc < 2) {
                printf("Usage: [filepath]\n");
                return -1;
        }

        char *filename = argv[1];
        unsigned char *bytes = NULL;
        unsigned int width;
        unsigned int height;
        decode_png_file(filename, &bytes, &width, &height);
        printf("width: %u height: %u\n", width, height);

        FILE *outfile = fopen("thingy.txt", "w");
        char pixel[18];
        for (int i = 0; i < height; i++) {
                for (int j = 0; j < width * 4; j+= 4) {
                        int idx = i * width * 4 + j;
                        sprintf(pixel, "[%hhu,%hhu,%hhu,%hhu]", 
                                bytes[idx],
                                bytes[idx + 1],
                                bytes[idx + 2],
                                bytes[idx + 3]);
                        fputs(pixel, outfile);
                }
                fputs("\n", outfile);
        }
        fclose(outfile);
        free(bytes);
}
