#include <stdio.h>
#include <stdlib.h>

/*
 * Reverse the order of the lowest nbits bits of value by swapping
 * bit i with bit (nbits - 1 - i) whenever the two bits differ.
 */
static unsigned long reflect(unsigned long value, unsigned long nbits) {
  unsigned long i;

  for (i = 0; i < nbits / 2; i++) {
    unsigned long low = nbits - 1 - i;
    if (((value >> i) ^ (value >> low)) & 1)
      value ^= (1UL << i) ^ (1UL << low);
  }
  return value;
}

int main(int argc, char **argv) {
  FILE *file;
  unsigned long width;     /* argv[2]: CRC width in bits (decimal) */
  unsigned long poly;      /* argv[3]: CRC polynomial (hex) */
  unsigned long reflected; /* argv[4]: nonzero if CRC is reflected (hex) */
  unsigned long init;      /* argv[5]: initial CRC value (hex) */
  unsigned long xorout;    /* argv[6]: final XOR value (hex) */
  unsigned long crc;
  unsigned long mask;
  int ch;
  int bit;

  if (argc != 7 || (file = fopen(argv[1], "rb")) == NULL)
    return EXIT_FAILURE;

  width = strtoul(argv[2], NULL, 10);
  poly = strtoul(argv[3], NULL, 16);
  reflected = strtoul(argv[4], NULL, 16);
  init = strtoul(argv[5], NULL, 16);
  xorout = strtoul(argv[6], NULL, 16);

  crc = init;
  while ((ch = getc(file)) != EOF) {
    unsigned long byte = (unsigned long)ch;

    if (reflected)
      byte = reflect(byte, 8);

    /* feed the byte into the top of the CRC register */
    crc ^= byte << (width - 8);

    /* shift out 8 bits, applying the polynomial when the top bit is set */
    for (bit = 0; bit < 8; bit++) {
      if ((crc >> (width - 1)) & 1)
        crc = (crc << 1) ^ poly;
      else
        crc <<= 1;
    }
  }

  /* keep only the low width bits (written to also work when width == 64) */
  mask = ((((unsigned long)1 << (width - 1)) - 1) << 1) | 1;
  crc &= mask;

  if (reflected)
    crc = reflect(crc, width);

  crc ^= xorout;
  fclose(file);

  /* print as hex, zero padded to ceil(width / 4) digits */
  printf("%0*lX\n", (int)((width + 3) / 4), crc);
  return 0;
}
