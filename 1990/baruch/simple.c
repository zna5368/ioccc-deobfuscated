/*
 * N-queens solver: reads the board size N and prints every way to place
 * N queens on an NxN board so that no two queens attack each other.
 *
 * Like the original, no headers are included; scanf, printf and puts are
 * implicitly declared.
 */

int size;           /* N: board width/height read from stdin */
int queen_col[99];  /* queen_col[r] = column of the queen in row r */

/* Is the queen on 'row' safe from every queen on the rows above it? */
int is_safe(int row) {
  int r;
  for (r = 0; r < row; r++) {
    int diff = queen_col[r] - queen_col[row];
    if (diff == 0 || diff == row - r || diff == r - row)
      return 0; /* same column or same diagonal */
  }
  return 1;
}

/* Print the board: ' ' and '#' make a checkerboard, 'Q' is a queen. */
void print_board(void) {
  int row, col;
  for (row = 0; row < size; row++) {
    for (col = 0; col < size; col++) {
      if (col == 0)
        printf(row == 0 ? "\n\n" : "\n"); /* blank line before each board */
      printf("%c", " #Q"[col == queen_col[row] ? 2 : (col ^ row) & 1]);
    }
  }
}

int main() {
  int row = 0;

  scanf("%d", &size);

  /* Iterative backtracking: stop once the first queen runs off the board. */
  while (queen_col[0] != size) {
    if (row == size) {
      /* every row has a queen: show it, then look for the next solution */
      print_board();
      row--;
      queen_col[row]++;
    } else if (queen_col[row] >= size) {
      /* no column left on this row: backtrack to the previous row */
      row--;
      queen_col[row]++;
    } else if (is_safe(row)) {
      /* queen fits: move to the next row starting at column 0 */
      row++;
      queen_col[row] = 0;
    } else {
      /* attacked: try the next column */
      queen_col[row]++;
    }
  }
  puts("");
}
