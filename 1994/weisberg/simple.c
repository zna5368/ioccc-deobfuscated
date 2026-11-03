/*
 * Print the odd primes up to LIMIT by trial division with the primes already
 * found.  Every prime is remembered in a table (2 is placed there first but is
 * never printed).  A candidate only needs to be divided by the primes below
 * next_square_root, the smallest prime whose square has not been passed yet.
 */

#define LIMIT (2 << 6 * sizeof(int))     /* 33554432: stop after this number */
#define TABLE_SIZE (8 << 5 * sizeof(int)) /* 8388608: bytes allocated */

main() {
  int candidate = 3;
  int divisor;
  int next_square_root = 2;
  int *primes, *end, *next;

  primes = (int *)malloc(TABLE_SIZE);
  primes[0] = 2;
  end = primes + 1;
  next = primes; /* only the first candidate, 3, is tried against 2 */

  for (;;) {
    divisor = *next;

    if (divisor >= next_square_root) {
      if (divisor * divisor > candidate) {
        /* no divisor up to the square root: candidate is prime */
        if (end - primes < TABLE_SIZE)
          *end++ = candidate;
        printf("%d\n", candidate);
      } else {
        /* candidate is the square of next_square_root: not prime, and the
         * next prime must now also be tried as a divisor */
        next_square_root = next[1];
      }
    } else if (candidate % divisor != 0) {
      /* not divisible: try the next prime */
      next++;
      continue;
    }

    /* move on to the next odd candidate, starting again at divisor 3 */
    next = primes + 1;
    candidate += 2;
    if (candidate > LIMIT)
      exit(0);
  }
}
