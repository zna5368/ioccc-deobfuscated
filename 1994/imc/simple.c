#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Every square maker fills an n x n square with a permutation of 0 .. n*n-1
 * whose rows, columns and both diagonals add up to the same total.
 *
 *   square  where the square is written (row by row)
 *   work    scratch space of at least n ints
 *   gap     number of ints to skip between the end of one row and the
 *           start of the next (lets a maker fill a quadrant of a bigger square)
 */
typedef void (*square_maker)(int n, int *square, int *work, int gap);

static void odd_square(int n, int *square, int *work, int gap);
static void four_square(int n, int *square, int *work, int gap);
static void even_square(int n, int *square, int *work, int gap);

/* random integer in the range 0 .. limit-1 */
static int random_below(int limit) { return (rand() / 256) % limit; }

/* Fisher-Yates shuffle of values[0 .. count-1] */
static void shuffle(int *values, int count)
{
  int i, j, tmp;

  for (i = count - 1; i > 0; i--) {
    j = random_below(i + 1);
    tmp = values[i];
    values[i] = values[j];
    values[j] = tmp;
  }
}

static square_maker choose_maker(int n)
{
  if (n == 4)
    return four_square;
  return n % 2 ? odd_square : even_square;
}

/*
 * Odd order n >= 3.
 *
 * The square is the sum of two "Latin" squares: a digit square that is
 * constant along the down-right diagonals and one that is constant along
 * the down-left diagonals.  One digit square is multiplied by n, the other
 * by 1, so every cell gets a unique number high*n + low.  The middle digit
 * is fixed at the end of the digit list so it lands on both main diagonals,
 * the other digits are shuffled for a random square.
 */
static void odd_square(int n, int *square, int *work, int gap)
{
  int middle = n / 2;
  int first_weight = random_below(2) ? n : 1;
  int second_weight = n + 1 - first_weight;
  int digit, count, row, col;
  int *cells;

  /* all digits 0 .. n-1 except the middle one, in descending order */
  count = 0;
  for (digit = n - 1; digit >= 0; digit--)
    if (digit != middle)
      work[count++] = digit;

  shuffle(work, n - 1);
  work[n - 1] = middle;
  for (row = 0; row < n; row++) {
    cells = square + row * (n + gap);
    for (col = 0; col < n; col++)
      cells[col] = work[(col + row) % n] * first_weight;
  }

  shuffle(work, n - 1);
  for (row = 0; row < n; row++) {
    cells = square + row * (n + gap);
    for (col = 0; col < n; col++)
      cells[col] += work[(n - 1 + col - row) % n] * second_weight;
  }
}

/*
 * Order 4 only.
 *
 * Also the sum of two digit squares (digits 0..3, weights 1 and 4).  The
 * digit pattern is built with exclusive-or on the row and column numbers;
 * the digits come in pairs that add up to 3 so every line has the same sum.
 */
static void four_square(int n, int *square, int *work, int gap)
{
  int pass, row, col, flip = 0, rev_row, rev_col, digit;
  int *cells;

  for (pass = 0; pass < 2; pass++) {
    work[0] = random_below(n);
    work[1] = 3 - work[0];
    work[2] = 1 ^ work[random_below(2)];
    work[3] = 3 - work[2];
    flip = pass == 0 ? 3 * random_below(2) : 3 - flip;

    for (row = 0; row < n; row++) {
      cells = square + row * (n + gap);
      rev_row = n - 1 - row;
      for (col = 0; col < n; col++) {
        rev_col = n - 1 - col;
        digit = work[rev_col ^ (rev_row & flip) ^ (rev_row / 2)];
        if (pass == 0)
          cells[col] = digit;
        else
          cells[col] += digit * 4;
      }
    }
  }
}

/* move the set bit at mask[from] to the highest clear slot (not skip) */
static void move_to_highest_clear(int *mask, int size, int from, int skip)
{
  int j = size;

  do
    j--;
  while (mask[j] || j == skip);
  mask[j] = 1;
  mask[from] = 0;
}

/* return the highest set slot of mask (not skip) */
static int highest_set(int *mask, int size, int skip)
{
  int j = size;

  do
    j--;
  while (!mask[j] || j == skip);
  return j;
}

/*
 * Even order n >= 6.
 *
 * Builds a magic square of order half = n/2 in each of the top two
 * quadrants, then derives the other values quadrant by quadrant:
 *
 *      A          C = A' + 2q          (q = half*half)
 *      D = A + 3q B = C - q
 *
 * Each quadrant is magic except the sums are off between top and bottom.
 * This is fixed by swapping, in every column of the top half, a chosen set
 * of cells with the cell half a square below.  A random mask selects the
 * columns to swap in each row, while two counters make sure the right
 * number of main diagonal and anti diagonal cells are swapped too.
 */
static void even_square(int n, int *square, int *work, int gap)
{
  int half = n / 2;
  int area = half * half;
  int stride = n + gap;
  int lower = stride * half; /* offset to the same cell in the bottom half */
  int *mask = work;
  int side, row, col, diag, anti, j, tmp;
  int diag_left, anti_left;
  int *cells;
  square_maker make_quadrant = choose_maker(half);

  make_quadrant(half, square, work, half + gap);
  make_quadrant(half, square + half, work, half + gap);

  for (row = 0; row < half; row++) {
    cells = square + row * stride;
    for (col = 0; col < n; col++) {
      if (col >= half)
        cells[col] += 2 * area;
      cells[col + lower] = cells[col] + (col >= half ? -1 : 3) * area;
    }
  }

  for (j = 0; j < half; j++)
    mask[j] = j < half / 2;
  diag_left = anti_left = (half + 1) / 2;

  for (side = 0; side < 2; side++) {
    for (row = 0; row < half; row++) {
      cells = square + side * half + row * stride;
      diag = row;            /* column on the main diagonal */
      anti = half - 1 - row; /* column on the anti diagonal (rows left) */

      shuffle(mask, half);

      /* make the main diagonal cell swap only while it is still needed */
      if (mask[diag]) {
        if (diag_left)
          diag_left--;
        else
          move_to_highest_clear(mask, half, diag, -1);
      } else if (diag_left > anti) {
        j = highest_set(mask, half, -1);
        diag_left--;
        mask[diag] = 1;
        mask[j] = 0;
      }

      /* same for the anti diagonal, never disturbing the diagonal cell */
      if (mask[anti]) {
        if (anti_left)
          anti_left--;
        else
          move_to_highest_clear(mask, half, anti, diag);
      } else if (anti_left > anti ||
                 (half == 3 && anti_left &&
                  (diag_left == 2 || anti == diag))) {
        j = highest_set(mask, half, diag);
        anti_left--;
        if (anti == diag)
          diag_left--;
        mask[anti] = 1;
        mask[j] = 0;
      }

      for (col = 0; col < half; col++)
        if (mask[col]) {
          tmp = cells[col];
          cells[col] = cells[col + lower];
          cells[col + lower] = tmp;
        }
    }

    diag_left = anti_left = half / 2 - half % 2;
    for (j = 0; j < half; j++)
      mask[j] = j < diag_left;
  }
}

int main(int argc, char **argv)
{
  int n, i, *square, *work;

  srand(getpid());
  if (argc < 2 || sscanf(argv[1], "%d", &n) != 1 || n < 3)
    n = 5;

  square = (int *)malloc(n * n * sizeof(int));
  work = square ? (int *)malloc(n * sizeof(int)) : NULL;
  if (!square || !work) {
    perror("malloc");
    return 1;
  }

  choose_maker(n)(n, square, work, 0);

  for (i = 0; i < n * n; i++)
    printf("%4d%c", square[i] + 1, (i + 1) % n ? ' ' : '\n');
  return 0;
}
