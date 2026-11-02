#!/bin/bash
TMPDIR=~/tmp valgrind -s --leak-check=full ./a.out 
