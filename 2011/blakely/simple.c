#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Conway's Game of Life on a square grid read from standard input.
 *
 * With no arguments the grid is evolved forwards forever.  With any argument
 * the program searches backwards in time for a grid that evolves into the
 * current one, stopping when no such grid exists.
 *
 * Cells are stored row by row in one flat array.  A cell's eight neighbours
 * are found at offsets -size-1, -size, -size+1, -1, +1, +size-1, +size and
 * +size+1.  Cells on the outer border are always killed before each step.
 */

#define DEAD ' '   /* 32: even, so it does not count as a live neighbour */
#define ALIVE '#'  /* 35: odd, so it counts as a live neighbour */
#define UNKNOWN 2  /* even: a cell of the trial grid not chosen yet */

#define MAX_CELLS 9802

int size;      /* width (and height) of the square grid */
int grid_end;  /* number of cells + 1 (the extra slot held the EOF byte) */
int neighbors; /* live neighbour count from the last count_neighbors() */

/*
 * Both grids share one buffer with next first: the neighbour count for the
 * top row reads a little before the start of grid, which lands in the
 * (always zero, i.e. dead) unused tail of next.
 */
char memory[2 * MAX_CELLS];
char *next = memory;              /* next generation / trial previous one */
char *grid = memory + MAX_CELLS;  /* current generation */

/*
 * Print a row of dashes, then every row of the grid.  The rightmost column
 * is never shown because its position is used for the newline.
 */
void print_grid(void) {
  int i;

  for (i = 0; i <= grid_end + size - 1; i++) {
    char ch;

    fflush(stdout);
    if (i < size)
      ch = '-';
    else if (i == size)
      ch = '\n';
    else if (i % size == 0)
      ch = '\n';
    else
      ch = grid[i - size - 1];
    printf("%c", ch);
  }
}

/* Count the live (odd valued) neighbours of cell i in grid g. */
int count_neighbors(char *g, int i) {
  char *cell = g + i;

  neighbors = cell[-size - 1] % 2 + cell[-size] % 2 + cell[-size + 1] % 2 +
              cell[-1] % 2 + cell[1] % 2 + cell[size - 1] % 2 +
              cell[size] % 2 + cell[size + 1] % 2;
  return neighbors;
}

/* The rules of Life: born with 3 neighbours, survives with 2 or 3. */
int lives_next(int alive, int count) {
  return count == 3 || (count == 2 && alive);
}

/* Forwards: compute the next generation of grid into next. */
void life_step(void) {
  int i;

  for (i = 1; i <= grid_end - 1; i++) {
    count_neighbors(grid, i);
    next[i] = lives_next(grid[i] != DEAD, neighbors) ? ALIVE : DEAD;
  }
}

/* How many combinations of the not yet chosen cells to try at cell i. */
int choice_count(int i) {
  if (i - i / size + 2 == size)
    return 1;
  if (i > 2 * size)
    return (i % size != 1) ? 2 : 4;
  return 8;
}

/*
 * Backwards: find a trial grid in next that evolves into grid, working
 * through the cells from position i onwards.  Returns 1 when one is found.
 *
 * The cells before i, and the cells right of / below cell i chosen by
 * earlier cells, are already decided.  Cell i decides the still undecided
 * neighbours to its right (first interior row only), below (first interior
 * column only) and below-right, so that its own neighbourhood is complete
 * and can be checked against the wanted state grid[i].
 */
int search(int i) {
  int choice, col, live;
  int set_right, set_below, set_below_right;

  if (i == grid_end - 1)
    return 1;

  /* Only the very first interior cell starts undecided: try dead, then alive */
  if (next[i] == UNKNOWN) {
    next[i] = DEAD;
    if (search(i))
      return 1;
    next[i] = ALIVE;
    if (search(i))
      return 1;
    return 0;
  }

  col = i % size;
  if (col == 0 || col == size - 1)
    return search(i + 1);

  for (choice = 0; choice < choice_count(i); choice++) {
    live = count_neighbors(next, i);

    if (i < grid_end - 2 * size - 1) {
      set_below = (col == 1);
      set_below_right = set_below ? 1 : col != size - 2;
      set_right = set_below_right && i < 2 * size;

      /* add the live cells this choice would create */
      live += (set_right ? (choice & 4) >> 2 : 0) +
              (set_below ? (choice & 2) >> 1 : 0) +
              (set_below_right ? (choice & 1) : 0);
    } else {
      set_right = set_below = set_below_right = 0;
    }

    if (lives_next(next[i] % 2, live) != (grid[i] == ALIVE))
      continue;

    if (set_below)
      next[i + size] = (choice & 2) ? ALIVE : DEAD;
    if (set_right)
      next[i + 1] = (choice & 4) ? ALIVE : DEAD;
    if (set_below_right)
      next[i + size + 1] = (choice & 1) ? ALIVE : DEAD;

    if (search(i + 1))
      return 1;

    /* undo the choice (cells not chosen reset the unused corner cell 0) */
    next[set_below_right ? i + size + 1 : 0] = UNKNOWN;
    next[set_below ? i + size : 0] = UNKNOWN;
    next[set_right ? i + 1 : 0] = UNKNOWN;
  }
  return 0;
}

/* Kill every cell on the outer border of both grids. */
void kill_borders(void) {
  int i;

  for (i = 0; i <= grid_end - 1; i = i + size) {
    int row = i / size;
    int left_of = (i == 0) ? 0 : i - 1; /* right edge of the previous row */

    grid[row] = DEAD;                   /* top row */
    grid[grid_end - size + row] = DEAD; /* bottom row */
    grid[i] = DEAD;                     /* left column */
    grid[left_of] = DEAD;               /* right column */
    next[row] = DEAD;
    next[grid_end - size - 1 + row] = DEAD;
    next[i] = DEAD;
    next[left_of] = DEAD;
  }
}

int main(int argc, char **argv) {
  /* Read the grid: newlines count rows, any non-space is a live cell */
  grid_end = size = 0;
  while ((grid[grid_end++] = getchar()) != EOF) {
    if (grid[grid_end - 1] == '\n') {
      size += 1;
      grid_end -= 1;
    }
    if (grid[grid_end - 1] != DEAD)
      grid[grid_end - 1] = ALIVE;
  }
  print_grid();

  for (;;) {
    memset(next, UNKNOWN, grid_end * sizeof(char));
    kill_borders();

    if (argc == 1)
      life_step();
    else if (search(size + 1) == 0)
      return 1;

    /* the top-left corner is shifted out: 32 becomes 0, 2 becomes 32 */
    next[0] <<= 4;
    memcpy(grid, next, grid_end * sizeof(char));
    print_grid();
    sleep(1);
  }
  return 0;
}
