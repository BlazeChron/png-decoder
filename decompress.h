#pragma once
#include "bitreader.h"

int decompress(struct bitreader *br);
int inflate(struct bitreader *br, int* out);

int inflate_block_no_compression(struct bitreader *br, char *decompressed_bytes); 
int inflate_block_fixed(struct bitreader *br, char *decompressed_bytes);
int inflate_block_dynamic(struct bitreader *br, char *decompressed_bytes);
