#include <stdio.h>

/*
 * The secret message is hidden inside the text of the original source file
 * cdupont.c.  The first 26 lines of that file are loaded into a 26 x 26 grid
 * of characters.  Starting from a fixed cell, each printed letter together
 * with a second "key" letter decides the grid coordinates of the next
 * letter, until a '.' is reached.
 */

#define GRID 26      /* the grid is 26 x 26 characters */
#define SLOTS 80     /* room reserved for the text */
#define LINE_LEN 80  /* fgets() may read up to 79 characters per line */

/*
 * Number of letters in each word of the message.  A space is printed after
 * each of these words; the last word ("scnc.") has no space after it.
 * (In the original the first length is the lowest bit of 0x5765248d and the
 * rest are its octal digits 12731222215, read from the left.)
 */
static const int word_lengths[] = {1, 1, 2, 7, 3, 1, 2, 2, 2, 2, 1, 5};
#define WORDS (int)(sizeof(word_lengths) / sizeof(word_lengths[0]))

main() {
  /*
   * Each line is stored GRID bytes after the previous one, but up to
   * LINE_LEN bytes are read, so a long line spills over into the following
   * slots until the next line overwrites them.  The grid cell (row, col) is
   * therefore text[row * GRID + col].
   */
  char text[SLOTS * GRID];
  FILE *source;
  int row = 4, col = 10;  /* coordinates of the first letter */
  int letter, key;
  int step;
  int word = 0, letters_in_word = 1;

  source = fopen("cdupont.c", "r");
  for (step = 0; step < GRID; step++)
    fgets(text + step * GRID, LINE_LEN, source);
  fclose(source);

  for (step = 0;; step++) {
    letter = text[row * GRID + col];
    if (col == row) {
      /* cells on the diagonal are replaced by a fixed letter and key */
      letter = 'p';
      key = 'h';
    } else {
      /* the key is found at the mirrored position */
      key = text[col * GRID + row];
    }
    putchar(letter);

    if (word < WORDS && letters_in_word++ == word_lengths[word]) {
      letters_in_word = 1;
      putchar(' ');
      word++;
    }

    /* the letter and key give the coordinates of the next letter */
    col = (step + 1) * (4 * letter + 3 * key) % GRID;
    row = (step + 1) * (letter + 2 * key) % GRID;

    if (letter == '.')
      break;
  }
  putchar('\n');
}
