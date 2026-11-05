/*
 * Presidents of the United States of America.
 *
 * Usage: ./simple name republican democrat
 *
 * There is no list of names and no table of parties.  A president's party is
 * decided by a hash of the first four letters of the name, and the two numbers
 * below were picked so that the hash gives the right answer for every lower
 * case last name from Franklin Pierce onwards.  A name that is not in that set
 * still gets an answer, so the same trick works on any name and any two groups.
 */

#define NAME_ARG 1
#define REPUBLICAN_ARG 2
#define DEMOCRAT_ARG 3

/* These two values are what makes the hash sort the presidents correctly. */
#define FIRST_MODULUS 4796
#define SECOND_MODULUS 275

/*
 * Treat the first four bytes of the name as one number.  On a little endian
 * CPU the first letter is the lowest byte of that number.  A name shorter than
 * three letters reaches past its terminating zero into the next argument.
 */
static int first_four_letters(const char *name)
{
    return *(const int *)name;
}

int main(int argc, char **argv)
{
    int letters = first_four_letters(argv[NAME_ARG]);
    int hash = letters % FIRST_MODULUS % SECOND_MODULUS;

    /*
     * The last step of the hash is a remainder by the argument count.  With
     * the usual three arguments argc is 4, and a remainder of zero means
     * democrat.
     */
    if (hash % argc == 0) {
        puts(argv[DEMOCRAT_ARG]);
    } else {
        puts(argv[REPUBLICAN_ARG]);
    }

    return 0;
}
