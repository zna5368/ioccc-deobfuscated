/*
 * Recursive descent calculator. Numbers are octal by default, 'x'/'X'
 * prefixes a hexadecimal number and 'd'/'D' prefixes a decimal number.
 * The result is printed in octal.
 *
 *   expr       ::= term expr_rest
 *   expr_rest  ::= '+' term expr_rest | '-' term expr_rest | (empty)
 *   term       ::= factor term_rest
 *   term_rest  ::= '*' factor term_rest | '/' factor term_rest
 *                | '%' factor term_rest | (empty)
 *   factor     ::= '+' factor | '-' factor | '(' expr ')'
 *                | ('d'|'D') decimal | ('x'|'X') hex | octal
 *
 * There is no error checking: a bad digit simply ends the number, and the
 * closing parenthesis is skipped without being looked at.
 */

static char *cursor; /* next character of the expression */

static int parse_expr(void);

static int parse_octal(int value)
{
    while (*cursor >= '0' && *cursor <= '7')
        value = value * 8 + (*cursor++ - '0');
    return value;
}

static int parse_decimal(int value)
{
    while (*cursor >= '0' && *cursor <= '9')
        value = value * 10 + (*cursor++ - '0');
    return value;
}

static int parse_hex(int value)
{
    for (;;) {
        if (*cursor >= '0' && *cursor <= '9')
            value = value * 16 + (*cursor++ - '0');
        else if (*cursor >= 'A' && *cursor <= 'F')
            value = value * 16 + (*cursor++ - 'A' + 10);
        else if (*cursor >= 'a' && *cursor <= 'f')
            value = value * 16 + (*cursor++ - 'a' + 10);
        else
            return value;
    }
}

static int parse_factor(void)
{
    int value;

    switch (*cursor) {
    case '(':
        cursor++;
        value = parse_expr();
        cursor++; /* skip ')' (not checked) */
        return value;
    case '-':
        cursor++;
        return -parse_factor();
    case '+':
        cursor++;
        return parse_factor();
    case 'x':
    case 'X':
        cursor++;
        return parse_hex(0);
    case 'd':
    case 'D':
        cursor++;
        return parse_decimal(0);
    default:
        return parse_octal(0);
    }
}

static int parse_term(void)
{
    int value = parse_factor();

    for (;;) {
        switch (*cursor) {
        case '*':
            cursor++;
            value = value * parse_factor();
            break;
        case '%':
            cursor++;
            value = value % parse_factor();
            break;
        case '/':
            cursor++;
            value = value / parse_factor();
            break;
        default:
            return value;
        }
    }
}

static int parse_expr(void)
{
    int value = parse_term();

    for (;;) {
        switch (*cursor) {
        case '+':
            cursor++;
            value = value + parse_term();
            break;
        case '-':
            cursor++;
            value = value - parse_term();
            break;
        default:
            return value;
        }
    }
}

int main(int argc, char **argv)
{
    /* the original only does anything when given exactly one argument */
    if (argc != 2)
        return 0;

    cursor = argv[1];
    return printf("%o\n", parse_expr());
}
