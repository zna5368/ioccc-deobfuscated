/*
 * The original program is laid out to look like a broken C fragment, but
 * almost all of it is either a comment, dead code, or code that is skipped
 * over by a goto. The only thing it really does is print a single string
 * once and then stop.
 *
 * The program is built with -include stdio.h -include math.h, so printf()
 * is declared without an #include line, just like the original.
 */

/* The original's "k['a']" array: 'a' == 97 ints, first one set to the size of
 * a function pointer. */
int table[97] = {sizeof(int (*)())};

int main(void) {
  /* The for loop condition multiplies table[0] by the first character of the
   * file name ("decot.c" -> 'd'). The result is never zero, so the loop body
   * is entered exactly once. */
  table[0] *= 'd';

  /* The string literal looks like it contains code and a comment, but it is
   * really just text: single quote, double quote, ",x);", TAB, slash, star,
   * NEWLINE, backslash. */
  printf("'\",x);\t/*\n\\");

  /* The original then does "goto _0" which jumps out of the loop to an
   * if-statement whose condition starts with "b &&". The global b is always
   * 0 at that point, so nothing else happens and main() falls off the end. */
  return 0;
}
