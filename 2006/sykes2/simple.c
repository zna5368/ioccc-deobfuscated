/*
 * Print the compile time (__TIME__, "HH:MM:SS") as large seven segment
 * digits built from '!' characters.
 *
 * Each of the 8 characters of the time occupies a cell 8 columns wide and
 * 7 rows tall. A cell is split into 4 blocks, each 2 columns wide, and every
 * block belongs to one segment (or to none, which is always blank).
 */

#define ROWS 7
#define COLUMNS 64 /* 8 cells of 8 columns; the last column is the newline */
#define CELL_WIDTH 8
#define BLOCK_WIDTH 2

/* segment bits */
#define NONE 0
#define TOP 0x01
#define UPPER_LEFT 0x02
#define LOWER_LEFT 0x04
#define LOWER_RIGHT 0x08
#define BOTTOM 0x10
#define UPPER_RIGHT 0x20
#define MIDDLE 0x40

/* which segment each 2 column block of a cell belongs to, row by row */
static const int layout[ROWS][CELL_WIDTH / BLOCK_WIDTH] = {
    {TOP, TOP, UPPER_RIGHT, NONE},
    {UPPER_LEFT, NONE, UPPER_RIGHT, NONE},
    {UPPER_LEFT, NONE, UPPER_RIGHT, NONE},
    {UPPER_LEFT, MIDDLE, UPPER_RIGHT, NONE},
    {LOWER_LEFT, NONE, LOWER_RIGHT, NONE},
    {LOWER_LEFT, NONE, LOWER_RIGHT, NONE},
    {BOTTOM, BOTTOM, LOWER_RIGHT, NONE},
};

/* segments lit for '0' .. '9' and ':' (which comes right after '9') */
static const int segments[] = {
    TOP | UPPER_LEFT | LOWER_LEFT | LOWER_RIGHT | BOTTOM | UPPER_RIGHT,
    LOWER_RIGHT | UPPER_RIGHT,
    TOP | LOWER_LEFT | BOTTOM | UPPER_RIGHT | MIDDLE,
    TOP | LOWER_RIGHT | BOTTOM | UPPER_RIGHT | MIDDLE,
    UPPER_LEFT | LOWER_RIGHT | UPPER_RIGHT | MIDDLE,
    TOP | UPPER_LEFT | LOWER_RIGHT | BOTTOM | MIDDLE,
    TOP | UPPER_LEFT | LOWER_LEFT | LOWER_RIGHT | BOTTOM | MIDDLE,
    TOP | LOWER_RIGHT | UPPER_RIGHT,
    TOP | UPPER_LEFT | LOWER_LEFT | LOWER_RIGHT | BOTTOM | UPPER_RIGHT | MIDDLE,
    TOP | UPPER_LEFT | LOWER_RIGHT | BOTTOM | UPPER_RIGHT | MIDDLE,
    MIDDLE,
};

main(argc, argv) char **argv;
{
  int row, column;

  for (row = 0; row < ROWS; row++) {
    for (column = 0; column < COLUMNS - 1; column++) {
      char symbol = __TIME__[column / CELL_WIDTH];
      int block = column % CELL_WIDTH / BLOCK_WIDTH;
      int lit = segments[symbol - '0'] & layout[row][block];

      putchar(lit ? '!' : ' ');
    }
    putchar('\n');
  }
}
