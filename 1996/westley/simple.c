/*
 * Usage: westley minute_map hour_map hand_symbols clock_image
 *
 * argv[1] minute_map   - offset into clock_image for each minute (0-59)
 * argv[2] hour_map     - offset into clock_image for each hour (0-23)
 * argv[3] hand_symbols - [1] is the minute hand, [2] is the hour hand
 * argv[4] clock_image  - the picture of the clock that is printed
 *
 * The original entry (westley.orig.c / westley.alt.c) writes each hand into
 * the clock image with
 *
 *     clock_image[map[field][now[field]]] = hand_symbols[field];
 *
 * inside the loop below.  This version of westley.c has no loop body, so
 * the hands are never drawn and the clock image is printed unchanged.
 */

main(argc, argv) char **argv;
{
  int field;
  char *clock_image;

  if (argc > 4) {
    clock_image = argv[4];

    /*
     * time() stores the current time (a long) straight into the bytes of
     * the program name, argv[0], so no separate variable is needed.
     */
    time(argv[0]);

    /*
     * argc is 5, so field takes the values 5/2 = 2 and then 2/2 = 1,
     * the indexes of tm_hour and tm_min when the struct tm from
     * localtime() is viewed as an int array.
     */
    for (field = argc / 2; field != 0; field /= 2) {
      /* nothing: the hand-drawing statement is missing in this version */
    }

    puts(clock_image);
  }
}
