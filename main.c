#include <stdio.h>

#include "bitreader.h"
#include "decompress.h"
#include "huffman_tree.h"

#include <stdlib.h>


// For testing using bytes in the DEFLATE spec order
int code_to_bytes(unsigned int code, int n, char* filename);
int test_decompress();

struct chunk {
        unsigned int chunk_length;
        unsigned int chunk_type;
        unsigned char *chunk_data;
};

int read_chunk(struct bitreader *br, struct chunk *c);
void print_chunk_type(unsigned int chunk_type);
unsigned int read_four_bytes(struct bitreader *br);

int main()
{
        test_decompress();

        char filename[] = "basn6a08.png";
        //char filename[] = "202610101545.png";
        struct bitreader *br = make_bitreader(filename); 

        // .png signature validation
        unsigned char png_signature[] = "\x89PNG\r\n\x1a\n";
        for (int i = 0; i < 8; i++) {
                if (png_signature[i] != read_byte(br)) {
                        printf("Invalid PNG Signature\n");
                        return -1;
                }
        }
/*
        // reading chunks
        int MAX_CHUNKS = 100;
        struct chunk chunks[MAX_CHUNKS]; // idk how large
        struct chunk *chunksptr = chunks;
        // idk if there's a better way to do this
        unsigned int IEND = 'I' << 24 | 'E' << 16 | 'N' << 8 | 'D';
        while (1) {
                struct chunk *c = malloc(sizeof(struct chunk));
                read_chunk(br, c);
                *chunksptr++ = *c;
                print_chunk_type(c->chunk_type);
                if (c->chunk_type == IEND) {
                        break;
                }
        }

        // reading IHDR chunk
        int ihdr_index = 0;
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned int bitd = 0;
        unsigned int colort = 0;
        unsigned int compm = 0;
        unsigned int filterm = 0;
        unsigned int interlacem = 0;
        for (int i = 3; i >= 0; i--) {
                width |= *(chunks[0].chunk_data + ihdr_index++) << (i * 8);
        }
        for (int i = 3; i >= 0; i--) {
                height |= *(chunks[0].chunk_data + ihdr_index++) << (i * 8);
        }
        bitd = *(chunks[0].chunk_data + ihdr_index++);
        colort = *(chunks[0].chunk_data + ihdr_index++);
        compm = *(chunks[0].chunk_data + ihdr_index++);
        filterm = *(chunks[0].chunk_data + ihdr_index++);
        interlacem = *(chunks[0].chunk_data + ihdr_index++);
        if (compm != 0)  {
                printf("invalid compression method");
                return -1;
        }
        if (filterm != 0)  {
                printf("invalid filter method");
                return -1;
        }
        if (colort != 6)  {
                printf("only truecolor alpha supported");
                return -1;
        }
        if (bitd != 8)  {
                printf("only bitdepth = 8 supported");
                return -1;
        }
        if (interlacem != 0)  {
                printf("interlacing not supported");
                return -1;
        }
        printf("width: %u height: %u\n", width, height);


        // compiling IDAT chunks
        FILE *tmp = tmpfile();
        if (tmp == NULL) {
                printf("unable to create tmp file for IDAT chunks\n");
                return -1;
        }
        // taking advantage of PNG spec requiring consecutive IDAT chunks
        unsigned int IDAT = 'I' << 24 | 'D' << 16 | 'A' << 8 | 'T';
        for (int i = 0; i < MAX_CHUNKS; i++) {
                if (chunks[i].chunk_type != IDAT) {
                        continue;
                }
                printf("found IDAT\n");
                print_chunk_type(chunks[i].chunk_type);
                for (unsigned int j = 0; j < chunks[i].chunk_length;
                     j++) {
                        fputc(*(chunks[i].chunk_data + j), tmp);
                }
        }
        char decompressed_bytes[300000];
        rewind(tmp);
        printf("rewind tmp\n");
        struct bitreader *idat_br = malloc(sizeof(struct bitreader));
        printf("created br\n");
        idat_br->fptr = tmp;
        idat_br->numbits = 0;
        idat_br->b = 0;
        printf("assigned fptr\n");
        decompress(idat_br, decompressed_bytes);
        printf("decompressed\n");
        for (int i = 0; i < 100; i++) {
                printf("%d\n", decompressed_bytes);
        }
        */
}

unsigned int read_four_bytes(struct bitreader *br)
{
        unsigned int value = 0;
        for (int i = 3; i >= 0; i--) {
                value |= read_byte(br) << (i * 8);
        }
        return value;
}

void print_chunk_type(unsigned int chunk_type) {
        char letters[5];
        for (int i = 0; i < 4; i++) {
                letters[i] = (chunk_type >> ((3 - i) * 8)) & 255;
        }
        letters[4] = '\0';
        printf("chunk_type: %s\n", letters);
}

int read_chunk(struct bitreader *br, struct chunk *c)
{
        // Reading chunks
        // File is big endian, so need to hack it with bitreader
        c->chunk_length = read_four_bytes(br);
        printf("chunk length: %u\n", c->chunk_length);

        c->chunk_type = read_four_bytes(br);

        c->chunk_data = malloc(c->chunk_length * sizeof(unsigned char));
        for (int i = 0; i < c->chunk_length; i++) {
                c->chunk_data[i] = read_byte(br);
        }

        // yea I'm not doing CRC maybe another time
        unsigned int chunk_crc = read_four_bytes(br);

        return 0;
}

int test_decompress()
{
        char filename[] = "python_hello_world.txt";
        struct bitreader *br = make_bitreader(filename); 
        char decompressed_bytes[300000];
        decompress(br, decompressed_bytes);
        printf("%s\n", decompressed_bytes);
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
