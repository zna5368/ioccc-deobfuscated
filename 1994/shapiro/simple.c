#include <stdio.h>

/*
 * The real program (the clock) is hidden inside the comments of shapiro.c.
 * Every byte of it is written as two letters from the range 'J'..'Y'
 * (16 letters, one per hex digit): first the low nibble, then the high one.
 * Every other character in the file is skipped, so the decoder below also
 * reads past the C code and the shape of the clock face.
 */
#define ENCODED_SOURCE "shapiro.c"
#define DIGIT_ZERO 'J'
#define DIGIT_COUNT 16

static int is_hex_letter(int c) {
  return c >= DIGIT_ZERO && c < DIGIT_ZERO + DIGIT_COUNT;
}

int main(void) {
  FILE *source;
  int c;
  int low_nibble = 0;
  int have_low_nibble = 0;

  source = fopen(ENCODED_SOURCE, "r");

  while ((c = getc(source)) != EOF) {
    if (!is_hex_letter(c))
      continue;

    if (!have_low_nibble) {
      low_nibble = c - DIGIT_ZERO;
      have_low_nibble = 1;
    } else {
      int high_nibble = c - DIGIT_ZERO;
      putchar(low_nibble | (high_nibble << 4));
      have_low_nibble = 0;
    }
  }

  return 0;
}
