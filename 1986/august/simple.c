/*
 * Lazily evaluated streams of integers stored in a linked list of cells.
 *
 * e = 2 + 1/2! + 1/3! + 1/4! + ... which in the mixed radix (factorial)
 * number system is written 2.1111... where the n-th fractional digit has
 * base n+1.  Each decimal digit is produced by multiplying the fractional
 * digits by 10 and normalising them (propagating carries from right to left).
 * The integer part that falls out is the next decimal digit.
 *
 * A cell is either an already computed value or a suspended computation
 * (a thunk) which, when forced, fills in its value and its next pointer.
 */
typedef struct cell {
  int kind : 3, value : 29;
  struct cell *next;
} cell;

enum {
  EVALUATED,    /* value and next are already known */
  TIMES_TEN,    /* the stream 'next', with each element multiplied by 10 */
  DIGITS,       /* decimal digits of the mixed radix number at 'next' */
  NORMALIZE     /* carry normalise 'next', 'value' is the base of digit 2 */
};

cell *new_cell();
void force();

char *sbrk();

/*
 * TIMES_TEN: first element is 10 * the head of the input stream, the rest
 * is another TIMES_TEN cell over the remainder of the input.
 */
void times_ten(cell *self) {
  cell *input = self->next;
  cell *rest;

  force(input);
  self->value = input->value * 10;

  rest = new_cell();
  rest->kind = TIMES_TEN;
  rest->next = input->next;
  self->next = rest;
}

/*
 * DIGITS: the input is a mixed radix number [integer, d1, d2, ...] where
 * digit dn has base n+1.  Output the integer part, then continue with the
 * digits of 10 * fraction, i.e. NORMALIZE(base 2)[0, 10*d1, 10*d2, ...].
 */
void digits(cell *self) {
  cell *input = self->next;
  cell *fraction;
  cell *normal, *zero, *scaled;

  force(input);
  self->value = input->value;
  fraction = input->next;

  /* reuse the input head as the DIGITS cell for the next decimal digit */
  input->kind = DIGITS;

  normal = new_cell();
  normal->kind = NORMALIZE;
  normal->value = 2;
  input->next = normal;

  zero = new_cell();
  zero->value = 0;
  normal->next = zero;

  scaled = new_cell();
  scaled->kind = TIMES_TEN;
  scaled->next = fraction;
  zero->next = scaled;
}

/*
 * NORMALIZE: the input is [high, low, ...] where low has base 'self->value'.
 * Produce high plus the carry out of low, followed by low modulo its base.
 * Digits further right are normalised lazily by turning the 'high' cell
 * into a NORMALIZE cell for the next base.
 */
void normalize(cell *self) {
  cell *high = self->next;
  cell *low = high->next;
  cell *digit;
  int base, amount;

  force(low);
  base = self->value;
  amount = low->value;
  self->value = high->value;

  high->kind = NORMALIZE;
  high->value = base + 1;

  /*
   * A carry from further right adds at most 9, so if that can not reach
   * the base the high digit is already final.
   */
  if (amount + 9 >= base) {
    force(high);
    amount = high->value;
    self->value += amount / base;

    digit = new_cell();
    digit->value = amount % base;
    digit->next = high->next;
    self->next = digit;
  }
}

void force(cell *thunk) {
  switch (thunk->kind) {
  case EVALUATED:
    break;
  case TIMES_TEN:
    times_ten(thunk);
    break;
  case DIGITS:
    digits(thunk);
    break;
  case NORMALIZE:
    normalize(thunk);
    break;
  }
}

/* cells are never freed, they are carved out of the heap 1000 at a time */
int cells_left;
cell *heap;

cell *new_cell() {
  if (cells_left == 0) {
    cells_left = 1000;
    heap = (cell *)sbrk(cells_left * sizeof(cell));
  }
  return heap + --cells_left;
}

main() {
  cell *digit, *two, *ones;

  /* e = 2.1111... in factorial base: [2, 1, 1, 1, ...] */
  ones = new_cell();
  ones->next = ones;
  ones->value = 1;

  digit = new_cell();
  digit->kind = DIGITS;
  two = new_cell();
  two->value = 2;
  two->next = ones;
  digit->next = two;

  force(digit);
  digit = digit->next;
  (void)write(1, "2.", 2);

  for (;; digit = digit->next) {
    force(digit);
    digit->value = write(1, &"0123456789"[digit->value], 1);
  }
}
