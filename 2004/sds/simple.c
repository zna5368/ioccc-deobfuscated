#include <stdio.h>

/*
 * Each hidden byte is stored as five base-3 digits, least significant first.
 * Every digit is written as one whitespace character.
 */
#define DIGITS_PER_BYTE 5

/*
 * Turn a whitespace character into its base-3 digit plus one, so that
 * zero can mean "this character does not carry any hidden data":
 *
 *   space   -> 1  (digit 0)
 *   tab     -> 2  (digit 1)
 *   newline -> 3  (digit 2)
 *   other   -> 0
 */
int whitespace_digit(int ch)
{
    switch (ch) {
    case ' ':
        return 1;
    case '\t':
        return 2;
    case '\n':
        return 3;
    default:
        return 0;
    }
}

int main()
{
    int ch;

    while ((ch = getchar()) != EOF) {
        int digit = whitespace_digit(ch);
        int byte = 0;
        int place = 1;
        int count = 0;

        /*
         * Collect a run of five whitespace characters.  If a non-whitespace
         * character interrupts the run, the partial value is thrown away
         * (together with that character) and scanning starts over.
         */
        while (digit != 0) {
            byte += (digit - 1) * place;
            place *= 3;

            if (++count == DIGITS_PER_BYTE) {
                putchar(byte);
                break;
            }
            digit = whitespace_digit(getchar());
        }
    }

    return 0;
}
