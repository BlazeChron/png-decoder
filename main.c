#include <stdio.h>

#include "bitreader.h"
#include "decompress.h"
#include "huffman_tree.h"

#include <stdlib.h>


// For testing using bytes in the DEFLATE spec order
int code_to_bytes(unsigned int code, int n, char* filename);
int test_decompress();

struct chunk {
        unsigned int chunk_type;
        unsigned char *chunk_data;
};

int read_chunk(struct bitreader *br, struct chunk *c);

int main()
{
        //test_decompress();

        char filename[] = "basn6a08.png";
        struct bitreader *br = make_bitreader(filename); 

        // .png signature validation
        unsigned char png_signature[] = "\x89PNG\r\n\x1a\n";
        for (int i = 0; i < 8; i++) {
                if (png_signature[i] != read_byte(br)) {
                        printf("Invalid PNG Signature\n");
                        return -1;
                }
        }

        // reading chunks
        struct chunk chunks[10000]; // idk how large
        struct chunk *chunksptr = chunks;
        // idk if there's a better way to do this
        unsigned int IEND = 'I' << 24 | 'E' << 16 | 'N' << 8 | 'D';
        while (1) {
                struct chunk *c = malloc(sizeof(struct chunk));
                read_chunk(br, c);
                *chunksptr++ = *c;
                if (c->chunk_type == IEND) {
                        break;
                }
        }
}

int read_chunk(struct bitreader *br, struct chunk *c)
{
        // Reading chunks
        // File is big endian, so need to hack it with bitreader
        unsigned int chunk_length = 0;
        for (int i = 3; i >= 0; i--) {
                chunk_length |= read_byte(br) << (i * 8);
        }
        printf("chunk length: %u\n", chunk_length);

        c->chunk_type = 0;
        for (int i = 3; i >= 0; i--) {
                c->chunk_type |= read_byte(br) << (i * 8);
        }

        c->chunk_data = malloc(chunk_length * sizeof(unsigned char));
        for (int i = 0; i < chunk_length; i++) {
                c->chunk_data[i] = read_byte(br);
        }

        // yea I'm not doing CRC maybe another time
        unsigned int chunk_crc = 0;
        for (int i = 3; i >= 0; i--) {
                chunk_crc |= read_byte(br) << (i * 8);
        }
        
}

int test_decompress()
{
        char filename[] = "python_hello_world.txt";
        struct bitreader *br = make_bitreader(filename); 
        decompress(br);
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
