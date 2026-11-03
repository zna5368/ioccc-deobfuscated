#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LINES 500
#define MAX_LINE_LEN 99

char lines[MAX_LINES][MAX_LINE_LEN];

/*
 * Raise *result to an integer power: *result = base ^ (int)exponent.
 * Only non-negative integer exponents are meaningful.
 */
void power(float *result, float base, float exponent)
{
    int count;

    *result = 1;
    for (count = exponent; count--;)
        *result *= base;
}

/* Apply one operator to the running accumulator. */
void apply(float *accumulator, char op, float operand)
{
    if (op == '+')
        *accumulator += operand;
    if (op == '-')
        *accumulator -= operand;
    if (op == '*')
        *accumulator *= operand;
    if (op == '/')
        *accumulator /= operand;
    if (op == '^')
        power(accumulator, *accumulator, operand);
}

/*
 * Evaluate the expression f(x, y) strictly left to right.
 * Operands are 'x', 'y' or a single digit; any other character
 * becomes the operator applied to the next operand.
 */
float evaluate(const char *expr, float x, float y)
{
    float result = 0;
    char op = '+';

    for (; *expr; expr++) {
        if (*expr == 'y')
            apply(&result, op, y);
        else if (*expr >= '0' && *expr <= '9')
            apply(&result, op, (float)*expr - '0');
        else if (*expr == 'x')
            apply(&result, op, x);
        else
            op = *expr;
    }
    return result;
}

/* Read up to MAX_LINES lines from stdin and print them in reverse order. */
void reverse_lines(void)
{
    int count = 0;

    while (fgets(lines[count++], MAX_LINE_LEN, stdin))
        lines[count - 1][strlen(lines[count - 1]) - 1] = '\0'; /* drop newline */
    count--;
    while (count--)
        puts(lines[count]);
}

int main(int argc, char *argv[])
{
    char *expr;
    float x, x_end, step, half_step, y;
    float k1, k2, k3_twice, x_mid;

    if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL ||
        argv[4] == NULL || argv[5] == NULL)
        return 1;

    expr = argv[1];
    if (!strcmp(expr, "-r"))
        reverse_lines();

    x = atof(argv[2]);
    x_end = atof(argv[3]);
    step = atof(argv[4]);
    y = atof(argv[5]);

    /* Fourth order Runge-Kutta integration of y' = f(x, y) */
    while (x < x_end) {
        half_step = step / 2;
        k1 = evaluate(expr, x, y);
        x_mid = x + half_step;
        k2 = evaluate(expr, x_mid, y + half_step * k1);
        k3_twice = 2 * evaluate(expr, x_mid, y + half_step * k2);
        y += step * k2 / 3 + step * k3_twice / 6 +
             step * evaluate(expr, x += step, y + half_step * k3_twice) / 6 +
             half_step * k1 / 3;
        printf("%f %f\n", x, y);
    }
    return 0;
}
