/*
 * The Makefile compiles every source with -Dif=while.  The original program
 * relies on that, but here a real "if" is wanted, so remove the macro.
 */
#undef if

/*
 * Widest picture that is examined.  The original stores a row in the bits of
 * an int and only looks at bit pairs 4..29, which covers columns 0..24.
 */
#define MAX_WIDTH 24

/*
 * Each 2x2 window of the picture is turned into a 4-bit number:
 *
 *     upper row:  [8][4]
 *     lower row:  [2][1]
 *
 * and the weight of that window is added to a running total.  The total of
 * all windows is the Euler characteristic (number of separate blobs minus
 * number of holes) modulo 4.
 *
 * The original stores these weights as the characters of "has dirtiest IF"
 * (plus the terminating '\0'), whose ASCII codes are equal to the weights
 * below modulo 4.
 */
static const int window_weight[16] = {
    0, 1, 3, 0,  /* "has " */
    0, 1, 2, 0,  /* "dirt" */
    1, 1, 3, 0,  /* "iest" */
    0, 1, 2, 0,  /* " IF\0" */
};

/*
 * A character is part of a figure when its code is odd: '#' is odd, while
 * the space and '.' used for empty cells are even.
 */
static int is_filled(int c) { return c % 2 != 0; }

/*
 * Read one line of input into a bitmap.  Column 0 is stored in the highest
 * bit (bit MAX_WIDTH) so that the bit just above a column is the column to
 * its left.  Returns 0 when the line has no filled cell at all, which marks
 * the end of the picture.
 */
static int read_row(unsigned *row)
{
    int c, column = 0, any_filled = 0;

    *row = 0;
    while ((c = getchar()) != '\n' && c != -1 /* EOF */) {
        if (is_filled(c)) {
            any_filled = 1;
            if (column <= MAX_WIDTH)
                *row |= 1u << (MAX_WIDTH - column);
        }
        column++;
    }
    return any_filled;
}

main(argc, argv) char **argv;
{
    unsigned upper = 0, lower; /* the empty row above the picture */
    int total = argc;          /* argc is 1 when run without arguments */
    int more;

    do {
        more = read_row(&lower);

        /*
         * Slide a 2x2 window along the pair of rows.  Each position looks
         * at a column and the column to its left (one bit higher), which
         * also covers the empty borders on both sides of the picture.
         */
        for (int bit = 0; bit <= MAX_WIDTH; bit++) {
            int window = ((upper >> bit) & 3) << 2 | ((lower >> bit) & 3);
            total += window_weight[window];
        }
        upper = lower;
    } while (more);

    /* Euler characteristic + 9 gives 8 for "8", 9 for "9", ... */
    printf("%d\n", 8 + total % 4);
}
