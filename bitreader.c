#include "bitreader.h"
#include <stdlib.h>

struct bitreader *make_bitreader(char *filename)
{
        struct bitreader *br = malloc(sizeof(struct bitreader));
        br->fptr = fopen(filename, "r");
        br->numbits = 0;
        br->b = 0;
        return br;
}

void free_bitreader(struct bitreader *br)
{
       fclose(br->fptr);
       free(br); 
}

void bitreader_get_char(struct bitreader *br)
{
        putchar(fgetc(br->fptr));
}

unsigned int read_byte(struct bitreader *br)
{
        br->numbits = 0;
        return fgetc(br->fptr);
}

unsigned int read_bit(struct bitreader *br)
{
        if (br->numbits <= 0) {
                br->b = read_byte(br);
                br->numbits = 8;
        }
        br->numbits -= 1;
        unsigned int bit = br->b & 1;
        br->b >>= 1;
        return bit;
}

unsigned int read_bits(struct bitreader *br, int n)
{
        unsigned int o = 0;
        for (int i = 0; i < n; i++) {
                o |= read_bit(br) << i;
        }
        return o;
}

unsigned int read_bytes(struct bitreader *br, int n)
{
        unsigned int o = 0;
        for (int i = 0; i < n; i++) {
                o |= read_byte(br) << (8 * i);
        }
        return o;
}
