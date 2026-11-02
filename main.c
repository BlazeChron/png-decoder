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
int paeth_predictor(int r, int c, unsigned char recon[], int width);
int recon_a(int r, int c, unsigned char recon[], int width);
int recon_b(int r, int c, unsigned char recon[], int width);
int recon_c(int r, int c, unsigned char recon[], int width);

int main()
{
        //test_decompress();

        //char filename[] = "basn6a08.png";
        char filename[] = "202610101545.png";
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
        struct bitreader *idat_br = malloc(sizeof(struct bitreader));
        idat_br->fptr = tmp;
        idat_br->numbits = 0;
        idat_br->b = 0;
        decompress(idat_br, decompressed_bytes);

        unsigned char *recon = malloc(sizeof(unsigned char*) * height * width);

        int idat_iterator = 0;
        for (int i = 0; i < height; i++) {
                unsigned char filter_type = decompressed_bytes[idat_iterator];
                idat_iterator++;
                for (int j = 0; j < width * 4; j++) {
                        int filter_x = decompressed_bytes[idat_iterator];
                        idat_iterator++;
                        int recon_x;
                        if (filter_type == 0) {
                                recon_x = filter_x;
                        } else if (filter_type == 1) {
                                recon_x = filter_x + recon_a(i, j, recon, width);
                        } else if (filter_type == 2) {
                                recon_x = filter_x + recon_b(i, j, recon, width);
                        } else if (filter_type == 3) {
                                int ave = recon_a(i, j, recon, width) / 2 + recon_b(i, j, recon, width) / 2;
                                recon_x = filter_x + ave;
                        } else if (filter_type == 4) {
                                recon_x = filter_x + paeth_predictor(i, j, recon, width);
                        }
                        recon[i * width * 4 + j] = recon_x & 255;
                }
        }
        for (int i = 0; i < height; i++) {
                for (int j = 0; j < width * 4; j+= 4) {
                        int idx = i * width * 4 + j;
                        printf("[%hhu,", recon[idx]);
                        printf("%hhu,", recon[idx + 1]);
                        printf("%hhu,", recon[idx + 2]);
                        printf("%hhu]", recon[idx + 3]);
                }
                printf("\n");
        }

        //for (int i = 0; i < 300; i += 4) {
        //        unsigned char r = decompressed_bytes[i];
        //        unsigned char g = decompressed_bytes[i + 1];
        //        unsigned char b = decompressed_bytes[i + 2];
        //        unsigned char a = decompressed_bytes[i + 3];
        //        printf("RGBA: %hhu %hhu %hhu %hhu\n", r, g, b, a);
        //}
}
int paeth_predictor(int row, int col, unsigned char recon[], int width) {
        int a = recon_a(row, col, recon, width);
        int b = recon_b(row, col, recon, width);
        int c = recon_c(row, col, recon, width);
        int p = a + b - c;
        int pa = p - a > 0 ? p - a : a - p;
        int pb = p - b > 0 ? p - b : b - p;
        int pc = p - c > 0 ? p - c : c - p;
        if (pa <= pb && pa <= pc) {
                return a;
        } else if (pb <= pc) {
                return b;
        } else {
                return c;
        }
}

int recon_a(int r, int c, unsigned char recon[], int width) {
        if (c >= 4) {
                return recon[r * width * 4 + c - 4];
        } else {
                return 0;
        }
}
int recon_b(int r, int c, unsigned char recon[], int width) {
        if (r > 0) {
                return recon[(r - 1) * width * 4 + c];
        } else {
                return 0;
        }
}
int recon_c(int r, int c, unsigned char recon[], int width) {
        if (r > 0 && c >= 4) {
                return recon[(r - 1) * width * 4 + c - 4];
        } else {
                return 0;
        }
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
