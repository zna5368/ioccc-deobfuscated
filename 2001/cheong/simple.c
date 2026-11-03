#include <stdio.h>

/*
 * Integer square root of a 2n digit number using the longhand method,
 * where each new root digit is found only by repeated subtraction.
 *
 * For each pair of input digits the remainder gains two digits, and the
 * odd numbers 20*root + 1, 20*root + 3, 20*root + 5, ... are subtracted
 * from it for as long as the remainder stays non-negative.  The number
 * of successful subtractions is the next digit of the root.
 */

/*
 * Try to subtract (20 * root + 2 * digit + 1) from the remainder, where
 * root is made of the first root_digits digits already found.  The
 * least significant digit of the remainder is remainder[last].
 *
 * The subtraction is done digit by digit with a borrow.  The digits of
 * the subtrahend are the doubled root digits, with 2 * digit + 1 (which
 * may be larger than 9) as its lowest digit.  The remainder is only
 * updated if the result is not negative.  Returns 1 on success.
 */
static int subtract_odd(int *remainder, const int *root, int root_digits,
                        int digit, int last) {
  int result[last + 1];
  int borrow = 0;

  for (int place = 0; place <= last; place++) {
    int position = last - place;
    int subtrahend;

    if (place == 0)
      subtrahend = 2 * digit + 1;
    else if (place <= root_digits)
      subtrahend = 2 * root[root_digits - place];
    else
      subtrahend = 0;

    int value = remainder[position] - subtrahend - borrow;
    borrow = 0;
    while (value < 0) {
      value += 10;
      borrow++;
    }
    result[position] = value;
  }

  if (borrow)
    return 0; /* would go negative: leave the remainder alone */

  for (int position = 0; position <= last; position++)
    remainder[position] = result[position];
  return 1;
}

int main(int argc, char **argv) {
  char *number = argv[1];
  int length = 0;

  while (number[length])
    length++;

  int remainder[length + 1];
  int root[length / 2 + 1];

  for (int i = 0; i < length; i++)
    remainder[i] = number[i] - '0';

  for (int found = 0; 2 * found < length; found++) {
    int last = 2 * found + 1; /* bring down the next pair of digits */
    int digit = 0;

    while (subtract_odd(remainder, root, found, digit, last))
      digit++;

    root[found] = digit;
    putchar('0' + digit);
  }
  putchar('\n');

  return argc;
}
