/*
 * ROT13 filter: copies standard input to standard output, rotating
 * every letter 13 places through the alphabet.  Running the output
 * through the program again restores the original text.
 *
 * Note: when read() hits end of file the loop still performs one more
 * write() of the last character before stopping, so the output is one
 * byte longer than the input (this is the "difference" the judges ask
 * about).
 */

/* decode one character: letters are rotated by 13, others pass through */
char rot13(char ch)
{
    char upper = ch & ~' ';          /* clear the lowercase bit (0x20) */
    char case_bit = ch - upper;      /* 0 for uppercase, 32 for lowercase */

    if (upper <= '@' || upper > 'Z') /* not a letter */
        return ch;

    /* (upper - 'A' + 13) % 26 + 1 is the 1-based position of the new letter */
    return (case_bit + '@') | ((upper - 52) % 26 + 1);
}

main(Manual)
{
    char ch = 0, out;
    int got, wrote;

    for (;;) {
        got = read(0, &ch, 1);
        out = rot13(ch);
        wrote = write(1, &out, 1);
        if (!(got & wrote))
            break;
    }
}

/* never called: the original only mentions it inside a discarded macro argument */
subr(C)
char *C;
{
    C = "Lint says \"argument Manual isn't used.\" What's that\tmean?";
}
