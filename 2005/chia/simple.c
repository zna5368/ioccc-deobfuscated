/*
 * Draws a Koch snowflake on the terminal with ANSI escape sequences.
 *
 * The drawing is done by a "turtle" that walks across a grid where every
 * terminal cell is split into two half-rows (top and bottom). Each visited
 * half-cell is remembered so the character printed in a cell can show the
 * top half ('`'), the bottom half ('.') or both (':').
 */

#define DIRECTIONS 6 /* the turtle turns in steps of 60 degrees */
#define RECURSION_DEPTH 4

/* positions are fixed point numbers: column * 16 and half-row * 16 */
int direction = 0;
int column = 328; /* column 20 (* 16 + 8) */
int row = 192;    /* half-row 12 (terminal row 6) */
int depth = RECURSION_DEPTH;
int visited[80][64] = {0}; /* [column][half-row] */

/* step taken in each direction, in sixteenths of a column / half-row */
int column_step[DIRECTIONS] = {8, 4, -4, -8, -4, 4};
int row_step[DIRECTIONS] = {0, -7, -7, 0, 7, 7};

/* counter used by the busy wait delay, wraps around from 0 to 0 */
short delay_counter;

void print(char *text) {
  int i;

  write(1, text, strlen(text));

  /* slow down so the drawing is animated */
  for (i = 0; i < 4; i++)
    while (--delay_counter)
      ;
}

void escape(char *code) {
  write(1, "\033[", 2);
  print(code);
}

/* take one step in the current direction and plot it */
void step(void) {
  int new_column = column + column_step[direction];
  int new_row = row + row_step[direction];
  int new_cell_column = new_column / 16;
  int new_half_row = new_row / 16;

  ++visited[new_cell_column][new_half_row];

  /* move the cursor left or right */
  if (column / 16 < new_cell_column)
    escape("C");
  else if (column / 16 > new_cell_column)
    escape("D");
  column = new_column;

  /* move the cursor up or down (a terminal row is two half-rows) */
  if (row / 32 < new_row / 32)
    escape("B");
  else if (row / 32 > new_row / 32)
    escape("A");
  row = new_row;

  /* draw the cell and then put the cursor back on it */
  if (visited[new_cell_column][new_half_row ^ 1])
    print(":\b"); /* both halves of the cell are filled */
  else if (new_half_row & 1)
    print(".\b"); /* bottom half */
  else
    print("`\b"); /* top half */
}

/* turn by turn * 60 degrees, then draw one Koch curve */
void koch(int turn) {
  direction = (direction + turn) % DIRECTIONS;

  if (depth--) {
    koch(0); /* straight */
    koch(1); /* left 60 degrees */
    koch(4); /* right 120 degrees */
    koch(1); /* left 60 degrees */
  } else {
    step();
  }
  ++depth;
}

int main() {
  escape("2J");    /* clear the screen */
  escape("7;21H"); /* move the cursor to row 7, column 21 */

  /* three sides of the snowflake, turning 120 degrees between them */
  koch(0);
  koch(4);
  koch(4);

  escape("H"); /* move the cursor to the top left corner */
}
