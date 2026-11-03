#include <stdio.h>

/*
 * rot13 filter.
 *
 * The lookup table is built as four 16-character blocks:
 *
 *   offset  0: "nopqrstuvwxyzzzz"
 *   offset 16: "abcdefghijklmmmm"
 *   offset 32: "NOPQRSTUVWXYZZZZ"
 *   offset 48: "ABCDEFGHIJKLMMMM"
 *
 * Each block is padded to 16 by repeating its last letter.  A letter's
 * rot13 partner sits exactly 16 positions away, so flipping bit 4 of its
 * offset (offset ^ 16) swaps between blocks 0<->1 and 2<->3.
 */
#define BLOCK 16

int main()
{
    char table[4 * BLOCK + 2];
    char *found;
    int pos, letter, ch;
    extern char *index();

    table[4 * BLOCK] = '\0';
    for (pos = 0; pos < BLOCK; pos++) {
        letter = pos > 12 ? 12 : pos; /* clamp to 'M' / 'm' / 'Z' / 'z' */
        table[pos + 3 * BLOCK] = 'A' + letter;
        table[pos + 2 * BLOCK] = 'N' + letter;
        table[pos + 1 * BLOCK] = 'a' + letter;
        table[pos + 0 * BLOCK] = 'n' + letter;
    }

    while ((ch = fgetc(stdin)) != EOF) {
        found = index(table, ch);
        if (found)
            fputc(table[(found - table) ^ BLOCK], stdout);
        else
            fputc(ch, stdout);
    }
}
