#include <stdio.h>

/*
 * Count the size of a C source the way IOCCC rule 2 does: whitespace
 * (space, form feed, tab, newline) is not counted, and the characters
 * ';', '{' and '}' are not counted when followed by whitespace or end
 * of file.  Every other character counts as one.
 *
 * Input stops at end of file or at the first NUL byte.
 */

static int is_whitespace(int c)
{
    return c == ' ' || c == '\f' || c == '\t' || c == '\n';
}

static int is_semi_or_brace(int c)
{
    return c == ';' || c == '{' || c == '}';
}

int main(void)
{
    int count = 0;
    int pending_semi_brace = 0; /* previous char was ; { or } */
    int c;

    while ((c = getchar()) != EOF && c != '\0') {
        if (is_whitespace(c)) {
            /* a pending ; { } followed by whitespace is free */
            pending_semi_brace = 0;
        } else if (is_semi_or_brace(c)) {
            /* the previous ; { } is followed by this non-space char */
            count += pending_semi_brace;
            pending_semi_brace = 1;
        } else {
            count += 1 + pending_semi_brace;
            pending_semi_brace = 0;
        }
    }

    printf("%d\n", count);
    return 0;
}
