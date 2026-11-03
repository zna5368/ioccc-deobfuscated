#include <stdio.h>

/*
 * The current generation lives in the file life.d, which is itself C source.
 * It is compiled straight into this program by the #include below, so the
 * program has to be rebuilt for every generation.  A board looks like this:
 *
 *     LIFE
 *     L _ _ _ _ _
 *     L _ _ O _ _
 *     ...
 *     GEN 1 STAT 329040
 *     END
 *
 * These macros turn the board into a character table where '\n' starts a
 * row, '_' is a dead cell and 'O' is a live cell.  The generation number
 * becomes the variable generation and the optional STAT value becomes stat.
 */
#define LIFE static const char board_text[] = {
#define L '\n',
#define _ '_',
#define O 'O',
#define GEN 0 }; int generation =
#define STAT ; long stat =
#define END ;

/*
 * STAT packs the shape of the board and how to resize it:
 *
 *     bits 16..    number of columns
 *     bits  8..15  number of rows
 *     bits  0..7   resize flags (see below)
 *
 * A board without STAT has stat == 0, which means "do not resize".
 */
long stat;

#include "life.d"

#define STAT_COLUMNS(s) ((s) >> 16)
#define STAT_ROWS(s) (((s) & 0xff00) >> 8)

/* resize flags, each one decided from the edges of the previous board */
#define ADD_BOTTOM(s) ((s) & 1)          /* last row has a live cell */
#define DROP_BOTTOM(s) (((s) & 3) == 2)  /* last two rows are empty */
#define ADD_TOP(s) ((s) & 8)             /* first row has a live cell */
#define DROP_TOP(s) (((s) & 12) == 4)    /* first two rows are empty */
#define ADD_RIGHT(s) ((s) & 16)          /* last column has a live cell */
#define DROP_RIGHT(s) (((s) & 48) == 32) /* last two columns are empty */
#define ADD_LEFT(s) ((s) & 128)          /* first column has a live cell */
#define DROP_LEFT(s) (((s) & 192) == 64) /* first two columns are empty */

/* bits flipped so that the "is empty" tests above read as set bits */
#define STAT_INVERT 0x66

#define MAX_ROWS (sizeof board_text + 2)

/* one row of the resized board */
struct row {
  int source; /* index into source_start[], or -1 for an added blank row */
  int length; /* number of cells in the row */
  int counted; /* width the original reports for the row in STAT */
};

static int source_start[MAX_ROWS]; /* offset of each source row's '\n' */
static int source_length[MAX_ROWS];
static int source_rows;

static struct row rows[MAX_ROWS];
static int row_count;

static int add_left;
static int first_column; /* 1, or 2 when the first column is dropped */

/* Find where every row of the compiled-in board starts. */
static void read_board(void) {
  int i;

  for (i = 0; board_text[i]; i++) {
    if (board_text[i] == '\n') {
      source_start[source_rows] = i;
      source_length[source_rows] = 0;
      source_rows++;
    } else if (source_rows > 0) {
      source_length[source_rows - 1]++;
    }
  }
}

/*
 * Count the cells of a row that has `cells' cells before resizing, after
 * the left and right columns have been dropped as stat asks.
 */
static int kept_cells(int cells) {
  int k, kept = 0;

  for (k = 1; k <= cells; k++) {
    if (k == 1 && DROP_LEFT(stat))
      continue;
    if (DROP_RIGHT(stat) && k >= STAT_COLUMNS(stat))
      continue;
    kept++;
  }
  return kept;
}

/* Append a row of the board, with `left' blank cells added in front. */
static void add_row(int source, int left) {
  int right = ADD_RIGHT(stat) ? 1 : 0;

  rows[row_count].source = source;
  rows[row_count].length = left + kept_cells(source_length[source]) + right;
  rows[row_count].counted = rows[row_count].length;
  row_count++;
}

/*
 * Append a blank row, built from as many cells as the previous board had
 * columns.  Quirk: the original always counts the last of those cells in
 * the reported width, even when the last column is dropped.
 */
static void add_blank_row(int left) {
  int cells = STAT_COLUMNS(stat) > 0 ? STAT_COLUMNS(stat) : 1;
  int right = ADD_RIGHT(stat) ? 1 : 0;

  rows[row_count].source = -1;
  rows[row_count].length = left + kept_cells(cells) + right;
  rows[row_count].counted = left + kept_cells(cells - 1) + 1 + right;
  row_count++;
}

/* Grow or shrink the board on each side as stat asks. */
static void resize_board(void) {
  int r;

  add_left = ADD_LEFT(stat) ? 1 : 0;
  first_column = DROP_LEFT(stat) ? 2 : 1;

  if (ADD_TOP(stat))
    add_blank_row(add_left);
  for (r = 0; r < source_rows; r++) {
    if (r == 0 && DROP_TOP(stat))
      continue;
    if (DROP_BOTTOM(stat) && r + 1 >= STAT_ROWS(stat))
      continue;
    add_row(r, add_left);
  }

  /*
   * Quirk: the original tests the new left cell of a bottom blank row against
   * the column count of the last row, so it is lost when the last column is
   * being dropped.
   */
  if (ADD_BOTTOM(stat))
    add_blank_row(DROP_RIGHT(stat) && source_rows > 0 &&
                          source_length[source_rows - 1] >= STAT_COLUMNS(stat)
                      ? 0
                      : add_left);
}

/* Is the cell at row r, column c (both starting at 1) of the board alive? */
static int cell(int r, int c) {
  int k;
  struct row *row;

  if (r < 1 || r > row_count)
    return 0;
  row = &rows[r - 1];
  if (c < 1 || c > row->length || row->source < 0)
    return 0;
  k = c - add_left + first_column - 1;
  if (k < 1 || k > source_length[row->source])
    return 0;
  return board_text[source_start[row->source] + k] == 'O';
}

/*
 * Live cells among columns c-1, c and c+1 of row r.  When c is past the end
 * of a shorter row, the original sums the last two cells of that row.
 */
static int three_cells(int r, int c) {
  if (r >= 1 && r <= row_count && c > rows[r - 1].length)
    c = rows[r - 1].length;
  return cell(r, c - 1) + cell(r, c) + cell(r, c + 1);
}

/* Conway's rule: born with 3 neighbours, survives with 2 or 3. */
static int next_state(int r, int c) {
  int alive = cell(r, c);
  int neighbours = three_cells(r - 1, c) + three_cells(r, c) +
                   three_cells(r + 1, c) - alive;

  return neighbours == 3 || (alive && neighbours == 2);
}

int main(void) {
  int r, c, alive;
  int edges = 0;          /* edge occupancy, becomes the low STAT byte */
  int row_alive = 0;      /* current row has a live cell */
  int last_row_alive = 0; /* previous row had a live cell */
  int last_two = 0;       /* last two cells of the row, newest in bit 0 */
  int width = 0;

  read_board();
  resize_board();

  printf("LIFE\n");
  for (r = 1; r <= row_count; r++) {
    last_row_alive = row_alive;
    row_alive = 0;
    last_two = 0;
    printf("\nL");

    for (c = 1; c <= rows[r - 1].length; c++) {
      alive = next_state(r, c);
      last_two = (last_two * 2 + alive) & 3;
      row_alive |= alive;
      printf(" %c", "_O"[alive]);
      if (c == 2)
        edges |= last_two << 6; /* first and second column */
    }

    if (r == 1)
      edges |= row_alive << 3; /* first row */
    else if (r == 2)
      edges |= row_alive << 2; /* second row */
    edges |= last_two << 4;    /* last and second-to-last column */
    width = rows[r - 1].counted;
  }

  /* last row and second-to-last row */
  edges |= row_alive | last_row_alive << 1;

  printf("\n\nGEN %d STAT %ld\nEND\n", ++generation,
         (long)width << 16 | row_count << 8 | (edges ^ STAT_INVERT));
  return 0;
}
