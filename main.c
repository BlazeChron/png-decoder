#include <stdio.h>

#include "bitreader.h"
#include "huffman_tree.h"
#include "decoder.h"
#include "decompress.h"

// For testing using bytes in the DEFLATE spec order
int code_to_bytes(unsigned int code, int n, char* filename);
int test_decompress();

int main()
{
        //test_decompress();

        //char filename[] = "basn6a08.png";
        //char filename[] = "202610101545.png";
        //char filename[] = "m.png";
        char filename[] = "image.png";
        //char filename[] = "suzukage.png";
        struct bitreader *br = make_bitreader(filename); 
        decode_png(br);
}


int test_decompress()
{
        char filename[] = "python_hello_world.txt";
        struct bitreader *br = make_bitreader(filename); 
        char decompressed_bytes[300000];
        decompress(br, decompressed_bytes);
        printf("%s\n", decompressed_bytes);
}


// Write bytes to file
int code_to_bytes(unsigned int code, int n, char* filename)
{
        FILE *fptr = fopen(filename, "wb");
        int numbits = 0;
        unsigned int current_byte = 0;
        for (int i = n - 1; i >= 0; i--) {
                if (numbits >= 8) {
                        fputc(current_byte, fptr);
                        numbits = 0;
                        current_byte = 0;
                }
                current_byte |= (code & (1 << i) ? 1 : 0) << numbits;
                numbits += 1;
        }

        fputc(current_byte, fptr);

        fclose(fptr);
}
