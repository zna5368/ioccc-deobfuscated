#include <stdlib.h>
#include <stdio.h>

/*
 * Find short arithmetic expressions built from a set of numbers.
 *
 *   ./simple number .. 0 target ..
 */

#define LIMIT (1 << 15)        /* all values must lie in [1, LIMIT) */

/* operator stored for each value (0 means the value is a given number) */
enum { LEAF = 0, MUL = 1, DIV = 2, ADD = 3, SUB = 4 };

/* "slot" an expression is printed in, i.e. what operator sits to its left */
enum { CTX_MUL = 0, CTX_DIV = 1, CTX_ADD = 2, CTX_SUB = 3 };

static const char symbol[] = "*/+-";   /* indexed by operator - 1 */

static int length[LIMIT];       /* length of shortest known expression, 0 = unknown */
static int operation[LIMIT];    /* operator used at the top of that expression */
static int left_operand[LIMIT]; /* larger operand */
static int right_operand[LIMIT];/* smaller operand */
static int found[LIMIT + 1024]; /* every value reached, in discovery order */

static int is_additive(int value)
{
    return operation[value] == ADD || operation[value] == SUB;
}

/* record "big op small = result" if it is new or shorter than before */
static void consider(int result, int len, int op, int big, int small, int *count)
{
    if (result <= 0 || result >= LIMIT)
        return;
    if (length[result] == 0)
        found[(*count)++] = result;
    else if (length[result] <= len)
        return;
    right_operand[result] = small;
    length[result] = len;
    operation[result] = op;
    left_operand[result] = big;
}

/*
 * Print the expression for value.  context tells which operator precedes it:
 *  - an additive expression in a multiplicative slot needs parentheses;
 *  - an expression to the right of '-' (or of '/') is printed without
 *    parentheses by inverting its operators: a - (b + c) -> a - b - c,
 *    a / (b * c) -> a / b / c.
 */
static void print_expr(int value, int context)
{
    int op = operation[value];
    int sym, inverted, shown, left_ctx, right_ctx;
    int parens;

    if (op == LEAF) {
        printf("%d", value);
        return;
    }

    sym = op - 1;                               /* 0..3 as in symbol[] */
    parens = is_additive(value) && (context == CTX_MUL || context == CTX_DIV);
    if (is_additive(value))
        inverted = (context == CTX_SUB);
    else
        inverted = (context == CTX_DIV);

    shown = inverted ? sym ^ 1 : sym;           /* swap * <-> /, + <-> - */
    left_ctx = (sym & 2) | inverted;            /* MUL/DIV or ADD/SUB slot */
    right_ctx = shown;

    if (parens)
        printf("(");
    print_expr(left_operand[value], left_ctx);
    printf(" %c ", symbol[shown]);
    print_expr(right_operand[value], right_ctx);
    if (parens)
        printf(")");
}

int main(int argc, char **argv)
{
    int count = 0;
    int i, j, op;

    /* read the given numbers up to the 0 separator; length = digit count */
    while (--argc > 0) {
        int number = atoi(*++argv);
        int rest = number;

        if (number <= 0 || number >= LIMIT)
            break;
        do
            length[number]++;
        while (rest /= 10);
        found[count++] = number;
    }

    /* combine every pair of reached values, growing the list as we go */
    for (i = 0; i < count; i++) {
        for (j = 0; j <= i; j++) {
            int a = found[i], b = found[j];
            int big = a > b ? a : b;
            int small = a > b ? b : a;
            int len = length[a] + length[b] + 3;   /* " op " is 3 chars */

            for (op = SUB; op >= MUL; op--) {
                int result = 0;

                switch (op) {
                case SUB:
                    result = big - small;
                    break;
                case ADD:
                    result = big + small;
                    break;
                case DIV:
                    /* additive operands need "(" ")"; kept for MUL too */
                    len += 2 * (is_additive(small) + is_additive(big));
                    if (big % small == 0)
                        result = big / small;
                    break;
                case MUL:
                    result = big * small;
                    break;
                }
                consider(result, len, op, big, small, &count);
            }
        }
    }

    /* print the expression for every target that was reached */
    while (--argc > 0) {
        int target = atoi(*++argv);

        if (target > 0 && target < LIMIT && length[target]) {
            print_expr(target, CTX_ADD);
            printf(" = %d\n", target);
        }
    }
    return 0;
}
