#pragma once
#include "bitreader.h"

struct huffman_node {
        unsigned int symbol; // 1 byte is not enough
        struct huffman_node *left;
        struct huffman_node *right;
};

struct huffman_tree {
        struct huffman_node *root;
};

struct huffman_tree *create_huffman_tree();
void free_huffman_tree(struct huffman_tree *ht);

int insert(struct huffman_tree *ht, unsigned int codeword,
           int n, unsigned int symbol);

unsigned int decode_symbol(struct huffman_tree *ht, struct bitreader *br);
