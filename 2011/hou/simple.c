#include <stdio.h>
#include <math.h>

/*
 * Scientific calculator: ./simple 'expression'
 *
 * Grammar actually parsed (functionally the same as a normal arithmetic
 * parser for valid input):
 *
 *   expression := factor { ('*' | '/') factor
 *                        | ('+' | '-') expression }
 *   factor     := number
 *               | '(' expression ')'
 *               | <any other character> factor     (a unary operator)
 *
 * Subtraction and division are not binary operators here: '-' is a unary
 * "negate" and '/' is a unary "reciprocal", so "a-b" is a + (-b) and
 * "a/b" is a * (1/b).  Function names are also unary operators: every
 * letter of "sin" is applied in turn to the operand, but only one letter
 * of each name ('i' for sin, 'a' for tan, ...) actually does something,
 * the others are no-ops.
 */

/* Example from the source: 11/26+222/31 -> 7.584367 (constants below) */
#define HASH_MULTIPLY 11
#define HASH_DIVIDE 26
#define HASH_OFFSET (222 / 31) /* = 7 */

/* Bit set of button slots (0..15) that hold an operator: 3*367 = 1101 */
#define OPERATOR_SLOTS (3 * 367)

/* Button slots 11..15 hold the digits (and '.' is handled by sscanf) */
#define FIRST_NUMBER_SLOT 11

enum operation {
  OP_NONE,       /* no-op character */
  OP_NEGATE,     /* '-'  (-x)  */
  OP_TAN,        /* 'a' of tan */
  OP_RECIPROCAL, /* '/'  (1/x) */
  OP_COS,        /* 'c' of cos */
  OP_EXP,        /* 'p' of exp */
  OP_SQRT,       /* 'q' of sqrt */
  OP_SIN,        /* 'i' of sin */
  OP_LOG         /* 'g' of log */
};

double result;
char *cursor;

void parse_expression(void);

/* First hash: maps a character to one of 16 button slots */
int button_slot(char ch) {
  return (ch * HASH_MULTIPLY / HASH_DIVIDE + HASH_OFFSET) & 15;
}

/* Second hash: maps a character to an operation, masked by the slot table */
int operation_of(char ch) {
  int ordinal = (ch % 21 + 7) % 9;
  int is_operator = OPERATOR_SLOTS >> button_slot(ch) & 1;
  return ordinal * is_operator;
}

void apply(int operation) {
  switch (operation) {
  case OP_NEGATE:
    result = -result;
    break;
  case OP_TAN:
    result = tan(result);
    break;
  case OP_RECIPROCAL:
    result = 1 / result;
    break;
  case OP_COS:
    result = cos(result);
    break;
  case OP_EXP:
    result = exp(result);
    break;
  case OP_SQRT:
    result = sqrt(result);
    break;
  case OP_SIN:
    result = sin(result);
    break;
  case OP_LOG:
    result = log(result);
    break;
  }
}

void parse_factor(void) {
  char ch = *cursor;
  int operation = operation_of(ch);

  if (button_slot(ch) >= FIRST_NUMBER_SLOT) {
    /* read a number and the character following it, then skip ahead to
     * that character */
    char next = 0;
    sscanf(cursor, "%lf%c", &result, &next);
    while (*++cursor != next)
      ;
  } else {
    cursor++;
    if (ch == '(') {
      parse_expression();
      cursor++; /* skip ')' */
    } else {
      parse_factor(); /* operand of a unary operator */
    }
  }
  apply(operation);
}

void parse_expression(void) {
  parse_factor();
  for (;;) {
    double left = result;
    if (*cursor == '/' || *cursor == '*') {
      /* '/' stays in place and becomes the unary reciprocal of the factor */
      parse_factor();
      result = result * left;
    } else if (*cursor == '+' || *cursor == '-') {
      /* '-' stays in place and becomes the unary negation; the rest of the
       * expression is summed recursively */
      parse_expression();
      result = result + left;
    } else {
      break;
    }
  }
}

/*
 * The original reuses main() as the parser with argc selecting the level:
 * 2 = top-level expression (prints), 3 = expression, 4 = factor.
 */
int main(int argc, char **argv) {
  if (argc < 2)
    return 0;
  cursor = argv[1];
  if (argc >= 4)
    parse_factor();
  else
    parse_expression();
  if (argc == 2)
    printf("%f\n", result);
  return argc >= 4;
}
