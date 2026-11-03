#include <stdio.h>

/*
 * picoAPL: evaluates an APL-style expression of binary numbers read from
 * standard input.  Operators are dyadic +, -, x and monadic -.  Expressions
 * group from right to left and parentheses may be used for grouping.
 *
 * Numbers have arbitrary precision.  Each number is a linked list of bits in
 * two's complement, least significant bit first.  The list ends with a
 * terminator node that stands for an infinite run of identical bits:
 *   END_ZEROS  ...0000  (the number 0, or the top of a positive number)
 *   END_ONES   ...1111  (the number -1, or the top of a negative number)
 */
enum tag { END_ZEROS, END_ONES, BIT0, BIT1 };

typedef struct number {
  enum tag tag;
  struct number *next; /* higher bits (unused for terminators) */
} *Number;

typedef Number (*Handler)(Number);

/* one handler per input character; unknown characters have none */
Handler handlers[256];

Number add(Number a, Number b);
Number parse_expression(void);
Number read_next(Number acc);

Number make_node(enum tag tag, Number next) {
  Number n = (Number)malloc(sizeof(*n));
  n->tag = tag;
  n->next = next;
  return n;
}

Number zero(void) { return make_node(END_ZEROS, NULL); }
Number minus_one(void) { return make_node(END_ONES, NULL); }

/* the terminator node, which tells the sign of the number */
Number sign_of(Number n) {
  while (n->tag == BIT0 || n->tag == BIT1)
    n = n->next;
  return n;
}

/* 2 * n */
Number shift_in_0(Number n) {
  if (n->tag == END_ZEROS)
    return n;
  return make_node(BIT0, n);
}

/* 2 * n + 1 */
Number shift_in_1(Number n) {
  if (n->tag == END_ONES)
    return n;
  return make_node(BIT1, n);
}

/* bitwise not: -n - 1 */
Number complement(Number n) {
  switch (n->tag) {
  case END_ZEROS:
    return make_node(END_ONES, n);
  case END_ONES:
    return make_node(END_ZEROS, n);
  case BIT0:
    return shift_in_1(complement(n->next));
  default:
    return shift_in_0(complement(n->next));
  }
}

/* -n */
Number negate(Number n) {
  switch (n->tag) {
  case END_ZEROS:
    return n;
  case END_ONES:
    return shift_in_1(complement(n));
  case BIT0:
    return shift_in_0(negate(n->next));
  default:
    return shift_in_1(complement(n->next));
  }
}

/* n + 1 */
Number increment(Number n) {
  switch (n->tag) {
  case END_ZEROS:
    return shift_in_1(negate(n));
  case END_ONES:
    return complement(n);
  case BIT0:
    return shift_in_1(n->next);
  default:
    return shift_in_0(increment(n->next));
  }
}

/* n - 1 */
Number decrement(Number n) {
  switch (n->tag) {
  case END_ZEROS:
    return complement(n);
  case END_ONES:
    return shift_in_0(minus_one());
  case BIT0:
    return shift_in_1(decrement(n->next));
  default:
    return shift_in_0(n->next);
  }
}

/* print the bits of a number, most significant first */
void print_bits(Number n) {
  if (n->tag == BIT0 || n->tag == BIT1) {
    print_bits(n->next);
    printf(n->tag == BIT0 ? "0" : "1");
  }
}

/* a * b */
Number multiply(Number a, Number b) {
  switch (a->tag) {
  case END_ZEROS:
    return a;
  case END_ONES:
    return negate(b);
  case BIT0:
    return shift_in_0(multiply(a->next, b));
  default:
    return add(shift_in_0(multiply(a->next, b)), b);
  }
}

/* a + 2 * b */
Number add_doubled(Number a, Number b) {
  switch (a->tag) {
  case END_ZEROS:
    return shift_in_0(b);
  case END_ONES:
    return shift_in_1(decrement(b));
  case BIT0:
    return shift_in_0(add(a->next, b));
  default:
    return shift_in_1(add(a->next, b));
  }
}

/* a + b */
Number add(Number a, Number b) {
  switch (a->tag) {
  case END_ZEROS:
    return b;
  case END_ONES:
    return decrement(b);
  case BIT0:
    return add_doubled(b, a->next);
  default:
    return add_doubled(increment(b), a->next);
  }
}

/* print a non-negative number */
void print_positive(Number n) {
  if (n->tag == END_ZEROS)
    printf("0");
  else
    print_bits(n);
}

/* print a negative number */
void print_negative(Number n) {
  if (n->tag == END_ONES) {
    printf("-1");
  } else {
    printf("-");
    print_positive(negate(n));
  }
}

void print_number(Number n) {
  if (sign_of(n)->tag == END_ZEROS)
    print_positive(n);
  else
    print_negative(n);
}

/* read a character and let its handler continue with the accumulator */
Number read_next(Number acc) { return (*handlers[getchar()])(acc); }

/* parse an expression up to ')' or newline */
Number parse_expression(void) { return read_next(zero()); }

/* operators: everything to the right is evaluated first */
Number op_subtract(Number left) { return add(left, negate(parse_expression())); }
Number op_add(Number left) { return add(left, parse_expression()); }
Number op_multiply(Number left) { return multiply(left, parse_expression()); }

/* '(' starts a sub-expression whose value becomes the new accumulator */
Number open_paren(Number acc) { return read_next(parse_expression()); }

/* binary digits are shifted into the accumulator */
Number digit_0(Number acc) { return read_next(shift_in_0(acc)); }
Number digit_1(Number acc) { return read_next(shift_in_1(acc)); }

/* ')' and newline end an expression */
Number end_expression(Number acc) { return acc; }

main() {
  handlers['('] = open_paren;
  handlers['x'] = op_multiply;
  handlers['-'] = op_subtract;
  handlers['+'] = op_add;
  handlers['0'] = digit_0;
  handlers['1'] = digit_1;
  handlers[' '] = read_next;
  handlers[')'] = handlers['\n'] = end_expression;
  print_number(parse_expression());
  printf("\n");
}
