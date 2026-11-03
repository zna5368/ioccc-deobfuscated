/*
 * Generates a C program that reads `count` integers and prints them in
 * ascending order using only nested if/else comparisons.
 *
 * Like the original, no headers are included: printf() is implicitly
 * declared and the Makefile force-includes <stdlib.h> for atoi() and exit().
 */

#define MAX_VALUES 26

int count;                   /* number of values the generated program sorts */
char order[MAX_VALUES + 2];  /* current ordering of variable names */

/* parse the decimal digits of the argument by hand (no validation) */
int parse_count(char *text)
{
    int value = 0;

    do {
        value = value * 10 + *text - '0';
        text++;
    } while (*text);

    return value;
}

void print_indent(int level)
{
    do {
        printf("    ");
        level--;
    } while (level);
}

/* print the opening of the generated program: declarations and scanf() */
void print_header(void)
{
    int i;

    printf("main()\n{\n    int a");
    i = 1;
    do {
        printf(",%c", 'a' + i);
        i++;
    } while (i < count);

    printf(";\n\n    scanf(\"%%d");
    i = 1;
    do {
        printf(" %%d");
        i++;
    } while (i < count);
    printf("\"");

    i = 0;
    do {
        printf(",&%c", 'a' + i);
        i++;
    } while (i < count);
    printf(");\n\n");
}

/* print the printf() statement that outputs the values in sorted order */
void print_sorted(int placed)
{
    int i;

    printf("printf(\"");
    i = placed;
    do {
        printf("%%d ");
        i--;
    } while (i);
    printf("\"");

    i = 0;
    do {
        printf(",%c", order[i]);
        i++;
    } while (i < placed);
    printf(");\n");
}

/*
 * order[0..placed-1] holds the variables already placed in sorted order.
 * Insert the next variable at every possible position, from the end to the
 * front, emitting the if/else comparison that selects that position.
 */
void generate(int placed)
{
    char saved[MAX_VALUES + 2];
    int pos, i;

    if (placed == count) {
        print_indent(placed);
        print_sorted(placed);
        return;
    }

    for (i = 0; i <= placed; i++)
        saved[i] = order[i];

    order[placed + 1] = 'a' + placed;
    for (pos = placed + 1; pos > 0; pos--) {
        /* move the new variable one slot toward the front */
        char tmp = order[pos];
        order[pos] = order[pos - 1];
        order[pos - 1] = tmp;

        print_indent(placed);
        if (pos <= placed)
            printf("else ");
        if (pos > 1)
            printf("if (%c<%c)", order[pos - 2], order[pos - 1]);
        printf("\n");

        generate(placed + 1);
    }

    for (i = 0; i <= placed; i++)
        order[i] = saved[i];
}

int main(int argc, char **argv)
{
    if (!argv[1] || atoi(argv[1]) <= 0 || atoi(argv[1]) >= 27)
        exit(1);

    count = parse_count(argv[1]);

    print_header();

    order[0] = 'a';
    generate(1);

    printf("}\n");
    exit(0);
}
