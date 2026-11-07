#include "decompress.h"
#include "huffman_tree.h"

int decode_trees(struct bitreader *br, struct huffman_tree **literal_tree,
                 struct huffman_tree **distance_tree);
int inflate(struct bitreader *br, char *db);

int inflate_block_no_compression(struct bitreader *br, char **decompressed_bytes); 
int inflate_block_fixed(struct bitreader *br, char **decompressed_bytes);
int inflate_block_dynamic(struct bitreader *br, char **decompressed_bytes);

// TODO Only returns success/failure, can't return stream rn
int decompress(struct bitreader *br, char* decompressed_bytes)
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
        //printf("CMF: %d, FLG: %d, Result: %d\n", CMF, FLG, (CMF * 256 + FLG) % 31);

        if ((CMF * 256 + FLG) % 31 != 0) {
                printf("Failed CMF FLG checksum\n");
                return -1;
        }

        unsigned int FDICT = (FLG >> 5) & 1;    // Should not be set for .png
        if (FDICT == 1) {
                printf("Invalid FDICT %d\n", FDICT);
                return -1;
        }
        inflate(br, decompressed_bytes);
        unsigned int ADLER32 = read_bytes(br, 4);   // Ignored

        return 0;
}

int inflate(struct bitreader *br, char* decompressed_bytes)
{
        unsigned int BFINAL = 0;

        // target .png is 217x120 pixels, 4bytes per pixel = 104160.
        // 300000 is conservative, bc I'm not sure yet
        //char decompressed_bytes[300000];

        while (!BFINAL) {
                BFINAL = read_bit(br);
                unsigned int BTYPE = read_bits(br, 2);
                if (BTYPE == 0) {
                        printf("Using no compression inflation\n");
                        inflate_block_no_compression(br, &decompressed_bytes); 
                } else if (BTYPE == 1) {
                        printf("Using fixed inflation\n");
                        inflate_block_fixed(br, &decompressed_bytes);
                } else if (BTYPE == 2) {
                        printf("Using dynamic inflation\n");
                        inflate_block_dynamic(br, &decompressed_bytes);
                } else {
                        printf("Invalid BTYPE for block\n");
                        return -1;
                }
        }

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
                       char** decompressed_bytes)
{
        while (1) {
                unsigned int symbol = decode_symbol(length_tree, br);
                if (symbol <= 255) {
                        **decompressed_bytes = symbol;
                        (*decompressed_bytes)++;
                        continue;
                }
                if (symbol == 256) {
                        return 0;
                }
                // length/distance
                symbol -= 257;
                int length = read_bits(br, length_extra_bits[symbol]) +
                             length_base[symbol];
                unsigned int dst_symbol = decode_symbol(distance_tree, br);
                int distance = read_bits(br, distance_extra_bits[dst_symbol]) +
                               distance_base[dst_symbol];
                for (int i = 0; i < length; i++) {
                        **decompressed_bytes = *(*decompressed_bytes - distance);
                        (*decompressed_bytes)++;
                }
        }
        return 0;
}

int max(int a, int b)
{
        return a > b ? a : b;
}

struct huffman_tree *bl_list_to_tree(int bl[], int n, unsigned int alphabet[])
{

        int bl_max = 0;
        for (int i = 0; i < n; i++) {
                bl_max = max(bl_max, bl[i]);
        }
        int bl_count[16] = {0}; //TODO idk the max number, p sure it's [0, 15]
        // Not sure if should duplicate this or just ignore bl_max
        for (int i = 0; i <= bl_max; i++) {
                for (int j = 0; j < n; j++) {
                        if (bl[j] != i || bl[j] == 0) {
                                continue;
                        }
                        bl_count[i]++;
                }
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

int code_length_codes_order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12,
                                 3, 13, 2, 14, 1, 15};

int decode_trees(struct bitreader *br, struct huffman_tree **literal_tree,
                 struct huffman_tree **distance_tree)
{
        // number of literal/length codes
        unsigned int HLIT = read_bits(br, 5) + 257;
        // number of distance codes
        unsigned int HDIST = read_bits(br, 5) + 1;
        // number of code length codes
        unsigned int HCLEN = read_bits(br, 4) + 4;

        int code_len_tree_bl[19] = {0};
        unsigned int code_len_alphabet[19];
        for (int i = 0; i < 19; i++) {
                code_len_alphabet[i] = i;
        }
        for (int i = 0; i < HCLEN; i++) {
                code_len_tree_bl[code_length_codes_order[i]] = read_bits(br, 3);
        }
        struct huffman_tree *code_len_tree = bl_list_to_tree(code_len_tree_bl,
                                                             19,
                                                             code_len_alphabet);

        int bit_len[318] = {0}; // 286 lit tree symbols + 32 dst tree symbols
        int i = 0;
        while (i < HLIT + HDIST) {
                unsigned int symbol = decode_symbol(code_len_tree, br);
                if (0 <= symbol && symbol <= 15) {
                        bit_len[i++] = symbol;
                } else if (symbol == 16) {
                        // copy previous code length 3-6 times
                        int prev_code_length = bit_len[i - 1];
                        int repeat_length = read_bits(br, 2) + 3;
                        for (int j = 0; j < repeat_length; j++) {
                                bit_len[i++] = prev_code_length;
                        }
                } else if (symbol == 17) {
                        int repeat_length = read_bits(br, 3) + 3;
                        for (int j = 0; j < repeat_length; j++) {
                                bit_len[i++] = 0;
                        }
                } else if (symbol == 18) {
                        int repeat_length = read_bits(br, 7) + 11;
                        for (int j = 0; j < repeat_length; j++) {
                                bit_len[i++] = 0;
                        }
                } else {
                }
        }
        unsigned int alphabet[286] = {0};
        for (int i = 0; i < 286; i++) {
                alphabet[i] = i;
        }

        // TODO better way to do this section than copying twice
        int literal_lengths[286];
        for (int i = 0; i < HLIT; i++) {
                literal_lengths[i] = bit_len[i];
        }
        int distances[32];
        for (int i = HLIT; i < HLIT + HDIST; i++) {
                distances[i - HLIT] = bit_len[i];
        }
        *literal_tree = bl_list_to_tree(literal_lengths, HLIT, alphabet);

        *distance_tree = bl_list_to_tree(distances, HDIST, alphabet);

        free_huffman_tree(code_len_tree);
        return 0;
}

int inflate_block_no_compression(struct bitreader *br, char **decompressed_bytes)
{
        unsigned int LEN = read_bytes(br, 2);
        unsigned int NLEN = read_bytes(br, 2);
        for (int i = 0; i < LEN; i++) {
                **decompressed_bytes = read_byte(br);
                (*decompressed_bytes)++;
        }
        return 0;
}

int inflate_block_fixed(struct bitreader *br, char **db)
{
        // pre-build known trees
        int bit_len[288];
        int i = 0;
        while (i < 144) {
               bit_len[i++] = 8; 
        }
        while (i < 256) {
               bit_len[i++] = 9; 
        }
        while (i < 280) {
               bit_len[i++] = 7; 
        }
        while (i < 288) {
               bit_len[i++] = 8; 
        }
        unsigned int alphabet[286] = {0};
        for (int j = 0; j < 286; j++) {
                alphabet[j] = j;
        }
        struct huffman_tree *literal_tree = bl_list_to_tree(bit_len, 286,
                                                            alphabet);
        
        for (int j = 0; j < 30; j++) {
                bit_len[i] = 5;
        }
        struct huffman_tree *distance_tree = bl_list_to_tree(bit_len, 30,
                                                             alphabet);
        inflate_block_data(br, literal_tree, distance_tree, db);

        free_huffman_tree(literal_tree);
        free_huffman_tree(distance_tree);
        return 0;
}

int inflate_block_dynamic(struct bitreader *br, char **db)
{
        struct huffman_tree *literal_tree;
        struct huffman_tree *distance_tree;
        decode_trees(br, &literal_tree, &distance_tree);
        inflate_block_data(br, literal_tree, distance_tree, db);

        free_huffman_tree(literal_tree);
        free_huffman_tree(distance_tree);
        return 0;
}
