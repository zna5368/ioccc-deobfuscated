/*
 * Single image random dot stereogram ("magic eye") in ASCII.
 *
 * stdin holds a block of lowercase letters giving the depth of each pixel.
 * Every output character either repeats the character `depth` places to its
 * left, or (near the left edge or where the depth grows) is a new character:
 * a random one in 32..126, or the next one taken from argv[1].
 */

char *pattern;          /* next character of argv[1] to use              */
char picture[9876];     /* all output written so far                     */
char in;                /* the input character just read                 */
int pos = 8;            /* where the next output character goes          */
int line_break;         /* the last line break character seen            */
int run;                /* new characters placed so far on this line     */
int use_argument;       /* take new characters from argv[1], not rand()  */

/* bookkeeping for the recursion that replaces the main loop */
int level;              /* how deep the current call is nested           */
int max_level;          /* how deep calls are allowed to nest            */
int nested;             /* the previous call was a nested call           */

int stereogram(char **argv) {
  if (read(0, &in, 1)) {
    if (in < 95) {
      /* line break (\n, \r, or anything that is not a depth letter) */
      if (line_break + 95 > in)
        run = 0;
      line_break = in;
      picture[pos] = in;
    } else {
      int depth = in - 95; /* 'a' is 2, 'z' is 27 */

      if (run++ < depth) {
        /* nothing to repeat yet: make a new character */
        if (use_argument) {
          if (!*pattern)
            pattern = argv[1];
          picture[pos] = *pattern++;
        } else {
          picture[pos] = rand() % 95 + 32;
        }
      } else {
        /* repeat the character that the other eye sees */
        picture[pos] = picture[pos - depth];
        run = depth;
      }
    }
    write(1, picture + pos, 1);
    ++pos;

    /*
     * Instead of a loop the function calls itself five times. The allowed
     * nesting depth (max_level) grows by one every time an outer call gets
     * back control, so the stack only grows logarithmically with the length
     * of the input. A nested call that just read a line break stops early.
     */
    int go_deeper;
    if (nested) {
      go_deeper = level < max_level && run;
      level++;
    } else {
      max_level++;
      nested = 1;
      go_deeper = 1;
    }
    if (go_deeper)
      for (int i = 0; i < 5; i++)
        stereogram(argv);
    level--;
  }
  nested = level > 0;
  return !max_level;
}

int main(int argc, char **argv) {
  use_argument = argc == 2;
  pattern = picture;
  srand(~time(0));
  return stereogram(argv);
}
