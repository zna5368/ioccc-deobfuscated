/*
 * Text scrambler disguised as a prime number generator.
 *
 * Every input byte that has the 64 bit set (letters and most punctuation
 * in the @..DEL range) is XORed with the key (argv[1] modulo 64). Since
 * the key is below 64, the 64 bit is never changed, so running the
 * output through the program again with the same key restores the text.
 */
int main(int argc, char *argv[]) {
  int key;
  char c; /* char, not int: a 0xFF byte reads as EOF on signed char */

  key = atoi(argv[1]) % 64;

  while ((c = getchar()) != -1) {
    if (c & 64)
      c ^= key;
    putchar(c);
  }
  return 0;
}
