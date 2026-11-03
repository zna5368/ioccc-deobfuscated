/*
 * Prints "Hello, world!" using a raw write() to standard output
 * followed by a newline from putchar().
 *
 * The original source is also a valid Bourne shell script and a
 * Fortran 77 program; only the C part is kept here.
 */

int message_length = 13; /* strlen("Hello, world!") */

main() {
  write(1, "Hello, world!", message_length); /* 1 is stdout */
  putchar('\n');
}
