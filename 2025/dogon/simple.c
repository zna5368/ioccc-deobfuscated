#include <stdio.h>
#include <stdlib.h>

/*
 * Print the digits of e = 1/0! + 1/1! + 1/2! + 1/3! + ...
 *
 * Both numbers are kept as big fixed point values in base 100000 (five
 * decimal digits per limb).  Limb 0 of "term" is the integer part; in "sum"
 * everything is shifted by one so limb 1 holds the integer part and limb 0
 * only collects overflow.
 *
 * The original packs every string into the bytes of an int, so it only
 * works properly on little endian machines (on big endian it prints
 * "CCOI" instead of "IOCC").  This version prints the little endian output.
 */

#define BASE      100000        /* 5 decimal digits per limb */
#define LIMBS     8192          /* number of limbs in each big number */
#define PER_LINE  16            /* limbs between newlines */

int main(void)
{
    int *term = calloc(LIMBS + 1, sizeof(int));     /* 1/divisor! */
    int *sum = calloc(LIMBS + 1, sizeof(int));      /* running total */
    int limb = 0;           /* current limb in the division pass */
    int divisor = 1;        /* term is divided by this during the pass */
    int done = 0;           /* leading limbs of term that are zero */

    printf("   ");
    printf("2.");

    term[0] = 1;            /* 1/0! */
    sum[1] = 1;             /* sum starts at 1/0! */

    while (done < LIMBS) {
        /* the remainder of this limb moves down to the next limb */
        term[limb + 1] += term[limb] % divisor * BASE;

        /*
         * Once the leading limb of the term becomes zero, no more will be
         * added to that limb of the sum, so it can be printed.
         */
        if (!term[done]) {
            if (done >= 2) {
                printf("%05d", sum[done]);
                if (done % PER_LINE == 0 || done >= LIMBS - 1)
                    printf("\n");
            }
            done++;
        }

        /* term /= divisor, then add it into the sum with carry */
        term[limb] /= divisor;
        sum[limb + 1] += term[limb];
        sum[limb] += sum[limb + 1] / BASE;
        limb++;
        sum[limb] %= BASE;

        /* after a full pass, the next term is 1/(divisor+1)! */
        if (limb == LIMBS) {
            limb = 0;
            divisor++;
        }
    }

    printf("IOCC");
    printf("C%5d", 2026);
    return 0;
}
