/*
 * The original smr.c is an empty (zero byte) file. The build copies it to an
 * executable named smr, which the shell runs as an empty script that prints
 * nothing. Printing nothing is an exact copy of the empty source, so it
 * reproduces itself.
 *
 * A C compiler needs a main function, so this version spells out the same
 * behavior: the source listing of the original program is empty, so print
 * nothing and exit successfully.
 */
int main(void)
{
    /* the original source has no characters, so there is nothing to print */
    return 0;
}
