#include <stdio.h>

/*
 * Usage: schweikh2 [num_ops [first [step [goal]]]]
 *
 * Build every expression "a0 op a1 op a2 ... op aN" whose operands form an
 * arithmetic sequence (a0 = first, a(k+1) = a(k) + step) and whose operators
 * are any combination of * / + -.  Each expression is evaluated with normal
 * precedence using exact fractions.  Expressions whose value equals the goal
 * are printed; a goal of 0/0 prints every expression.
 */

typedef struct {
  long num, den;
} Fraction;

typedef unsigned long OperatorMask;

#define MAX_TERMS 20

enum { ARG_NUM_OPS, ARG_FIRST, ARG_STEP, ARG_GOAL, NUM_ARGS };

/* Operator codes stored two bits per operator in an OperatorMask. */
enum { OP_MUL = 0, OP_DIV = 1, OP_ADD = 2, OP_SUB = 3 };
static const char operator_symbols[] = "*/+-";

static Fraction operands[MAX_TERMS];
static Fraction terms[MAX_TERMS];
static Fraction args[NUM_ARGS] = {
    {9, 1},  /* number of operators */
    {1, 1},  /* first operand */
    {1, 1},  /* increment between operands */
    {42, 1}, /* goal value (0/0 = print everything) */
};

/* Last printf/sscanf return value; becomes the exit status. */
static int last_result;

static void print_fraction(Fraction f) {
  if (f.den != 1) {
    last_result = printf("%ld/%ld", f.num, f.den);
  } else {
    last_result = printf("%ld", f.num);
  }
}

/* Divide numerator and denominator by their greatest common divisor. */
static void reduce(Fraction *f) {
  long a = f->num, b = f->den;

  if (b != 0) {
    while (a != 0) {
      long remainder = b % a;
      b = a;
      a = remainder;
    }
    b = b < 0 ? -b : b;
    f->num /= b;
    f->den /= b;
  }
}

int main(int argc, char *argv[]) {
  OperatorMask ops, mask;
  int i, term;
  long num_ops;

  /* Read the optional "num" or "num/den" arguments. */
  for (i = 1; i <= NUM_ARGS; i++) {
    if (argc > i) {
      last_result =
          sscanf(argv[i], "%ld/%ld", &args[i - 1].num, &args[i - 1].den);
    }
    reduce(&args[i - 1]);
  }
  num_ops = args[ARG_NUM_OPS].num;

  /* Build the arithmetic sequence of operands. */
  operands[0] = args[ARG_FIRST];
  for (i = 0; i < num_ops; i++) {
    operands[i + 1].num = operands[i].num * args[ARG_STEP].den +
                          args[ARG_STEP].num * operands[i].den;
    operands[i + 1].den = operands[i].den * args[ARG_STEP].den;
    reduce(&operands[i + 1]);
  }

  /*
   * Each mask encodes one operator combination, two bits per operator
   * (lowest bits = first operator).  Count down from 4^num_ops - 1 to 0;
   * decrementing past 0 wraps to all ones, which ends the loop.
   */
  for (mask = ~(~0 << num_ops * 2); mask != ~(OperatorMask)0; mask--) {

    /* Pass one: multiply and divide, collecting the terms of the sum. */
    term = 0;
    terms[term] = operands[0];
    for (ops = mask, i = 1; i <= num_ops; i++, ops /= 4) {
      int op = ops & 3;

      if (op == OP_DIV) {
        terms[term].num *= operands[i].den;
        terms[term].den *= operands[i].num;
        if (terms[term].den < 0) {
          terms[term].num *= -1;
          terms[term].den *= -1;
        }
      } else if (op == OP_MUL) {
        terms[term].num *= operands[i].num;
        terms[term].den *= operands[i].den;
      } else {
        /* + or -: the right operand starts a new term. */
        term++;
        terms[term] = operands[i];
      }
    }

    /* Pass two: add or subtract the terms into terms[0]. */
    term = 0;
    for (ops = mask, i = 0; i < num_ops; i++, ops /= 4) {
      int op = ops & 3;

      if (op == OP_ADD || op == OP_SUB) {
        long sign = (op == OP_SUB) ? -1 : +1;

        term++;
        terms[0].num = terms[0].num * terms[term].den +
                       sign * terms[term].num * terms[0].den;
        terms[0].den = terms[0].den * terms[term].den;
      }
    }
    reduce(&terms[0]);

    /* Print the expression if it hits the goal (or the goal is 0/0). */
    if (args[ARG_GOAL].den == 0 || (args[ARG_GOAL].num == terms[0].num &&
                                    args[ARG_GOAL].den == terms[0].den)) {
      for (ops = mask, i = 0; i < num_ops; i++, ops /= 4) {
        print_fraction(operands[i]);
        last_result = printf(" %c ", operator_symbols[ops & 3]);
      }
      print_fraction(operands[i]);
      last_result = printf(" = ");
      print_fraction(terms[0]);
      last_result = printf("%c", '\n');
    }
  }
  return last_result - 1;
}
