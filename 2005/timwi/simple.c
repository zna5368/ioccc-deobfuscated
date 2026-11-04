#include <stdio.h>

#define MEMORY_SIZE 3125 /* 5 * 5 * 5 * 5 * 5 */

/*
 * The input integers and the BF program share one array: first the integers
 * read by scanf(), then one unused slot (where scanf() failed on the ':'),
 * then one int per program character as it is read with getchar().
 */
int input_and_program[MEMORY_SIZE];
int tape[MEMORY_SIZE];
int *cell = tape;                     /* BF data pointer */
int *next_input = input_and_program;  /* next integer for ',' */

/* Flags passed to run() */
#define RESUME_READING 1 /* when this loop ends, read new characters again */
#define READ_SOURCE 2    /* read each instruction from stdin and store it */
#define EXECUTE 4        /* perform instructions (clear while skipping) */

enum op { OP_NONE, OP_INC, OP_DEC, OP_LEFT, OP_RIGHT, OP_OUTPUT, OP_INPUT,
          OP_OPEN, OP_CLOSE };

/*
 * Instructions are not recognised by their character but by remainders, so
 * many other characters act as instructions too (whitespace does nothing).
 */
enum op classify(int c)
{
    if (c % 7 == 0)
        return OP_OPEN;                        /* '[' */
    if (c % 5 == 0) {
        if (c % 2)
            return OP_DEC;                     /* '-' */
        return c % 4 == 0 ? OP_LEFT : OP_NONE; /* '<' */
    }
    if (c % 3 == 0)
        return c % 5 == 3 ? OP_CLOSE : OP_NONE; /* ']' */
    if (c % 4) {
        if (c % 2)
            return OP_INC;                     /* '+' */
        if (c % 3 == 2)
            return OP_RIGHT;                   /* '>' */
        return c % 5 == 1 ? OP_OUTPUT : OP_NONE; /* '.' */
    }
    return c % 8 ? OP_INPUT : OP_NONE;         /* ',' */
}

/*
 * Interpret the program starting at 'start' until a ']' (or end of input).
 * Returns the distance from 'start' to the ']' plus 2, or the distance to
 * the end of input plus 1. A '[' uses this to know where its loop ends.
 */
int run(int mode, int *start)
{
    int *pc = start;

    for (;;) {
        int c;

        if (mode & READ_SOURCE) {
            *pc = getchar();
            if (*pc == 0)
                return pc - start + 2;
        }
        c = *pc;
        if (c < 1)
            return pc - start + 1;

        switch (classify(c)) {
        case OP_CLOSE:
            return pc - start + 2;

        case OP_OPEN:
            if ((mode & EXECUTE) && *cell) {
                /* run the loop body once */
                if (!run(mode & ~RESUME_READING, pc + 1))
                    return pc - start;
                /* then come back to this '[' using the stored copy */
                if (mode & READ_SOURCE)
                    mode = (mode & ~READ_SOURCE) | RESUME_READING;
            } else {
                /* skip (or read past) the loop body to find its ']' */
                int length = run(mode & READ_SOURCE, pc + 1);

                if (!length)
                    return pc - start;
                if (!(mode & READ_SOURCE))
                    mode = (mode & EXECUTE) |
                           (mode & RESUME_READING ? READ_SOURCE : 0);
                pc += length;
            }
            continue;

        case OP_INC:
            if (mode & EXECUTE)
                (*cell)++;
            break;
        case OP_DEC:
            if (mode & EXECUTE)
                (*cell)--;
            break;
        case OP_RIGHT:
            if (mode & EXECUTE)
                cell++;
            break;
        case OP_LEFT:
            if (mode & EXECUTE)
                cell--;
            break;
        case OP_OUTPUT:
            if (mode & EXECUTE)
                printf("%d\n", *cell);
            break;
        case OP_INPUT:
            if (mode & EXECUTE)
                *cell = *next_input++;
            break;
        case OP_NONE:
            break;
        }
        pc++;
    }
}

int main(int argc, char **argv)
{
    int *slot = input_and_program;

    (void)argv;
    if (argc != 1)
        return 2; /* no command-line arguments are supported */

    /* read integers until scanf() fails on the ':' */
    while (scanf("%i", slot))
        slot++;

    return run(READ_SOURCE | EXECUTE, slot + 1);
}
