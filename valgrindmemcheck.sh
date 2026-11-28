#!/bin/bash
TMPDIR=~/tmp valgrind -s --leak-check=full ./png_test_executable
