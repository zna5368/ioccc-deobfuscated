#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * Shamir's Secret Sharing over the field GF(256).
 *
 * Every argument names a point x (0..255) and a file:
 *   x-file   input share: the file holds the byte values of the polynomial at x
 *   x+file   output share: the file receives the byte values at x
 *
 * For every byte position, the input points define a polynomial and the
 * output points are found by Lagrange interpolation.  Splitting a secret is
 * "-secret 1-/dev/urandom ... 1+share1 2+share2 ...", the secret being the
 * value at x = 0 and the random files the polynomial coefficients.
 * Restoring is "1-share1 2-share2 ... +secret".
 */

#define FIELD_SIZE 256
#define MAX_POINTS 256

/* Field arithmetic tables */
static int product[FIELD_SIZE][FIELD_SIZE]; /* product[a][b] = a * b       */
static int inverse[FIELD_SIZE];             /* a * inverse[a] = 1          */

/* Input shares */
static int input_count;
static int input_x[MAX_POINTS];
static int input_denominator[MAX_POINTS]; /* prod_{m != j} (x_j - x_m)  */
static int input_byte[MAX_POINTS];
static FILE *input_file[MAX_POINTS];

/* Output shares */
static int output_count;
static int output_x[MAX_POINTS];
static int output_numerator[MAX_POINTS]; /* prod_j (x - x_j), 0 if x is an input */
static int output_byte[MAX_POINTS];
static FILE *output_file[MAX_POINTS];

enum error {
    ERR_POINT_TOO_LARGE,
    ERR_DUPLICATE_INPUT,
    ERR_OPEN_INPUT,
    ERR_TOO_MANY_OUTPUTS,
    ERR_OPEN_OUTPUT,
    ERR_BAD_SYNTAX,
    ERR_NO_INPUTS,
    ERR_NO_OUTPUTS
};

static void fail(enum error error)
{
    const char *message = NULL;

    switch (error) {
    case ERR_POINT_TOO_LARGE:
        message = " Point Value Too Large";
        break;
    case ERR_DUPLICATE_INPUT:
        message = " Duplicate Input Point";
        break;
    case ERR_OPEN_INPUT:
        message = " Failed To Open Input File";
        break;
    case ERR_TOO_MANY_OUTPUTS:
        message = " Too Many Output Points";
        break;
    case ERR_OPEN_OUTPUT:
        /* the original reports output failures with the input message */
        message = " Failed To Open Input File";
        break;
    case ERR_BAD_SYNTAX:
        message = " Bad Argument Syntax";
        break;
    case ERR_NO_INPUTS:
        message = " No Input Files";
        break;
    case ERR_NO_OUTPUTS:
        message = " No Output Files";
        break;
    }
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

static int multiply(int a, int b);

/*
 * Product of the powers of two 2^i and 2^j (i, j < 8) in the field.
 *
 * 2^i is the product of the "Fermat powers" 2^n where n runs over the bits of
 * i (n = 1, 2, 4).  Fermat powers that appear only once multiply like normal
 * numbers (giving 2^(i^j)), while a Fermat power F that appears in both
 * squares to F + F/2.
 */
static int multiply_powers_of_two(int i, int j)
{
    int shared = i & j;
    int distinct = i ^ j;
    int result = 1;
    int n;

    for (n = 1; n <= shared; n <<= 1) {
        if (shared & n)
            result = multiply(result, 1 << n | 1 << (n - 1));
    }
    if (result > 1)
        return multiply(result, 1 << distinct);
    return 1 << distinct;
}

/*
 * Field multiplication (nim-multiplication): addition is XOR, and the product
 * is the XOR of the products of every pair of set bits.  Results are cached
 * in the product table.
 */
static int multiply(int a, int b)
{
    int result = product[a][b];
    int i, j;

    if (result)
        return result;
    for (i = 0; a >> i; i++) {
        for (j = 0; b >> j; j++) {
            if (((a >> i) & 1) && ((b >> j) & 1))
                result ^= multiply_powers_of_two(i, j);
        }
    }
    product[a][b] = product[b][a] = result;
    return result;
}

/* Fill the multiplication and inverse tables. */
static void build_tables(void)
{
    int a, b;

    for (a = 0; a < FIELD_SIZE; a++) {
        for (b = 0; b <= a; b++) {
            if (multiply(a, b) == 1) {
                inverse[a] = b;
                inverse[b] = a;
            }
        }
    }
}

/* Read the number in front of '-' or '+'. */
static int parse_point(const char **arg)
{
    const char *p = *arg;
    int x = 0;

    while (*p >= '0' && *p <= '9') {
        x = x * 10 + (*p - '0');
        if (x >= FIELD_SIZE)
            fail(ERR_POINT_TOO_LARGE);
        p++;
    }
    *arg = p;
    return x;
}

static void parse_arguments(int argc, char *argv[])
{
    int k, m;

    for (k = 1; k < argc; k++) {
        const char *arg = argv[k];
        int x = parse_point(&arg);

        if (*arg == '-') {
            for (m = 0; m < input_count; m++) {
                if (input_x[m] == x)
                    fail(ERR_DUPLICATE_INPUT);
            }
            input_x[input_count] = x;
            input_file[input_count] = fopen(arg + 1, "r");
            if (!input_file[input_count])
                fail(ERR_OPEN_INPUT);
            input_count++;
        } else if (*arg == '+') {
            if (output_count >= MAX_POINTS)
                fail(ERR_TOO_MANY_OUTPUTS);
            output_x[output_count] = x;
            output_file[output_count] = fopen(arg + 1, "w");
            if (!output_file[output_count])
                fail(ERR_OPEN_OUTPUT);
            output_count++;
        } else {
            fail(ERR_BAD_SYNTAX);
        }
    }
    if (input_count == 0)
        fail(ERR_NO_INPUTS);
    if (output_count == 0)
        fail(ERR_NO_OUTPUTS);
}

/* Precompute the parts of the Lagrange basis polynomials that never change. */
static void prepare_interpolation(void)
{
    int i, j, n;

    /* in GF(2^n) subtraction is XOR */
    for (i = 0; i < input_count; i++) {
        n = 1;
        for (j = 0; j < input_count; j++) {
            if (j != i)
                n = product[n][input_x[i] ^ input_x[j]];
        }
        input_denominator[i] = n;
    }
    for (i = 0; i < output_count; i++) {
        n = 1;
        for (j = 0; j < input_count; j++)
            n = product[n][output_x[i] ^ input_x[j]];
        output_numerator[i] = n;
    }
}

/*
 * Evaluate the polynomial through the current input bytes at every output
 * point:  f(x) = sum_j y_j * prod_{m != j} (x - x_m) / (x_j - x_m)
 */
static void interpolate(void)
{
    int i, j, value;

    for (i = 0; i < output_count; i++) {
        value = 0;
        if (output_numerator[i] == 0) {
            /* the output point is one of the input points: copy it */
            for (j = 0; j < input_count; j++) {
                if (output_x[i] == input_x[j])
                    value = input_byte[j];
            }
        } else {
            for (j = 0; j < input_count; j++) {
                int denominator = product[input_denominator[j]]
                                         [output_x[i] ^ input_x[j]];
                int basis = product[output_numerator[i]][inverse[denominator]];
                value ^= product[input_byte[j]][basis];
            }
        }
        output_byte[i] = value;
    }
}

int main(int argc, char *argv[])
{
    int k;

    build_tables();
    parse_arguments(argc, argv);
    prepare_interpolation();

    for (;;) {
        for (k = 0; k < input_count; k++) {
            int c = getc(input_file[k]);
            if (c == EOF)
                exit(0);
            input_byte[k] = c;
        }
        interpolate();
        for (k = 0; k < output_count; k++)
            putc(output_byte[k], output_file[k]);
    }
}
