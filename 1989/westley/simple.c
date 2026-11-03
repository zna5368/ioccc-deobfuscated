/*
 * The original program has no #include lines and relies on getchar() and
 * putchar() being implicitly declared, so this version does the same.
 */

#define END_OF_FILE (-1)

/* the argument count picks the mode: bit 0 = ROT13, bit 1 = reverse */
#define MODE_ROT13 1
#define MODE_REVERSE 2

/*
 * Rotate a letter 13 places through the alphabet.  This is the same formula
 * as the original: (c & 31) is the letter's position (A/a = 1), adding 12 and
 * taking it mod 26 gives the position 13 letters further on (counting from 0),
 * (c & 32) keeps the lower case bit, and 65 is 'A'.
 */
int rot13(int c)
{
    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
        return (((c & 31) + 12) % 26) + (c & 32) + 'A';
    return c;
}

/*
 * Read one character and handle the rest of the input recursively.  When
 * copying forwards the character is printed before the recursive call; when
 * reversing it is printed afterwards, so the last character read comes out
 * first.
 */
void filter(int mode)
{
    int c = getchar();

    if (mode & MODE_ROT13)
        c = rot13(c);

    if (c == END_OF_FILE)
        return;

    if (!(mode & MODE_REVERSE))
        putchar(c);

    filter(mode);

    if (mode & MODE_REVERSE)
        putchar(c);
}

int main(int argc, char **argv)
{
    /* no arguments: copy, 1: ROT13, 2: reverse, 3: ROT13 and reverse */
    int mode = argc - 1;

    (void)argv; /* only the number of arguments matters */

    filter(mode);
    return 0;
}
