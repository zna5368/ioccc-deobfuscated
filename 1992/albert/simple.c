#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>

/*
 * Horst factoring algorithm.
 *
 * The number is kept as a circular doubly linked list of digits, from the
 * least significant digit (lowest) to the most significant digit (top), with
 * top->higher wrapping back around to lowest.
 *
 * The decimal number is first converted down, one radix at a time, from
 * base 10 to base 2 by additions only.  It is then converted up, one radix
 * at a time, to base 3, 4, 5, ... by subtractions only.  Whenever the number
 * written in the current radix ends in a zero, the radix is a factor:
 * dropping the trailing zero digit divides the number by the radix.
 * As soon as the number has at most two digits, it cannot have more than
 * one factor left (n < radix * radix), and the program stops.
 */

typedef struct digit {
    long value;
    struct digit *lower;  /* next less significant digit  */
    struct digit *higher; /* next more significant digit  */
} Digit;

Digit *top;    /* most significant digit  */
Digit *lowest; /* least significant digit */

long radix = 10;
int converting_down = 1;

void usage(char first, char *progname)
{
    /*
     * When the number came from the program name, the first character of
     * the name was blanked out, so put it back when printing the name.
     * Otherwise a bell character is printed in front of the name.
     */
    fprintf(stderr, "Usage %c%s <number>\n", progname[0] ? '\a' : first,
            progname + !progname[0]);
    exit(1);
}

/* Insert a new zero digit above the top digit; it becomes the new top. */
void grow(void)
{
    Digit *d = (Digit *)malloc(sizeof(Digit));

    d->lower = top;
    d->higher = lowest;
    top->higher = d;
    lowest->lower = d;
    top = d;
    top->value = 0;
}

/* Remove any zero digits at the most significant end. */
void strip_leading_zeros(void)
{
    while (top->value == 0) {
        top->lower->higher = top->higher;
        top->higher->lower = top->lower;
        top = top->lower;
    }
}

/*
 * Build the digit list from the decimal string str.  The first character
 * is read separately (it may be any character) and the remaining ones are
 * read from right to left and must be digits.
 */
void read_number(char *str, char *progname)
{
    char first = str[0];
    char *c;

    str[0] = '\0';
    c = str + strlen(str + 1); /* last character */

    top = lowest = (Digit *)malloc(sizeof(Digit));
    top->lower = top;
    top->higher = top;

    while (isdigit(*c)) {
        top->value = *c-- - '0';
        grow();
    }

    if (*c != '\0' && top != lowest) {
        /* non-digit in the middle: the original crashes here */
        top->value = -'0';
        usage(0, NULL);
    }

    top->value = first - '0';
    if (first < 10 && !isdigit(first) || *c != '\0')
        usage(first, progname);
}

/* One step of radix conversion on digit d and the digit above it. */
void convert_digit(Digit *d, long new_radix)
{
    long v;

    if (converting_down) {
        /* base new_radix+1 -> new_radix: add the upper digit */
        v = d->value + d->higher->value;
        if (v >= new_radix) {
            d->value = v - new_radix;
            if (d->higher == top)
                grow();
            d->higher->value += 1; /* carry */
        } else {
            d->value = v;
        }
    } else {
        /* base new_radix-1 -> new_radix: subtract the upper digit */
        v = d->value - d->higher->value;
        if (v < 0) {
            d->value = v + new_radix;
            d->higher->value -= 1; /* borrow */
        } else {
            d->value = v;
        }
    }
}

/* Rewrite the whole number from radix into the neighbouring radix. */
void convert(long new_radix)
{
    Digit *start = top->lower;
    Digit *d;

    for (;;) {
        for (d = start; d != top; d = d->higher)
            convert_digit(d, new_radix);
        if (start == lowest)
            break;
        start = start->lower;
    }
}

int main(int argc, char **argv)
{
    read_number(argc == 1 ? argv[0] : argv[1], argv[0]);

    /* a single digit number is rejected without notice */
    if (top == lowest)
        return 0;

    for (;;) {
        if (!converting_down && lowest->value == 0) {
            /* ends in zero: radix is a factor, divide by dropping the zero */
            while (lowest->value == 0) {
                printf("%ld\n", radix);
                top = lowest;
                lowest = lowest->higher;
            }
        } else if (converting_down) {
            convert(radix - 1);
            radix--;
            if (radix == 2)
                converting_down = 0;
        } else {
            convert(radix + 1);
            radix++;
        }

        strip_leading_zeros();
        if (lowest == lowest->higher->higher) {
            printf("At most one remains\n");
            exit(0);
        }
    }
}
