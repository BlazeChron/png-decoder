#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "../decoder.h"

// TODO figure out what these do
//#define STBI_MALLOC
//#define STBI_REALLOC
//#define STBI_FREE
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"


/*
Features to test

test features:
- bas basic
- sXX odd sizes XX is XXxXX image (eg. s32 is 32x32)
- bga background colors
- gXX gamma //TODO not supported
- fXX filter
- psX palette //TODO not supported
- Ancillary chunks //TODO not supported
- zXX zlib compression level
- corrupted files //TODO add proper errors

interlacing
- n no interlacing
- i interlacing //TODO not supported

color-type
- 0g grayscale //TODO not supported
- 2c rgb //TODO not supported
- 3p paletted //TODO not supported
- 4a grayscale + alpha //TODO not supported
- 6a rgb + alpha

bit-depth
- 01 //TODO not supported
- 02 //TODO not supported
- 04 //TODO not supported
- 08
- 16 //TODO not supported
*/

// TODO add filter here
int is_valid_test_image(char *filename) {
    return 1;
}

// Compares with stb_image, returns 0 on success and 1 on failed test
int compare_with_stb_image(char *full_filepath) {
    int width, height, n;

    // own api
    unsigned char *bytes = NULL;
    decode_png_file(full_filepath, &bytes, &width, &height);

    // stb_image.h implementation
    unsigned char *data = stbi_load(full_filepath, &width, &height, &n, 0);

    // ... process data if not NULL ...
    // ... x = width, y = height, n = # 8-bit components per pixel ...
    // ... replace '0' with '1'..'4' to force that many components per pixel
    // ... but 'n' will always be the number that it would have been if you said 0
    int test_failed = 0;
    if (data != NULL && bytes != NULL) {
        char pixel[18];
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width * n; j++) {
                    int idx = i * width * n + j;
                    test_failed = data[idx] != bytes[idx];
            }
        }
        //printf(test_failed ? "Test failed\n" : "Test passed\n");
    } else {
        printf("unable to parse file\n");
        test_failed = 1;
    }

    stbi_image_free(data);
    free(bytes);

    return test_failed;
}

int main(int argc, char *argv[])
{
    //struct test_image test_images[175] = {0};
    // Look through test images in directory
    char filedir[] = "./test/pngsuite/png/";
    char fullpath[sizeof(filedir) + 13];
    printf("sizeof(filedir) %ld\n", sizeof(filedir));
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(filedir)) == NULL) {
        printf("Failed to open directory %s\n", filedir);
        return -1;
    }
    printf("Scanning directory %s\n", filedir);

    int total_tests = 0;
    int tests_passed = 0;
    while((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            printf("Found %s, not a file, skipping...\n", ent->d_name);
            continue;
        }
        if (is_valid_test_image(ent->d_name)) {
            total_tests++;
            sprintf(fullpath, "%s%s", filedir, ent->d_name);
            if (compare_with_stb_image(fullpath) == 0) {
                tests_passed++;
                printf("%s passed test\n", ent->d_name);
            } else {
                printf("%s failed test\n", ent->d_name);
            }
        } else {
            printf("Found %s, skipping...\n", ent->d_name);
        }
    } 
    printf("Passed %d / %d test cases\n", tests_passed, total_tests);
    closedir(dir);
//basn6a08.png

/*


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
*/

//    return test_failed; // return 1 if failed for github actions
    return 0; // return 1 if failed for github actions
}
