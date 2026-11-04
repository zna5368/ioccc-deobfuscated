/*
 * Rational number calculator.
 *
 * The expression is parsed by recursive descent with precedence levels.
 * Every pending value lives in a "frame" of three ints on a small stack:
 * the numerator, the denominator and the precedence level of the operator
 * whose right-hand operand the frame is collecting.  The operand of an
 * operator is evaluated in the frame directly after the current one.
 */

#define NUM 0
#define DEN 1
#define LEVEL 2
#define FRAME_SIZE 3

/* precedence levels: an operand stops at an operator of the same or lower level */
#define TOP_LEVEL 0
#define PAREN_LEVEL 1
#define ADD_LEVEL 3 /* binary + and - */
#define MUL_LEVEL 4
#define DIV_LEVEL 5
#define NEGATE_LEVEL 6 /* unary - */

static int stack[99];
static char *input;

static void evaluate(int *frame);

/*
 * Skip the operator and evaluate its right-hand operand in the next frame.
 * The operand's denominator is not cleared: it is only set by a digit.
 */
static int *operand_of(int *frame, int level)
{
  int *operand = frame + FRAME_SIZE;

  input++;
  operand[NUM] = 0;
  operand[LEVEL] = level;
  evaluate(operand);
  return operand;
}

/*
 * Handle the character at the input position.  Returns 0 when the value
 * of this frame is finished (operator of lower precedence, ')' or a zero
 * denominator), otherwise 1.
 */
static int step(int *frame)
{
  int *operand;
  int level;

  switch (*input) {
  case '+':
    if (frame[LEVEL] >= ADD_LEVEL)
      return 0;
    operand = operand_of(frame, ADD_LEVEL);
    frame[NUM] = frame[NUM] * operand[DEN] + frame[DEN] * operand[NUM];
    frame[DEN] = frame[DEN] * operand[DEN];
    break;

  case '-':
    /* a minus seen while the value is still zero is a unary minus */
    if (frame[NUM] == 0) {
      frame[DEN] = 1;
      level = NEGATE_LEVEL;
    } else {
      level = ADD_LEVEL;
    }
    if (frame[LEVEL] >= level)
      return 0;
    operand = operand_of(frame, level);
    frame[NUM] = frame[NUM] * operand[DEN] - frame[DEN] * operand[NUM];
    frame[DEN] = frame[DEN] * operand[DEN];
    break;

  case '*':
    if (frame[LEVEL] >= MUL_LEVEL)
      return 0;
    operand = operand_of(frame, MUL_LEVEL);
    frame[NUM] = frame[NUM] * operand[NUM];
    frame[DEN] = frame[DEN] * operand[DEN];
    break;

  case '/':
    if (frame[LEVEL] >= DIV_LEVEL)
      return 0;
    operand = operand_of(frame, DIV_LEVEL);
    frame[NUM] = frame[NUM] * operand[DEN];
    frame[DEN] = frame[DEN] * operand[NUM];
    break;

  case '(':
    operand = operand_of(frame, PAREN_LEVEL);
    frame[NUM] = operand[NUM];
    frame[DEN] = operand[DEN];
    break;

  case ')':
    /* only the frame that the '(' opened consumes the ')' */
    if (frame[LEVEL] <= PAREN_LEVEL)
      input++;
    return 0;

  default:
    /* any other character (even a space) is treated as a decimal digit */
    frame[NUM] = 10 * frame[NUM] + *input - '0';
    input++;
    frame[DEN] = 1;
    break;
  }
  return frame[DEN] != 0;
}

/*
 * Reduce the fraction in the frame to lowest terms and move the sign to the
 * numerator.  Euclid's algorithm keeps its sequence of remainders in the
 * unused stack space after the frame.
 */
static void reduce(int *frame)
{
  int *euclid = frame + FRAME_SIZE;
  int negative = 0;
  int gcd;

  if (frame[NUM] < 0) {
    frame[NUM] = -frame[NUM];
    negative = !negative;
  }
  if (frame[DEN] < 0) {
    frame[DEN] = -frame[DEN];
    negative = !negative;
  }

  /* start with the larger value followed by the smaller one */
  if (frame[NUM] < frame[DEN]) {
    euclid[0] = frame[DEN];
    euclid[1] = frame[NUM];
  } else {
    euclid[0] = frame[NUM];
    euclid[1] = frame[DEN];
  }
  if (negative)
    frame[NUM] = -frame[NUM];

  if (euclid[1] == 0)
    euclid[1] = 1;
  while ((euclid[2] = euclid[0] % euclid[1]) != 0)
    euclid++;
  gcd = euclid[1];

  frame[NUM] /= gcd;
  frame[DEN] /= gcd;
}

/* Evaluate the expression at the input position into the frame. */
static void evaluate(int *frame)
{
  while (*input) {
    if (!step(frame))
      return;
    reduce(frame);
  }
  if (frame[LEVEL] == TOP_LEVEL)
    printf("%d/%d"
           "\n",
           frame[NUM], frame[DEN]);
}

int main(int argc, char **argv)
{
  input = argv[1];
  /* the first frame starts at argc, so extra arguments use up stack space */
  evaluate(&stack[argc]);
  return 1;
}
