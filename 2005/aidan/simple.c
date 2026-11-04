#include <stdio.h>
#include <stdlib.h>

/*
 * Sudoku solver and generator.
 *
 * Each cell is stored in one int:
 *   bit 0       "tried" flag used by the generator
 *   bits 1..9   the digits that are still possible in the cell (candidates)
 *   bits 10+    the solved digit (0 when the cell is still unknown)
 */
#define CELLS 81
#define ALL_CANDIDATES 0x3fe /* bits 1..9 */
#define TRIED_FLAG 1
#define VALUE(cell) ((cell) >> 10)
#define GIVEN(digit) ((digit) ? 1 << (digit) | (digit) << 10 : ALL_CANDIDATES)

/* bits of the program mode */
#define MODE_GENERATE 1 /* the grid starts empty and a seed is given */
#define MODE_UNIQUE 2   /* keep searching after a solution to prove it is the only one */
#define MODE_USER 4     /* the "U" argument asked for the uniqueness check */

enum unit_kind { ROW, COLUMN, BOX };
static const char *unit_names[] = {"row", "col", "box"};

/* one level of the guessing stack */
struct frame {
  int cells[CELLS];
  int guess_cell;  /* cell that was guessed (generator: the removed cell) */
  int guess_digit; /* digit that was guessed (generator: scan start cell) */
  int start_digit; /* first digit tried, 0 = no guess made yet
                      (generator: digit of the removed cell) */
};

#define SAVED 82 /* frame where the generator keeps its puzzle */
struct frame stack[83];
int depth = 0, max_depth = 0, mode;
long seed;

static const char separator[] = "\n------+-------+------\n";

/* linear congruential generator, returns a number in 0..80 */
static int next_random(void) {
  seed = 1103515245 * seed + 12345 & 0x7fffffff;
  return (int)(81.0 * seed / 2147483648.0);
}

static int unit_of(int kind, int cell) {
  switch (kind) {
  case ROW:
    return cell / 9;
  case COLUMN:
    return cell % 9;
  default:
    return cell % 9 / 3 + cell / 27 * 3;
  }
}

static void print_cell(int cell, int digit) {
  int count = cell + 1;
  printf("%c %s", digit ? digit | '0' : '.',
         count % 3    ? ""
         : count % 9  ? "| "
         : count % 27 ? "\n"
         : count % 81 ? separator
                      : "\n");
}

/*
 * Remove the digits already placed in a row/column/box from the candidates of
 * the other cells in it.  Returns 0 if the grid is impossible.
 */
static int eliminate(int *cells, int kind) {
  int unused[9], cell, unit;

  for (unit = 0; unit < 9; unit++)
    unused[unit] = ALL_CANDIDATES;

  for (cell = 0; cell < CELLS; cell++) {
    unit = unit_of(kind, cell);
    if (!(cells[cell] & unused[unit]))
      goto contradiction;
    unused[unit] &= ~(1 << VALUE(cells[cell]));
  }
  for (cell = 0; cell < CELLS; cell++) {
    unit = unit_of(kind, cell);
    if (!VALUE(cells[cell]))
      cells[cell] &= unused[unit];
    if (!cells[cell])
      goto contradiction;
  }
  return 1;

contradiction:
  if (depth == 0)
    printf("!!! %s %i\n", unit_names[kind], unit + 1);
  return 0;
}

/*
 * Place a digit in every row/column/box where it has only one possible cell.
 * Returns the number of digits placed.
 */
static int hidden_singles(int *cells, int digit, int kind) {
  int where[9], cell, unit, placed = 0;

  for (unit = 0; unit < 9; unit++)
    where[unit] = -1; /* -1 = nowhere, -2 = several cells */

  for (cell = 0; cell < CELLS; cell++)
    if (cells[cell] >> digit & 1) {
      unit = unit_of(kind, cell);
      where[unit] = where[unit] == -1 ? cell : -2;
    }

  for (unit = 0; unit < 9; unit++)
    if (where[unit] >= 0 && !VALUE(cells[where[unit]])) {
      cells[where[unit]] = GIVEN(digit);
      placed++;
    }
  return placed;
}

/*
 * Guess a digit for the first unknown cell and push a new frame.  A frame
 * that already guessed carries on with the next digit after its last guess.
 * Digits are tried in a circle starting at a random digit.  Returns 0 when
 * every digit of the cell has been tried.
 */
static int guess(void) {
  struct frame *top = &stack[depth];
  int wrapped = top->start_digit;
  int cell = wrapped ? top->guess_cell : 0;
  int digit = wrapped ? top->guess_digit % 9 + 1
                      : (top->start_digit = next_random() % 9 + 1);

  for (; cell < CELLS; cell++, digit = top->start_digit, wrapped = 0) {
    if (VALUE(top->cells[cell]))
      continue;
    for (;; digit = digit % 9 + 1, wrapped = 1) {
      if (wrapped && digit == top->start_digit)
        return 0;
      if (top->cells[cell] >> digit & 1) {
        top->guess_digit = digit;
        top->guess_cell = cell;
        depth++;
        stack[depth] = stack[depth - 1];
        stack[depth].cells[cell] = GIVEN(digit);
        if (depth > max_depth)
          max_depth = depth;
        return 1;
      }
    }
  }
  return 1;
}

/*
 * Solve the grid on top of the stack.  Returns 1 when a solution was found
 * (with MODE_UNIQUE: when exactly one solution exists), otherwise 0.
 */
static int solve(void) {
  for (;;) {
    int *cells = stack[depth].cells;
    int cell, digit, solved = 0, progress = 0;

    if (!eliminate(cells, ROW) || !eliminate(cells, COLUMN) ||
        !eliminate(cells, BOX)) {
      /* contradiction: undo the last guess and forbid that digit */
      if (depth == 0) {
        mode = 0;
        return 0;
      }
      depth--;
      stack[depth].cells[stack[depth].guess_cell] &=
          ~(1 << stack[depth].guess_digit);
      continue;
    }

    /* naked singles: cells with only one candidate left */
    for (cell = 0; cell < CELLS; cell++) {
      int bits = cells[cell] & ALL_CANDIDATES;

      if (VALUE(cells[cell])) {
        solved++;
        continue;
      }
      for (digit = 0; !(bits & 1) && (bits /= 2); digit++)
        ;
      if (bits == 1) {
        cells[cell] |= digit << 10;
        progress++;
      }
    }

    if (solved == CELLS) {
      /* in uniqueness mode go back and try the other guesses */
      if (depth && mode & MODE_UNIQUE) {
        depth--;
        continue;
      }
      return 1;
    }

    for (digit = 1; digit <= 9; digit++) {
      progress += hidden_singles(cells, digit, ROW);
      progress += hidden_singles(cells, digit, COLUMN);
      progress += hidden_singles(cells, digit, BOX);
    }

    if (!progress && !guess())
      return 0;
    stack[depth].start_digit = 0;
  }
}

static void print_result(int ok) {
  int cell;

  printf("\n\n");
  for (cell = 0; cell < CELLS; cell++)
    print_cell(cell, VALUE(stack[depth].cells[cell]));
  printf("\n%s (stk %i %i)\n", ok ? "Done" : "No way!", depth, max_depth);
}

/*
 * Turn the solved grid in stack[SAVED] into a puzzle by removing digits as
 * long as the puzzle keeps exactly one solution.
 */
static int remove_digits(void) {
  struct frame *puzzle = &stack[SAVED];
  int cell, first;

restart:
  depth = SAVED;
  puzzle->guess_digit = cell = next_random();
  for (first = 1; first || cell != puzzle->guess_digit; cell = (cell + 1) % 81) {
    first = 0;
    puzzle->start_digit = VALUE(puzzle->cells[cell]);
    if (puzzle->cells[cell] & TRIED_FLAG)
      continue;

    /* try removing the digit of this cell */
    puzzle->cells[cell] = ALL_CANDIDATES | TRIED_FLAG;
    puzzle->guess_cell = cell;
    stack[0] = *puzzle;
    max_depth = depth = 0;
    mode = MODE_UNIQUE;
    stack[0].start_digit = 0;

    if (solve())
      goto restart; /* still unique: start again at another random cell */
    if (mode == 0)
      return 0;

    /* more than one solution: put the digit back */
    depth = SAVED;
    puzzle->cells[cell] = GIVEN(puzzle->start_digit) | TRIED_FLAG;
  }
  mode = 0;
  return 1;
}

int main(int argc, char **argv) {
  int cell, ok;

  mode = argc > 1 ? *argv[1] != 'U' ? MODE_GENERATE : MODE_UNIQUE | MODE_USER
                  : 0;
  seed = mode & MODE_GENERATE ? atol(argv[1]) : 123;

  /* read the puzzle ('.' or '0' for an empty cell), a generated one is empty */
  for (cell = 0; cell < CELLS;) {
    /* '0'..'9' become 0..9, other characters 10 or more, EOF becomes -49 */
    int digit = mode & MODE_GENERATE ? 0 : getchar() ^ '0';

    next_random();
    if (digit == ('.' ^ '0'))
      digit = 0;
    if (digit < 10) {
      stack[0].cells[cell] = GIVEN(digit);
      print_cell(cell++, digit);
    }
  }

  ok = solve();
  print_result(ok);
  if (mode != MODE_GENERATE || !ok)
    return !ok;

  /* generator: make a puzzle out of the solution */
  stack[SAVED] = stack[depth];
  ok = remove_digits();
  print_result(ok);
  return !ok;
}
