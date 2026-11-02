#pragma once
#include "bitreader.h"

int decompress(struct bitreader *br, char* decompressed_bytes);
//int inflate(struct bitreader *br, char* decompressed_bytes);

struct huffman_tree *bl_list_to_tree(int bl[], int n, unsigned int alphabet[]);

int inflate_block_no_compression(struct bitreader *br, char *decompressed_bytes); 
int inflate_block_fixed(struct bitreader *br, char *decompressed_bytes);
int inflate_block_dynamic(struct bitreader *br, char *decompressed_bytes);
