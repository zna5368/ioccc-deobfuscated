#include "stdio.h"

/*
 * Every character of the message is built from three octal digits
 * (for example 'h' = 0150).  Character number i of the message gets the
 * "key" n = MESSAGE_LENGTH - 1 - i, and each octal digit is computed from n
 * by a different function.
 */
#define MESSAGE_LENGTH 14 /* strlen("hello world!\r\n") */

/* the first 14 prime numbers, one per key */
int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43};

/*
 * Products of primes.  The low octal digit of a character is the index of the
 * first product that is divisible by the character's prime.
 */
long prime_products[] = {
    989L,   /* 0: 23 * 43           -> ' ' 'h'         */
    5L,     /* 1: 5                 -> '!'             */
    26L,    /* 2: 2 * 13            -> '\n' 'r'        */
    0L,     /* 3: unused                               */
    88319L, /* 4: 7 * 11 * 31 * 37  -> 'd' 'l'         */
    123L,   /* 5: 3 * 41            -> '\r' 'e'        */
    0L,     /* 6: unused                               */
    9367L,  /* 7: 17 * 19 * 29      -> 'o' 'w'         */
};

/* true if every bit of mask is set in n */
#define HAS_BITS(n, mask) (((n) & (mask)) == (mask))

/* high octal digit: 1 for letters (0100-0177), 0 for ' ', '!', '\r', '\n' */
int high_digit(int n) {
  return HAS_BITS(n, 8 | 2) | HAS_BITS(n, 4) | HAS_BITS(n, 8 | 1) |
         HAS_BITS(n, 2 | 1);
}

/* middle octal digit, taken from a Fibonacci-like sequence */
int middle_digit(int n) {
  int sequence[6];
  int step, i;

  if (n < 2)
    return 1;
  if (n < 8) {
    n -= 1;
    step = 'D';
    sequence[0] = 2;
    sequence[1] = 0;
  } else {
    n -= 7;
    step = 5;
    sequence[0] = sequence[1] = 1;
  }
  for (i = 2; i < n; ++i)
    sequence[i] = sequence[i - 2] + sequence[i - 1] + step;
  return sequence[n - 1] % n + 4;
}

/* low octal digit: index of the first product divisible by the n-th prime */
int low_digit(int n) {
  int index;

  for (index = 0; index < 8; ++index)
    if (prime_products[index] && !(prime_products[index] % (long)primes[n]))
      return index;
  /* never reached for this message, the original falls through to here */
  return middle_digit(n);
}

int main(void) {
  char message[MESSAGE_LENGTH + 1];
  int i, n;

  for (i = 0; i < MESSAGE_LENGTH; ++i) {
    n = MESSAGE_LENGTH - 1 - i;
    message[i] = (high_digit(n) << 6) | (middle_digit(n) << 3) | low_digit(n);
  }
  message[MESSAGE_LENGTH] = '\0';

  printf(message); /* "hello world!\r\n" */
  return 0;
}
