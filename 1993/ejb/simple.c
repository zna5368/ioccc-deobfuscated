/*
 * 1993/ejb - Tower of Hanoi (positive level) or the Patience puzzle,
 * also known as the Chinese rings (negative level).
 *
 * Like the original, no headers are included: atoi, malloc, memset,
 * printf and puts are all implicitly declared.
 */

static signed char invalid_msg[] = "invalid";
static signed char move_fmt[] = "move ring %d from stack %d to stack %d\n";

/* Patience puzzle state: one character per ring, ring 1 is the rightmost. */
static signed char *rings;
static int ring_count;

/* Move rings 1..n from peg 'from' to peg 'to' (pegs are numbered 1, 2, 3). */
static void hanoi(int n, int from, int to)
{
    int spare = 6 - from - to;

    if (n != 1)
        hanoi(n - 1, from, spare);
    printf(move_fmt, n, from, to);
    if (n != 1)
        hanoi(n - 1, spare, to);
}

/* Set ring i to state ('1' = on the U, '0' = off) and show the puzzle. */
static void set_ring(int i, int state)
{
    rings[ring_count - i] = state;
    puts(rings);
}

/*
 * Change rings 1..n so they are all in the given state.
 *
 * A ring can only be moved when the ring just below it is on and every
 * ring below that is off (ring 1 can always be moved).
 */
static void patience(int n, int state)
{
    int other = state == '1' ? '0' : '1';

    if (n == 1) {
        set_ring(1, state);
    } else if (n == 2) {
        if (state == '1') {
            set_ring(1, state);
            set_ring(2, state);
        } else {
            set_ring(2, state);
            set_ring(1, state);
        }
    } else if (state == '1') {
        /* put rings 1..n-1 on, take 1..n-2 off, put ring n on, restore */
        patience(n - 1, state);
        patience(n - 2, other);
        set_ring(n, state);
        patience(n - 2, state);
    } else {
        /* take 1..n-2 off, take ring n off, restore 1..n-2, take 1..n-1 off */
        patience(n - 2, state);
        set_ring(n, state);
        patience(n - 2, other);
        patience(n - 1, state);
    }
}

int main(int argc, char *argv[])
{
    int level;

    if (argc != 2) {
        puts(invalid_msg);
        return 1;
    }

    level = atoi(argv[1]);
    if (level < -128 || level > 127 || level == 0) {
        puts(invalid_msg);
        return 1;
    }

    if (level > 0) {
        hanoi(level, 1, 2);
    } else {
        /*
         * Start with every ring on the U.  Like the original, the buffer
         * holds exactly ring_count bytes and is not NUL terminated.
         */
        ring_count = -level;
        rings = (signed char *)malloc(ring_count);
        memset(rings, '1', ring_count);
        puts(rings);
        patience(ring_count, '0');
    }
    return 0;
}
