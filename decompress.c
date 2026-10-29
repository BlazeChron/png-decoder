#include "decompress.h"

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
