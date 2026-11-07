#pragma once

/**
 * decode_png_file - Decodes the .png file given by filename into RGBA bytearray
 *
 * Decodes the .png file into a 1D array bytes. Every 4 adjacent bytes
     represents RGBA values of a pixel. The pixels are arranged as a scanline.
     Eg. For an image of width 3, the bytes[0] is the first row first
     col, bytes[1] is first row second col, bytes[2] = first row third col,
     bytes [3] = second row first col.
 *
 * @param char *filename: Name of the .png file. Naming it .png is not relevant
     as long as it is in .png format. Only supports truecoloralpha.
     Does not check filename and will just fail if it's invalid.
 * @param char **bytes: bytearray for each color value of each pixel in RGBA.
     Should be set to NULL when passed to the function. Length can be calculated
     from width * height * 4. User should free this pointer themselves.
 * @param int *width: Width of the image.
 * @param int *height: Height of the image.
 *
 * @return
      0 - No errors
     -1 - Some error idk
     -2 - bytes is not NULL
 */
int decode_png_file(char *filename, unsigned char **bytes, unsigned int* width,
                    unsigned int* height);

//int decode_png(struct bitreader *br);
