/*
 * This file contains no program of its own. When it is compiled, the
 * preprocessor reads the terminal device as if it were a header file, so
 * the C code typed at the keyboard (ended with EOF, usually control-D)
 * becomes the whole source of the program being built.
 *
 * For example, typing the contents of input.txt produces a program that
 * prints "Hello, world!".
 */
#include "/dev/tty"
