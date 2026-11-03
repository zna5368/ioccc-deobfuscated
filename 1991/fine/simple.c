/*
 * The entry's Makefile defines a function-like macro named main(a,b), so the
 * name is wrapped in parentheses to stop the preprocessor from expanding it.
 */
int (main)(void)
{
  int ch;

  while ((ch = getchar()) != -1) { /* -1 is EOF; no headers are included */
    /* bits 5 and 6 of a letter: 0x40 for upper case, 0x60 for lower case */
    int case_bits = ch & 0x60;
    /* clear the case bit (0x20) and flip bit 6 (0x40): 'A'/'a' -> 1 ... 'Z'/'z' -> 26 */
    int letter_index = (ch & 0xDF) ^ 0x40;

    if (letter_index >= 1 && letter_index <= 26) {
      /* rotate the alphabet position by 13, keeping it in the range 1..26 */
      int rotated = (letter_index - 1 + 13) % 26 + 1;
      putchar(case_bits | rotated);
    } else {
      putchar(ch);
    }
  }
  return 0;
}
