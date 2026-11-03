#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * A code is four colors packed into the four hex digits (nibbles) of an
 * unsigned short. Internally colors are 0 .. colors-1; they are shown to the
 * user as 1 .. colors by adding 1 to every digit (0x1111).
 */
#define DIGIT_OFFSET 0x1111
#define ALL_BULLS 0x40 /* score for 4 bulls and 0 cows */

/* Add one to a code, counting in base `colors` with one digit per nibble. */
static unsigned short next_code(unsigned short code, unsigned short colors)
{
    unsigned short mask = 0xF;     /* selects the current digit */
    unsigned short limit = colors; /* `colors` placed in the current digit */
    int position;

    code++;
    for (position = 0; position < 4; position++) {
        if ((code ^ limit) & mask)
            return code; /* digit is still a valid color: no carry */
        /* digit overflowed: set it to 0 and carry 1 into the next digit
           (mask + mask / 15 is a 1 in the next digit, e.g. 0xF + 1 = 0x10) */
        code += mask + mask / 15 - limit;
        limit <<= 4;
        mask <<= 4;
    }
    return 0;
}

/*
 * Compare a secret code with a guess and return bulls * 16 + cows.
 * A bull is a guess digit in the same position as in the secret, a cow a
 * guess digit found in a different position. Each guess digit counts at most
 * once: when it matches it is replaced by 0xF, which is never a color.
 */
static unsigned short score(unsigned short secret, unsigned short guess)
{
    unsigned short result = 0, mask;
    int rotation, position;

    for (rotation = 0; rotation < 4; rotation++) {
        for (position = 0, mask = 0xF; position < 4; position++, mask <<= 4) {
            if (!((secret ^ guess) & mask)) {
                result++;
                guess |= mask;
            }
        }
        /* rotate the guess one digit to compare it in other positions */
        guess = guess << 4 | guess >> 12;
        if (rotation == 0)
            result <<= 4; /* the bulls go into the high digit */
    }
    return result;
}

int main(int argc, char **argv)
{
    unsigned short *candidates, count, kept, secret = 0, guess = 0, answer;
    int colors, i;
    char *name = argv[0];
    /* called by a name ending in 'd' (mind): the user guesses, else we do */
    int computer_guesses = name[strlen(name) - 1] != 'd';

    srand(time(0));

    if (!(argc == 2 && sscanf(argv[1], "%d", &colors) && colors))
        colors = 6;

    candidates = (unsigned short *)calloc(colors * colors * colors * colors,
                                          sizeof(unsigned short));
    if (!candidates) {
        printf("No    memory\t\n");
        exit(1);
    }

    /* list every possible code, starting from 0000 */
    for (count = 1; count < colors * colors * colors * colors; count++)
        candidates[count] = next_code(candidates[count - 1], colors);

    if (computer_guesses)
        guess = candidates[rand() % count];
    else
        secret = candidates[rand() % count];

    do {
        printf("(%d): ????\b\b\b\b", count);
        if (computer_guesses) {
            /* show our guess and read the user's bulls and cows */
            printf("%04X\n", guess + DIGIT_OFFSET);
            printf("??\b\b");
            scanf("%hx", &answer);
        } else {
            /* read the user's guess and show its bulls and cows */
            scanf("%hx", &guess);
            guess -= DIGIT_OFFSET;
            printf("??\b\b");
            printf("%02x\n", answer = score(secret, guess));
        }

        /* keep only the codes that would have given the same answer */
        kept = 0;
        for (i = 0; i < count; i++)
            if (score(candidates[i], guess) == answer)
                candidates[kept++] = candidates[i];
        count = kept;

        if (answer == ALL_BULLS || count == 0)
            break;
        guess = candidates[rand() % count];
    } while (1);

    printf("\n");
}
