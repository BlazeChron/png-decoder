#!/bin/bash

#gcc test.c ../bitreader.c ../decompress.c ../huffman_tree.c ../decoder.c -lm
#gcc test/test.c bitreader.c decompress.c huffman_tree.c decoder.c -lm -o png_test_executable
gcc -DGITHUB_ACTIONS test/test.c bitreader.c decompress.c huffman_tree.c decoder.c -lm -o png_test_executable

./png_test_executable
