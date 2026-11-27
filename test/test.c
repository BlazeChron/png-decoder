#include <stdio.h>

#include "../decoder.h"

// TODO figure out what these do
//#define STBI_MALLOC
//#define STBI_REALLOC
//#define STBI_FREE
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"


int main(int argc, char *argv[])
{
    if (argc < 2) {
            printf("Usage: [filepath]\n");
            return -1;
    }

    char *filename = argv[1];
    int width, height, n;

    // own api
    unsigned char *bytes = NULL;
    decode_png_file(filename, &bytes, &width, &height);

    // stb_image.h implementation
    unsigned char *data = stbi_load(filename, &width, &height, &n, 0);
    printf("n: %d\n", n);

    // ... process data if not NULL ...
    // ... x = width, y = height, n = # 8-bit components per pixel ...
    // ... replace '0' with '1'..'4' to force that many components per pixel
    // ... but 'n' will always be the number that it would have been if you said 0
    int test_failed = 0;
    if (data != NULL && bytes != NULL) {
        FILE *outfile = fopen("thingy.txt", "w");
        char pixel[18];
        // TODO this is wrong, assumes it is the truecolor alpha type, 4 colors
        // per pixel. Should be comparing byte by byte
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width * n; j++) {
                    int idx = i * width * n + j;
                    test_failed = data[idx] != bytes[idx];
            }
        }
        printf(test_failed ? "Test failed\n" : "Test passed\n");
    } else {
        printf("unable to parse file\n");
    }


    // TODO optional print, remove for actual program
#ifndef GITHUB_ACTIONS
    printf("github actions is not active\n");
    if (data != NULL) {
        FILE *outfile = fopen("thingy.txt", "w");
        char pixel[18];
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width * n; j+= n) {
                int idx = i * width * n + j;
                fputc('[', outfile);
                for (int k = 0; k < n; k++) {
                    sprintf(pixel, "%hhu", data[idx + k]);
                    fputs(pixel, outfile);
                    if (k != n - 1) {
                        fputc(',', outfile);
                    }
                }
                fputc(']', outfile);
            }
            fputs("\n", outfile);
        }
        fclose(outfile);
    }
#endif

    stbi_image_free(data);
    free(bytes);

    return test_failed; // return 1 if failed for github actions
}
