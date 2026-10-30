#include "decompress.h"
#include "huffman_tree.h"

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

int length_extra_bits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3,
                           3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
int length_base[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                     35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
int distance_extra_bits[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7,
                           7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
int distance_base[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                     257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
                     8193, 12289, 16385, 24577};

int inflate_block_data(struct bitreader *br, struct huffman_tree *length_tree,
                       struct huffman_tree *distance_tree,
                       char* decompressed_bytes)
{
        while (1) {
                unsigned int symbol = decode_symbol(length_tree, br);
                if (symbol <= 255) {
                        *decompressed_bytes = symbol;
                        decompressed_bytes++;
                        continue;
                }
                if (symbol == 256) {
                        return 0;
                }
                // length/distance
                symbol -= 257;
                unsigned int dst_symbol = decode_symbol(distance_tree, br);
                int length = read_bits(br, length_extra_bits[symbol]) +
                             length_base[symbol];
                int distance = read_bits(br, distance_extra_bits[dst_symbol]) +
                               distance_base[dst_symbol];
                for (int i = 0; i < length; i++) {
                        *decompressed_bytes = *(decompressed_bytes - distance);
                }
        }
        return 0;
}

int max(int a, int b)
{
        return a > b ? a : b;
}

struct huffman_tree *bl_list_to_tree(int bl[], int n, char alphabet[])
{
        int bl_max = 0;
        for (int i = 0; i < n; i++) {
                bl_max = max(bl_max, bl[i]);
        }
        int bl_count[16] = {0}; //TODO idk the max number, p sure it's [0, 15]
        // Not sure if should duplicate this or just ignore bl_max
        for (int i = 0; i < n; i++) {
                bl_count[bl[i]]++;
        }
        unsigned int next_code[16] = {0}; // Definitely 16
        for (int i = 2; i <= bl_max; i++) {
                next_code[i] = (next_code[i - 1] + bl_count[i - 1]) << 1;
        }

        struct huffman_tree *ht = create_huffman_tree();

        for (int i = 0; i < n; i++) {
                int bitlen = bl[i];
                if (bitlen != 0) {
                        insert(ht, next_code[bitlen], bitlen, alphabet[i]);
                        next_code[bitlen]++;
                }
        }
        return ht;
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
