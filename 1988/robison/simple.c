#include <stdio.h>

/*
 * Spigot algorithm for the digits of e = 2 + 1/2! + 1/3! + ...
 *
 * All arithmetic is done in unsigned char (mod 256), exactly like the
 * original, so that out-of-range arguments wrap the same way.
 */
unsigned char digits = 10;  /* first argument: digits to print (incl. the 2) */
unsigned char base = 10;    /* second argument: output radix */
unsigned char terms;        /* number of mixed-radix positions in use */
unsigned char fraction[256]; /* mixed-radix digits of e - 2 */

main(argc, argv) char *argv[];
{
  unsigned char pos, carry, value, quotient, count, digit;

  if (argc >= 2)
    digits = atoi(argv[1]);
  if (argc >= 3)
    base = atoi(argv[2]);

  printf("%s", "2.");
  fflush(stdout);

  terms = base + digits;

  /* e - 2 = 1/2! + 1/3! + ... : every mixed-radix digit starts at 1 */
  pos = 0;
  while (--pos)
    fraction[pos] = 1;

  while (--digits) {
    fraction[1] = 0;

    /*
     * Multiply the mixed-radix fraction by the base, from the last
     * position down to position 1, carrying the overflow leftwards.
     */
    quotient = 0;
    pos = terms;
    while (--pos) {
      carry = -quotient;   /* carry from the previous position */
      quotient = 0;
      value = carry;
      for (count = base; count; count--) {
        value += fraction[pos];
        while (value >= pos) {
          quotient--;      /* quotient is kept negated */
          value -= pos;
        }
      }
      fraction[pos] = value;
    }

    /* the carry out of position 1 is the next digit */
    digit = -quotient;
    printf("%x", digit);
    fflush(stdout);
    terms--;
  }

  printf("%s", "\n");
  fflush(stdout);
}
