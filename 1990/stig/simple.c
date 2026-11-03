#include <stdio.h>

/*
 * Check whether the compiler supports nested comments.
 *
 * The expression "slash-star slash-star slash 0 star slash star star slash 1"
 * is read differently depending on how comments work:
 *
 *   Standard C (no nesting):  the comment opened by the first slash-star ends
 *                             at the first star-slash after it, which leaves
 *                             "0 *", then an empty comment, then "1".
 *                             0 * 1 == 0
 *
 *   Nested comments:          the two slash-stars open two levels which are
 *                             closed by the next two star-slashes, leaving
 *                             only "1".
 */
int main()
{
    int nested_comments = /*/*/0*/**/1;

    if (nested_comments)
        printf("Nested comments allowed.\n");
    else
        printf("No nested comments allowed.\n");

    return 0;
}
