#define _POSIX_SOURCE
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t waiting;

static void on_alarm(int sig)
{
    (void)sig;
    waiting = 0; /* stop the busy loop */
}

int main(int argc, char **argv)
{
    unsigned int bits;
    unsigned int counter;

    /* the original returns at once if given four or more arguments */
    if (argc > 4)
        return 0;

    /*
     * Number of bits is taken from the last argument. With no argument
     * atoi("./yarng") is 0, so the count wraps around and runs (nearly)
     * forever.
     */
    bits = (unsigned int)atoi(argv[argc - 1]);
    counter = bits;

    do {
        signal(SIGALRM, on_alarm);
        waiting = 1;
        counter += alarm(1);

        /* spin for one second; the count is unpredictable */
        while (waiting)
            ++counter;

        /* print the lowest bit of the counter; one bit fewer to go */
        bits -= (unsigned int)write(1, (counter & 1) ? "1" : "0", 1);
    } while (bits != 0);

    return write(1, "\n", 1) != 1;
}
