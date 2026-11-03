/*
 * The number 33303285 written in base 25 has the digits 3 10 6 10 6 10
 * (most significant first).  Each digit is the radix used for one place of
 * the clock: HH MM SS  ->  hour tens (3), hour units (10), minute tens (6),
 * minute units (10), second tens (6), second units (10).
 */
#define SECONDS_PER_DAY 86400 /* 0250600 in octal */
#define PACKED_RADICES 33303285
#define PACKED_BASE 25
#define CLOCK_DIGITS 6
#define OUTPUT_LENGTH 9

main(argc, argv) char **argv;
{
  long now = time(0);
  long seconds = now % SECONDS_PER_DAY; /* seconds since midnight GMT */
  long radices = PACKED_RADICES;
  /*
   * The digits are stored over the bytes of argv[0], one digit per
   * pointer-sized slot (the original treats argv[0] as an array of char *).
   */
  char **slot = (char **)argv[0];
  int place;

  (void)argc;

  /* Fill the clock from the last digit (seconds units) back to the first. */
  for (place = CLOCK_DIGITS - 1; place >= 0; place--) {
    int radix = radices % PACKED_BASE;
    slot[place] = (char *)(seconds % radix + '0');
    seconds /= radix;
    radices /= PACKED_BASE;
  }

  write(1, argv[0], OUTPUT_LENGTH);
}
