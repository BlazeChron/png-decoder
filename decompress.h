#pragma once
#include "bitreader.h"

int decompress(struct bitreader *br, char* decompressed_bytes);
//int inflate(struct bitreader *br, char* decompressed_bytes);

//struct huffman_tree *bl_list_to_tree(int bl[], int n, unsigned int alphabet[]);

