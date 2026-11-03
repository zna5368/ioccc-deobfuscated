#include <stdio.h>

#define WORD_BITS 16

int atoi(char *);

int first;  /* first addend */
int second; /* second addend */
int sum;    /* result, built one bit at a time */

/* Return bit number `bit` of `value` as 0 or 1. */
int get_bit(int value, int bit) { return (value & 1 << bit) != 0; }

/*
 * One stage of a ripple-carry adder.  Adds bit `bit` of both addends plus
 * the incoming carry, stores the sum bit in `sum`, and returns the carry
 * that feeds the next stage.
 */
int full_adder(int bit, int carry_in) {
  int a = get_bit(first, bit);
  int b = get_bit(second, bit);
  int half = a ^ b;
  int sum_bit = half ^ carry_in;
  int carry_out = (a && b) || (half && carry_in);

  sum |= sum_bit << bit;
  return carry_out;
}

int main(int argc, char **argv) {
  int bit;
  int carry = 0;

  if (argc != 3)
    return 0;

  first = atoi(argv[1]);
  second = atoi(argv[2]);

  /* cascade the carry through 16 adders; the final carry is dropped */
  for (bit = 0; bit < WORD_BITS; bit++)
    carry = full_adder(bit, carry);

  printf("%d\n", sum);
  return 0;
}
