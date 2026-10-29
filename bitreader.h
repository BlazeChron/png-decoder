#pragma once
#include <stdio.h>

struct bitreader {
       FILE *fptr;
       unsigned int b;   //current byte
       int numbits; // not sure if should be unsigned, it's [0, 8] anyway
};

struct bitreader *make_bitreader(char *filename);
void bitreader_get_char(struct bitreader *br);

unsigned int read_byte(struct bitreader *br);
unsigned int read_bit(struct bitreader *br);
unsigned int read_bits(struct bitreader *br, int n);
unsigned int read_bytes(struct bitreader *br, int n);
