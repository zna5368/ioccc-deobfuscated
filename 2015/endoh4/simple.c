/*
 * Draws a factorization diagram: one '@' for every command line argument after
 * the first, arranged so that each factor of the count becomes a level of
 * grouping.  Groups alternate between sitting side by side and being stacked.
 */

/* Passed as row_width when the shape of the diagram is not yet decided */
#define SHAPE_UNDECIDED -1

/*
 * number * number, keeping only the low 32 bits.  The result turns negative
 * once number reaches 46341, which draw_block depends on.  See below.
 */
int wrapped_square(int number)
{
    return (int)((unsigned)number * (unsigned)number);
}

/*
 * Tries each candidate whose square fits in number, and remembers the last one
 * that divides it.  Starting from 1, this leaves *divisor as the largest
 * divisor of number which is no bigger than its square root.
 */
void search_divisors(int number, int *candidate, int *divisor)
{
    while (wrapped_square(*candidate) <= number) {
        if (number % *candidate == 0) {
            *divisor = *candidate;
        }
        ++*candidate;
    }
}

/*
 * Draws a block of `count` parts.
 *
 * When row_width is 0, the parts go side by side, and each is a single dot.
 * Otherwise the parts are stacked, and each is a row of row_width dots.
 * A count of 0 with a row_width of 0 is the smallest part: one dot.
 *
 * A composite count is split into groups of `divisor` parts, which is what
 * makes the factors visible.  A prime count has a divisor of 1 and cannot be
 * split, so its parts are drawn directly.
 */
void draw_block(int count, int row_width)
{
    int remaining = count;
    int divisor = 1;
    int candidate = 1;

    search_divisors(remaining, &candidate, &divisor);

    if (row_width == SHAPE_UNDECIDED) {
        /* The closest pair of factors gives the squarest diagram */
        draw_block(divisor, remaining / divisor);
        return;
    }

    if (count == 0) {
        putchar('@');
        return;
    }

    while (remaining > 0) {
        if (divisor == 1) {
            draw_block(row_width, 0);
        } else {
            draw_block(divisor, row_width);
        }
        putchar(row_width != 0 ? '\n' : ' ');
        remaining -= divisor;

        /*
         * candidate keeps counting up and is never reset.  Normally its
         * square is already too large and this finds nothing.  After about
         * 46000 groups the square wraps around to a negative number, the
         * search runs again on what is left, and may pick a new divisor.
         * This only changes the grouping of counts with a huge prime factor.
         */
        candidate++;
        search_divisors(remaining, &candidate, &divisor);
    }
}

int main(int argc, char *argv[])
{
    (void)argv;

    draw_block(argc - 1, SHAPE_UNDECIDED);
    return 0;
}
