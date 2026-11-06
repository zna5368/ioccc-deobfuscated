#include <stdio.h>

/*
 * POSIX whitespace in the C locale: space, '\t', '\n', '\v', '\f', '\r'.
 * Note that '\a' and '\b' are NOT whitespace.
 */
static int is_space(int c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

int main(void)
{
    int lines = 0;
    int words = 0;
    int chars = 0;
    int in_word = 0;
    int c;

    while ((c = getchar()) != EOF) {
        chars++;
        if (c == '\n')
            lines++;

        if (!is_space(c)) {
            in_word = 1;
        } else if (in_word) {
            /* whitespace right after a word ends that word */
            words++;
            in_word = 0;
        }
    }

    /* a word still open at end of input counts too */
    if (in_word)
        words++;

    printf("%8d%8d%8d\n", lines, words, chars);
    return 0;
}
