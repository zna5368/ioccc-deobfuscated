/*
 * This file is never meant to be linked or run: it has no main().
 * Compiling it with `cc -c` places the bytes of the string below in the
 * .rodata section of the object file, so writing that object file to a
 * terminal (or through `strings`) shows the greeting.
 *
 * The variable name is kept to three letters so that `strings`, which only
 * shows runs of four or more printable characters, does not print it.
 */
char *msg = "Hello world.\n";
