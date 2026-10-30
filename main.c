#include <stdio.h>

#include "bitreader.h"
#include "decompress.h"
#include "huffman_tree.h"


// For testing using bytes in the DEFLATE spec order
int code_to_bytes(unsigned int code, int n, char* filename);

int main()
{
        char filename[] = "code_to_bytes.txt";
        code_to_bytes(0b00010110111, 11, filename);
        struct bitreader *br = make_bitreader(filename); 

        char alphabet[] = "ABCD";
        int bl[] = {2, 1, 3, 3};
        struct huffman_tree *ht = bl_list_to_tree(bl, 4, alphabet);

        //struct huffman_tree *ht = create_huffman_tree();
        //insert(ht, 0b01, 2, 'A');
        //insert(ht, 0b1, 1, 'B');
        //insert(ht, 0b000, 3, 'C');
        //insert(ht, 0b001, 3, 'D');
        printf("%c\n", decode_symbol(ht, br));
        printf("%c\n", decode_symbol(ht, br));
        printf("%c\n", decode_symbol(ht, br));
        printf("%c\n", decode_symbol(ht, br));
        printf("%c\n", decode_symbol(ht, br));
        printf("%c\n", decode_symbol(ht, br));
        //for (int i = 0; i < 11; i++) {
        //        printf("%d\n", read_bit(br));
        //}

        //struct bitreader *br = make_bitreader("python_hello_world.txt"); 
        //decompress(br);
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
