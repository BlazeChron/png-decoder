#!/bin/bash

gcc test.c ../bitreader.c ../decompress.c ../huffman_tree.c ../decoder.c -lm
