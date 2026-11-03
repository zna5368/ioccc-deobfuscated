#include <stdio.h>

/*
 * Prime number printer.
 *
 * The original program does all of its work in the C preprocessor.  The
 * source file includes itself again and again, and every time it is read it
 * adds one to a binary counter whose bits are the macros a..m being defined
 * or not.  The low four bits (a..d) choose a trial divisor and the high nine
 * bits (e..m) choose the candidate number.  When the preprocessor is finished
 * the only C code left is a main() made of printf() calls, one per prime.
 *
 * Here the self inclusion is a recursive function and the macros are
 * ordinary variables.
 */

#define DIVISOR_BITS 4      /* a, b, c, d */
#define DIVISORS_PER_CANDIDATE (1 << DIVISOR_BITS)
#define LAST_DIVISOR (2 * (DIVISORS_PER_CANDIDATE - 1) + 1) /* 31 */
#define INCLUDES_PER_FILE 15 /* the fifteen M lines */
#define MAX_DEPTH 4         /* nesting tracked by w, x, y, z */

static int counter;       /* the 13 bit counter in the macros a..m */
static int has_divisor;   /* the macro dd */

/* one pass over the file: test one (candidate, divisor) pair */
void test_next_pair(void)
{
    int divisor, candidate;

    counter++;
    divisor = 2 * (counter % DIVISORS_PER_CANDIDATE) + 1;   /* ff */
    candidate = 2 * (counter / DIVISORS_PER_CANDIDATE) + 1; /* vv */

    /* only odd numbers are generated, so 1 stands in for 2 */
    if (candidate == 1)
        candidate = 2;

    if (divisor < candidate && divisor != 1 && candidate % divisor == 0)
        has_divisor = 1;

    /* after the last divisor the candidate is either prime or not */
    if (divisor == LAST_DIVISOR) {
        if (has_divisor)
            has_divisor = 0;
        else
            printf("%d\n", candidate);
    }
}

/* the file includes itself fifteen times until it is four levels deep */
void include_file(int depth)
{
    int i;

    test_next_pair();
    if (depth < MAX_DEPTH)
        for (i = 0; i < INCLUDES_PER_FILE; i++)
            include_file(depth + 1);
}

main()
{
    include_file(1);
}
