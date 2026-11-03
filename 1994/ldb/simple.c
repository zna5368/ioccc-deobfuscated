/*
 * Print one randomly chosen line from stdin using reservoir sampling.
 *
 * Each new line number n replaces the currently chosen line with
 * probability 1/n, so every line ends up equally likely to be printed
 * without having to store the whole input.
 */

#define MAX_LINE 232

char chosen[MAX_LINE];  /* the line that will be printed */
char discard[MAX_LINE]; /* lines that were not picked are read here */

int main(void) {
    int line_count = 1;
    char *buffer;

    srand(time(NULL));

    for (;;) {
        /* pick line_count's line with probability 1 / line_count */
        if (rand() % line_count++ == 0)
            buffer = chosen;
        else
            buffer = discard;

        if (fgets(buffer, MAX_LINE, stdin) == NULL)
            break;
    }

    puts(chosen);
    return 0;
}
