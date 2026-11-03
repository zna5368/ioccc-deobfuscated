#include <stdio.h>

/*
 * Roman numeral converter.
 *
 * If standard output is a terminal, every line of input is read as a decimal
 * number and printed as a lower case Roman numeral.
 *
 * Otherwise every Roman numeral found in the input is rewritten as an
 * arithmetic expression, so that the output can be piped into bc(1):
 * "x*x" becomes "(10)*(10)" and "iv" becomes "(1-2+5)".
 *
 * The original program drove all of this through SIGPIPE: it closed the read
 * end of a pipe, wrote to it forever, and used signal(2) to choose which
 * function would run next.  Here the same steps are ordinary loops.
 */

int atoi();
int isatty();
void exit();

#define LINE_SIZE 120 /* 'x' */
#define SIGPIPE_EXIT 13 /* exit() was installed as the SIGPIPE handler */

/* value of a Roman digit, 0 for anything else */
int roman_value(int ch)
{
    switch (ch) {
    case 'i': case 'I': return 1;
    case 'v': case 'V': return 5;
    case 'x': case 'X': return 10;
    case 'l': case 'L': return 50;
    case 'c': case 'C': return 100;
    case 'd': case 'D': return 500;
    case 'm': case 'M': return 1000;
    case 'w': case 'W': return 5000;
    }
    return 0;
}

/* print n as a lower case Roman numeral followed by a newline */
void print_roman(int n)
{
    /* one, five and ten for hundreds, tens and units */
    const char *digits = "ivxlcdm" + 4;
    int place = 100;

    while (n >= 1000) {
        n -= 1000;
        putchar('m');
    }
    while (n > 0) {
        int d = n / place;
        char one = digits[0], five = digits[1], ten = digits[2];

        if (d == 9) {
            putchar(one);
            putchar(ten);
        } else if (d == 4) {
            putchar(one);
            putchar(five);
        } else {
            if (d >= 5) {
                putchar(five);
                d -= 5;
            }
            while (d-- > 0)
                putchar(one);
        }
        n %= place;
        place /= 10;
        digits -= 2;
    }
    putchar('\n');
}

/* terminal output: decimal numbers to Roman numerals, one per line */
void decimal_to_roman(void)
{
    char line[LINE_SIZE];

    while (fgets(line, LINE_SIZE, stdin))
        print_roman(atoi(line));
    exit(0);
}

/* piped output: rewrite Roman numerals as expressions for bc */
void roman_to_expression(void)
{
    for (;;) {
        int prev = 0; /* value of the previous digit, 0 if not in a numeral */
        int value;
        char ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            value = roman_value(ch);
            if (value) {
                if (prev) {
                    /* a smaller digit before a larger one is subtracted */
                    if (prev < value)
                        printf("%d", -2 * prev);
                    printf("+%d", value);
                } else {
                    printf("(%d", value);
                }
            } else {
                if (prev)
                    printf(")");
                printf("%c", ch);
            }
            prev = value;
        }
        if (prev)
            printf(")");

        if (ch == EOF) {
            printf("\t");
            fflush(stdout);
            exit(SIGPIPE_EXIT);
        }
        printf("\n");
        fflush(stdout);
    }
}

int main()
{
    if (isatty(1))
        decimal_to_roman();
    else
        roman_to_expression();
    return 0;
}
