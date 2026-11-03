#include <stdio.h>
#include <stdlib.h>

/*
 * The world's slowest factorial program.
 *
 * This file is a "generation" of a program that rewrites its own source code.
 * The two #define lines below are the only state. Every generation is compiled
 * and then fed its own source on stdin. It writes the next generation to
 * stdout with the lines changed to:
 *
 *     COUNTER   -> COUNTER + 1
 *     FACTORIAL -> FACTORIAL * COUNTER
 *
 * So the generation with COUNTER == k has FACTORIAL == (k - 1)!. When COUNTER
 * reaches n + 1 the answer n! is printed instead and the program exits with a
 * non-zero status to tell the driver script (simple.sh) to stop.
 */
#define COUNTER 1
#define FACTORIAL 1

int main(int argc, char **argv)
{
    char line[256];
    int value;
    int counter_rewritten = 0;
    int factorial_rewritten = 0;

    (void)argc;

    /* Last generation: report the answer and stop the loop */
    if (COUNTER == atoi(argv[1]) + 1) {
        (void)fprintf(stderr, "%s factorial = %d\n", argv[1], FACTORIAL);
        exit(1);
    }

    /* Copy our own source from stdin to stdout, updating the two #defines */
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (!counter_rewritten &&
            sscanf(line, "#define COUNTER %d", &value) == 1) {
            (void)printf("#define COUNTER %d\n", value + 1);
            counter_rewritten = 1;
        } else if (!factorial_rewritten &&
                   sscanf(line, "#define FACTORIAL %d", &value) == 1) {
            (void)printf("#define FACTORIAL %d\n", value * COUNTER);
            factorial_rewritten = 1;
        } else {
            (void)fputs(line, stdout);
        }
    }
    exit(0);
}
