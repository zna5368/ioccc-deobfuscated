/*
 * The keyboard picture. Every character position 0..60 that the original
 * program visits is listed here: letters mark the keys, spaces and
 * newlines are printed as-is. Position 0 is only reached when no word is
 * given (argc == 1).
 */
static const char keyboard[] = "\n"
                               "q w e r t y u i o p\n"
                               " a s d f g h j k l \n"
                               "  z x c v b n m    \n";

#define LAST_POSITION 60 /* the final newline */

int main(int argc, char **argv)
{
  char *word = argv[1];
  int position;

  /* the original starts drawing at position argc - 1 */
  for (position = argc - 1; position <= LAST_POSITION; position++) {
    char key = keyboard[position];

    if (key >= 'a' && key <= 'z') {
      /* a key: 'o' if the letter was typed, '.' otherwise */
      putchar(strchr(word, key) ? 'o' : '.');
    } else {
      putchar(key);
    }
  }
  return 0;
}
