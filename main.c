#include <stdio.h>

#include "bitreader.h"

int decompress(struct bitreader *br);
int inflate(struct bitreader *br, int* out);

int inflate_block_no_compression(struct bitreader *br, char *decompressed_bytes); 
int inflate_block_fixed(struct bitreader *br, char *decompressed_bytes);
int inflate_block_dynamic(struct bitreader *br, char *decompressed_bytes);

// Might be useless, just for testing
int code_to_bytes(unsigned int code, int n, char* filename);

int main()
{
        char filename[] = "code_to_bytes.txt";
        code_to_bytes(0b11101000001, 11, filename);
        struct bitreader *br = make_bitreader(filename); 
        for (int i = 0; i < 11; i++) {
                printf("%d\n", read_bit(br));
        }
        //struct bitreader *br = make_bitreader("python_hello_world.txt"); 
        //decompress(br);
}

// TODO Only returns success/failure, can't return stream rn
int decompress(struct bitreader *br)
{
        unsigned int CMF = read_byte(br);
        int CM = CMF & 15;      // Compression method
        if (CM != 8) {
                printf("Invalid CM: %d\n", CM);
                return -1;
        }
        unsigned int CINFO = (CMF >> 4) & 15;   // Compression Info
        if (CINFO > 7) {
                printf("Invalid CINFO: %d\n", CINFO);
                return -1;
        }
        unsigned int FLG = read_byte(br);

        if ((CMF * 256 + FLG) % 31 != 0) {
                printf("Failed CMF FLG checksum\n");
                return -1;
        }

        unsigned int FDICT = (FLG >> 5) & 1;    // Should not be set for .png
        if (FDICT == 1) {
                printf("Invalid FDICT %d\n", FDICT);
                return -1;
        }
        int out;
        inflate(br, &out);
        unsigned int ADLER32 = read_bytes(br, 4);   // Ignored

        return 0;
}

int inflate(struct bitreader *br, int* out)
{
        unsigned int BFINAL = 0;

        // target .png is 217x120 pixels, 4bytes per pixel = 104160.
        // 300000 is conservative, bc I'm not sure yet
        char decompressed_bytes[300000];

        while (!BFINAL) {
                BFINAL = read_bit(br);
                unsigned int BTYPE = read_bits(br, 2);
                if (BTYPE == 0) {
                        inflate_block_no_compression(br, decompressed_bytes); 
                } else if (BTYPE == 1) {
                        inflate_block_fixed(br, decompressed_bytes);
                } else if (BTYPE == 2) {
                        inflate_block_dynamic(br, decompressed_bytes);
                } else {
                        printf("Invalid BTYPE for block\n");
                        return -1;
                }
        }

        // TODO just for testing
        printf("%s\n", decompressed_bytes);
        return 0;
}

int inflate_block_no_compression(struct bitreader *br, char *decompressed_bytes)
{
        unsigned int LEN = read_bytes(br, 2);
        unsigned int NLEN = read_bytes(br, 2);
        for (int i = 0; i < LEN; i++) {
                decompressed_bytes[i] = read_byte(br);
        }
        return 0;
}

int inflate_block_fixed(struct bitreader *br, char *decompressed_bytes)
{
        return -1;
}

int inflate_block_dynamic(struct bitreader *br, char *decompressed_bytes)
{
        return -1;
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
