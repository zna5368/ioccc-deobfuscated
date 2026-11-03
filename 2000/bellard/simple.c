/*
 * Print the Mersenne prime 2^6972593 - 1 in base 10.
 *
 * The number is held as an array of decimal digits (least significant first).
 * It is computed by binary exponentiation: each step squares the number using
 * a Number Theoretic Transform (a Fast Fourier Transform done modulo a prime)
 * to multiply the digit polynomial by itself, and doubles it when the current
 * exponent bit is set.
 */

#define MODULUS 754974721 /* prime 45 * 2^24 + 1, so 2^24-th roots exist */
#define MAX_LOG2 22       /* largest transform holds 2^22 digits */
#define MAX_SIZE (1 << MAX_LOG2)
#define ROOT 362              /* primitive 2^24-th root of unity mod MODULUS */
#define INVERSE_ROOT 415027540 /* 362^-1 mod MODULUS */
#define EXPONENT 6972593

int digits[MAX_SIZE]; /* number in base 10, or its transform */
int size;             /* current transform length (power of two) */

static int mulmod(int x, int y) { return (long long)x * y % MODULUS; }

/*
 * In-place NTT of digits[0..size-1]. 'root' must be a primitive 2^24-th root
 * of unity; it is squared at each level so that for a butterfly of half-width
 * 'half' the twiddle base is a primitive (2*half)-th root.
 * Decimation in frequency followed by a bit-reversal permutation.
 */
void ntt(int root) {
  int half, i, j, w;

  for (half = 1 << 23; half; half /= 2, root = mulmod(root, root)) {
    if (half >= size)
      continue;
    for (int block = 0; block < size; block += 2 * half) {
      w = 1;
      for (i = block; i < block + half; i++) {
        int x = digits[i];
        int y = digits[i + half];
        digits[i] = (x + y) % MODULUS;
        digits[i + half] = mulmod(MODULUS + x - y, w);
        w = mulmod(w, root);
      }
    }
  }

  /* bit-reversal permutation: j is i with its bits reversed */
  j = 0;
  for (i = 1; i < size; i++) {
    int bit = size / 2;
    /* increment j in bit-reversed order */
    while (!((j ^= bit) & bit))
      bit /= 2;
    if (i < j) {
      int tmp = digits[i];
      digits[i] = digits[j];
      digits[j] = tmp;
    }
  }
}

int main(void) {
  int inverse_size = 1; /* size^-1 mod MODULUS */
  int bit, i, top;

  digits[0] = 2; /* start with 2^1: the leading exponent bit, 2^22 */
  size = 1;

  for (bit = MAX_LOG2 - 1; bit >= 0; bit--) {
    size *= 2;
    inverse_size = mulmod(inverse_size, (MODULUS + 1) / 2);

    /* square the number: forward transform, square pointwise, inverse */
    ntt(ROOT);
    for (i = 0; i < size; i++)
      digits[i] = mulmod(mulmod(digits[i], digits[i]), inverse_size);
    ntt(INVERSE_ROOT);

    /* propagate carries, doubling the number if this exponent bit is 1 */
    int factor = (EXPONENT >> bit) & 1 ? 2 : 1;
    int carry = 0;
    for (i = 0; i < size; i++) {
      carry += factor * digits[i];
      digits[i] = carry % 10;
      carry /= 10;
    }
  }

  /* find the most significant nonzero digit */
  for (top = size - 1; digits[top] == 0; top--)
    ;

  digits[0]--; /* 2^n - 1 (2^n never ends in 0, so no borrow) */

  for (i = top; i >= 0; i--)
    printf("%d", digits[i]);
}
