#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Words are stored in word[1..len]; index 0 is unused so the
 * positions match the original program. At most 63 letters fit.
 */
#define WORD_SIZE 64

/*
 * A letter is an ASCII letter (either case) or any byte in the upper
 * half of a single-byte character set (128..255).
 */
static int is_letter(unsigned c)
{
    return (c | 32) - 'a' < 26 || c >> 7 == 1;
}

int main()
{
    unsigned c, len, pick, word[WORD_SIZE];

    srand(time(0));

    for (;;) {
        /* collect a run of letters; the first non-letter ends the word */
        len = 0;
        while ((c = getchar(), is_letter(c)) && len + 1 < WORD_SIZE)
            word[++len] = c;

        /*
         * Print the word with its first and last letters in place and
         * the middle letters shuffled.
         *
         * The first letter printed is word[1]. Its slot is then filled
         * with the last letter, which is only picked once all middle
         * letters (slots 2..len) are gone. Each printed slot is refilled
         * with the letter at the end of the array.
         */
        pick = 1;
        while (len > 0) {
            putchar(word[pick]);
            word[pick] = word[len];
            pick = len > 2 ? 2 + rand() % (len - 2) : 1;
            len--;
        }

        if (c == (unsigned)EOF)
            break;

        /* copy the separator (or overflow letter) unchanged */
        putchar(c);
    }
    return 0;
}
