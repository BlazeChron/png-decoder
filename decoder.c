#include <stdio.h>
#include <stdlib.h>

#include "bitreader.h"
#include "decompress.h"

#include "decoder.h"

#define MAX_CHUNKS 1000 //idk how large

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

int decode_png_file(char *filename, unsigned char **bytes, unsigned int* w,
                    unsigned int* h)
{
        if (*bytes != NULL) {
                printf("bytes pointer is not NULL\n");
                return -2;
        }
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
        // TODO MAX_CHUNKS should probably be dynamically calculated
        struct chunk chunks[MAX_CHUNKS];
        int valid_chunks = 0;
        struct chunk *chunksptr = chunks;
        // idk if there's a better way to do this
        unsigned int IEND = 'I' << 24 | 'E' << 16 | 'N' << 8 | 'D';
        while (1) {
                //struct chunk *c = malloc(sizeof(struct chunk));
                read_chunk(br, chunksptr);
                print_chunk_type(chunksptr->chunk_type);
                valid_chunks++;
                if (chunksptr->chunk_type == IEND) {
                        break;
                }
                *chunksptr++;
        }
        printf("chunk count found: %d\n", valid_chunks);

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
        int idat_chunk_lengths = 0;
        if (tmp == NULL) {
                printf("unable to create tmp file for IDAT chunks\n");
                return -1;
        }
        // taking advantage of PNG spec requiring consecutive IDAT chunks
        unsigned int IDAT = 'I' << 24 | 'D' << 16 | 'A' << 8 | 'T';
        for (int i = 0; i < valid_chunks; i++) {
                if (chunks[i].chunk_type != IDAT) {
                        continue;
                }
                printf("found IDAT of length: %d\n", chunks[i].chunk_length);
                idat_chunk_lengths += chunks[i].chunk_length;
                for (unsigned int j = 0; j < chunks[i].chunk_length;
                     j++) {
                        fputc(*(chunks[i].chunk_data + j), tmp);
                }
        }

        //char *decompressed_bytes = malloc(sizeof(char) * width * height * 4);
        char *decompressed_bytes = malloc(sizeof(char) * (width * 4 + 1) * height);

        fflush(tmp);
        rewind(tmp);
        struct bitreader *idat_br = malloc(sizeof(struct bitreader));
        idat_br->fptr = tmp;
        idat_br->numbits = 0;
        idat_br->b = 0;
        decompress(idat_br, decompressed_bytes);
        printf("decompress finished\n");
        fflush(stdout);

        int bytes_per_pixel = 4;
        int stride = width * bytes_per_pixel;

        // Reconstructed array of pixels
        unsigned char *recon = malloc(sizeof(unsigned char) * height * stride);
        //recon = malloc(sizeof(unsigned char) * height * stride);

        int idat_iterator = 0;
        for (int r = 0; r < height; r++) {
                unsigned char filter_type = decompressed_bytes[idat_iterator++];
                for (int c = 0; c < stride; c++) {
                        int filter_x = decompressed_bytes[idat_iterator++];
                        int recon_x = 0;
                        if (filter_type == 0) {
                                recon_x = filter_x;
                        } else if (filter_type == 1) {
                                recon_x = filter_x + recon_a(r, c, recon, stride);
                        } else if (filter_type == 2) {
                                recon_x = filter_x + recon_b(r, c, recon, stride);
                        } else if (filter_type == 3) {
                                int ave = (recon_a(r, c, recon, stride) + recon_b(r, c, recon, stride)) / 2;
                                recon_x = filter_x + ave;
                        } else if (filter_type == 4) {
                                recon_x = filter_x + paeth_predictor(r, c, recon, stride);
                        } else {
                                //printf("Unknown filter type %d\n", filter_type);
                        }
                        recon[r * stride + c] = recon_x & 255;
                }
        }

        *w = width;
        *h = height;
        *bytes = recon;

        // cleanup
        for (int i = 0; i < valid_chunks; i++) {
                if (chunks[i].chunk_data != NULL) {
                        free(chunks[i].chunk_data);
                }
        }
        free(decompressed_bytes);
        free_bitreader(idat_br);
        free_bitreader(br);
}

int paeth_predictor(int row, int col, unsigned char recon[], int stride) {
        int a = recon_a(row, col, recon, stride);
        int b = recon_b(row, col, recon, stride);
        int c = recon_c(row, col, recon, stride);
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

int recon_a(int r, int c, unsigned char recon[], int stride) {
        if (c >= 4) {
                return recon[r * stride + c - 4];
        } else {
                return 0;
        }
}
int recon_b(int r, int c, unsigned char recon[], int stride) {
        if (r > 0) {
                return recon[(r - 1) * stride + c];
        } else {
                return 0;
        }
}
int recon_c(int r, int c, unsigned char recon[], int stride) {
        if (r > 0 && c >= 4) {
                return recon[(r - 1) * stride + c - 4];
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
