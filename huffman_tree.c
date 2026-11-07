#include "huffman_tree.h"
#include <stdlib.h>

void free_huffman_node(struct huffman_node *hn);

struct huffman_tree *create_huffman_tree()
{
        struct huffman_tree *ht = malloc(sizeof(struct huffman_tree));
        ht->root = malloc(sizeof(struct huffman_node));
        ht->root->left = NULL;
        ht->root->right = NULL;
        return ht;
}

void free_huffman_tree(struct huffman_tree *ht)
{
        free_huffman_node(ht->root);
        free(ht);
}

void free_huffman_node(struct huffman_node *hn)
{
        if (hn->left != NULL) {
                free_huffman_node(hn->left);
        }
        if (hn->right != NULL) {
                free_huffman_node(hn->right);
        }
        free(hn);
}

enum Direction {
        LEFT,
        RIGHT
};

void create_node_if_null(struct huffman_node *node, enum Direction dir) {
        if (dir == LEFT) {
                if (node->left == NULL) {
                        node->left = malloc(sizeof(struct huffman_node));
                        node->left->left = NULL;
                        node->left->right = NULL;
                }
        } else {
                if (node->right == NULL) {
                        node->right = malloc(sizeof(struct huffman_node));
                        node->right->left = NULL;
                        node->right->right = NULL;
                }
        }
}

// TODO Not sure size of codeword
int insert(struct huffman_tree *ht, unsigned int codeword,
           int n, unsigned int symbol)
{
        struct huffman_node *current_node = ht->root;
        for (int i = n - 1; i >= 0; i--) {
                if (codeword & (1 << i)) {
                        create_node_if_null(current_node, RIGHT);
                        current_node = current_node->right;
                } else {
                        create_node_if_null(current_node, LEFT);
                        current_node = current_node->left;
                }
        }
        current_node->symbol = symbol;
}

unsigned int decode_symbol(struct huffman_tree *ht, struct bitreader *br) {
        struct huffman_node *current_node = ht->root;
        while (current_node->left != NULL || current_node->right != NULL) {
                unsigned int bit = read_bit(br);
                if (bit) {
                        current_node = current_node->right;
                } else {
                        current_node = current_node->left;
                }
        }
        return current_node->symbol;
}
