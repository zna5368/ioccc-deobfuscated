/*
 * Decodes a message hidden in the compiler warnings produced while compiling
 * lush.c. Run it as:
 *
 *     cc -include stdio.h -o lush lush.c |& ./simple
 *
 * Every line of lush.c that redefines the macro "f" makes the compiler print
 *
 *     lush.c:LINE: warning: "f" redefined
 *
 * The last digit of LINE is one bit of the message: odd is 1, even is 0.
 * A warning is only counted when its last digit differs from the previously
 * counted one, so repeated digits are skipped. Every four bits form an index
 * into the letters table.
 */

#define LINE_SIZE 999
#define BITS_PER_LETTER 4
#define UNUSED_LETTER 15

char line[LINE_SIZE];
char message[LINE_SIZE];
char *letters = "WHolNe rd.msg!*\n";

/* Is c the last digit of a number that is followed by ':' or '.'? */
int ends_number(char *c)
{
    return c[0] >= '0' && c[0] <= '9' && (c[1] == ':' || c[1] == '.');
}

/* Does text start with word? */
int starts_with(char *text, char *word)
{
    while (*word)
        if (*text++ != *word++)
            return 0;
    return 1;
}

int main(void)
{
    char *pos;
    char *out = message;
    char last_digit = 0;
    int bits = 0;
    int bit_count = 0;

    while (fgets(line, LINE_SIZE, stdin)) {
        /* find the line number, e.g. the "0" of "lush.c:40: warning" */
        for (pos = line; *pos && !ends_number(pos); pos++)
            ;

        if (*pos == '\0' || pos[2] == '\0' ||
            !starts_with(pos + 3, "warning") || *pos == last_digit)
            continue;

        last_digit = *pos;
        bits = bits * 2 + (*pos & 1);
        bit_count++;

        if (bit_count == BITS_PER_LETTER) {
            if (bits < UNUSED_LETTER)
                *out = letters[bits];
            out++;
            bits = 0;
            bit_count = 0;
        }
    }

    *out = '\0';
    puts(message);
}
