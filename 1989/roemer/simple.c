/*
 * Print e = 2.71828... to 3139 decimal places.
 *
 * e = 1/0! + 1/1! + 1/2! + 1/3! + ...
 *
 * Both e and the current term 1/n! are kept as arrays of decimal digits
 * (element 0 is the integer part).  Each pass divides the term by the next
 * n with long division and adds the quotient into the sum.
 */

#define DIGITS 3141     /* digits kept for the sum and the term   */
#define TABLE 31415     /* room for every long division step r*10+d */

char sum[DIGITS], term[DIGITS];
int quotients[TABLE], remainders[TABLE]; /* cache of r/n and r%n */

main() {
  register char *sum_digit, *term_digit, *carry_digit;
  register int rest, i, n, *cached, nonzero, start;
  int count;

  /* sum = 2.5 (1/0! + 1/1! + 1/2!),  term = 0.5 (1/2!) */
  sum[0] = n = 2;
  sum[1] = term[1] = 5;
  start = 1; /* position of the first non zero digit of the term */

  do {
    nonzero = rest = 0;
    n++;

    /* forget the cached divisions by the previous n */
    for (i = 0; i < 10 * n; i++)
      remainders[i] = quotients[i] = -1;
    quotients[0] = 0;
    remainders[0] = 0;

    /* term = term / n  and  sum = sum + term, one digit at a time */
    sum_digit = sum + start;
    term_digit = term + start;
    for (count = DIGITS - start; count; count--, sum_digit++, term_digit++) {
      rest = rest * 10 + *term_digit;
      cached = quotients + rest;
      if (*cached == -1) {
        *cached = rest / n;
        remainders[rest] = rest % n;
      }
      *term_digit = *cached;
      *sum_digit += *term_digit;

      /* carry into the digits on the left */
      carry_digit = sum_digit;
      while (*carry_digit >= 10) {
        *carry_digit -= 10;
        (*--carry_digit)++;
      }

      rest = remainders[rest];
      if (!nonzero && *term_digit) {
        nonzero = 1;
        start = DIGITS - count;
      }
    }

    /*
     * Round the last digit up.  sum_digit is now one past the end of sum,
     * so this rounds a byte outside the array, just as the original does.
     */
    if (rest + (n >> 1) >= n)
      while (++*sum_digit == 10)
        *sum_digit-- = 0;
  } while (nonzero);

  {
    /* turns the 3 into a 2 by writing into a string literal */
    char *point = "3.1415";
    --*point;
    write(1, point, 2);
  }
  for (i = 1; i < DIGITS - 1; i++)
    write(1, "0123456789" + sum[i], 1);
  sum[0] = 0;
  puts(sum);
}
