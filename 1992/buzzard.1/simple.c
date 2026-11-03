/*
 * The program is written for an imaginary machine that only knows
 * "a = b", "a += b", "a -= b", "a *= b" and "a /= b" (Babbage's Analytical
 * Engine). It has no if, goto or comparison. Instead the whole program is
 * one straight list of assignments that is run again and again until
 * `halted` is set:
 *
 *   - `pc` holds the number of the label we want to run next.
 *   - `executing` is 1 while we are inside the code that should run and 0
 *     while we are only passing over code that should be skipped.
 *   - every store is written as  dest += (value - dest) * executing,
 *     so it changes nothing when executing is 0.
 *
 * Comparisons are made with only + - * / by squeezing a number down to
 * 0 or 1 with two integer divisions (see positive()). They are only correct
 * for numbers smaller than about 16 million.
 */

/* the "registers" of the machine */
int pc = 0;        /* label to run next */
int executing;     /* 1: run the statements, 0: skip them */
int halted = 0;    /* set to 1 to leave the main loop */
int scratch;       /* shared work register used by some instructions */

/* the program's own variables */
int test;          /* result of comparisons, also scratch arithmetic */
int num;           /* second argument, the result being calculated */
int acc;           /* first argument, a counter, then the printed answer */
int aux;           /* swap space / flag / BCD digit place value */

/* label numbers, in the order they appear in the program */
enum {
  START,          /* 0 */
  NOT_FACTORIAL,  /* 1 */
  GCD_LOOP,       /* 2 */
  GCD_SUBTRACT,   /* 3 */
  FACTORIAL,      /* 4 */
  FACTORIAL_LOOP, /* 5 */
  FIND_FACTOR,    /* 6 */
  FACTOR_LOOP,    /* 7 */
  TO_BCD,         /* 8 */
  BCD_LOOP,       /* 9 */
  PRIME           /* 10 */
};

/* 1 if value > 0, else 0 (only for |value| below about 16 million) */
int positive(int value)
{
  value += 2047;
  value /= 2048;
  value += 8191;
  value /= 8192;
  return value;
}

/* dest = value, but only while executing */
void store(int *dest, int value)
{
  value -= *dest;
  value *= executing;
  *dest += value;
}

/* label: start executing here if pc points at this label */
void label(int number)
{
  int not_here;

  scratch = pc;
  scratch -= number;
  not_here = positive(scratch * scratch);
  executing += 2;
  executing -= not_here;
  executing /= 2;
}

/* goto target */
void jump(int target)
{
  store(&pc, target);
  executing = 0;
}

/* if condition (0 or 1) is 0, goto target */
void jump_if_zero(int condition, int target)
{
  int new_pc = pc;

  new_pc -= target;
  new_pc *= condition;
  new_pc += target;
  store(&pc, new_pc);
  executing *= condition;
}

/* if condition (0 or 1) is 1, goto target */
void jump_if_one(int condition, int target)
{
  int step = target;

  step -= pc;
  step *= executing;
  step *= condition;
  pc += step;
  executing *= 1 - condition;
}

/* stop the main loop */
void halt(void)
{
  store(&halted, 1);
  store(&pc, 0);
  executing = 0;
}

void add(int *dest, int left, int right) { store(dest, right + left); }
void subtract(int *dest, int left, int right) { store(dest, left - right); }
void multiply(int *dest, int left, int right) { store(dest, left * right); }
void divide(int *dest, int left, int right) { store(dest, left / right); }

/*
 * Division by a variable. The division is calculated even when the code
 * is skipped, so the divisor is replaced by 1 when not executing to avoid
 * dividing by zero.
 */
void safe_divide(int *dest, int left, int right)
{
  int divisor;

  scratch = right;
  divisor = right;
  divisor -= 1;
  divisor *= executing;
  divisor += 1;
  store(dest, left / divisor);
}

/* dest = 1 if value > 0, else 0 */
void set_positive(int *dest, int value) { store(dest, positive(value)); }

/* dest = 1 if value != 0, else 0 */
void set_nonzero(int *dest, int value)
{
  int either;

  store(&scratch, positive(value));
  either = positive(-value);
  either += scratch;
  either += 1;
  either /= 2;
  store(dest, either);
}

main(argc, argv) char **argv;
{
  if (argc < 3)
    exit(1);

  while (!halted) {
    label(START);
    store(&acc, atoi(argv[1]));
    store(&num, atoi(argv[2]));
    set_nonzero(&test, acc);
    jump_if_zero(test, FACTORIAL); /* first argument 0: factorial */
    set_positive(&test, acc);
    jump_if_one(test, NOT_FACTORIAL);
    store(&acc, 0xbad); /* negative first argument */
    halt();

    label(NOT_FACTORIAL);
    subtract(&test, acc, 1);
    set_nonzero(&test, test);
    jump_if_zero(test, FIND_FACTOR); /* first argument 1: find a factor */

    /* greatest common divisor by repeated subtraction */
    label(GCD_LOOP);
    subtract(&test, acc, num);
    set_nonzero(&aux, test);
    set_positive(&test, test);
    jump_if_zero(aux, TO_BCD); /* acc == num: answer is num */
    jump_if_one(test, GCD_SUBTRACT);
    store(&aux, num); /* acc < num: swap them */
    store(&num, acc);
    store(&acc, aux);
    label(GCD_SUBTRACT);
    subtract(&acc, acc, num);
    set_nonzero(&test, acc);
    jump_if_one(test, GCD_LOOP);
    jump(TO_BCD);

    /* num = acc! */
    label(FACTORIAL);
    store(&acc, num);
    store(&num, 1);
    label(FACTORIAL_LOOP);
    set_positive(&test, acc);
    jump_if_zero(test, TO_BCD);
    multiply(&num, num, acc);
    subtract(&acc, acc, 1);
    jump(FACTORIAL_LOOP);

    /* count num down until it divides acc */
    label(FIND_FACTOR);
    store(&acc, num);
    label(FACTOR_LOOP);
    subtract(&num, num, 1);
    subtract(&test, num, 1);
    set_nonzero(&test, test);
    jump_if_zero(test, PRIME); /* reached 1: no factor */
    safe_divide(&test, acc, num);
    multiply(&test, test, num);
    subtract(&test, acc, test); /* test = acc % num */
    set_positive(&test, test);
    jump_if_one(test, FACTOR_LOOP);

    /*
     * Rewrite num so that its decimal digits become hex digits
     * (binary coded decimal), so printing in hex shows it in decimal.
     */
    label(TO_BCD);
    store(&acc, 0);
    store(&aux, 1);
    label(BCD_LOOP);
    divide(&test, num, 10);
    multiply(&test, test, 10);
    subtract(&test, num, test); /* lowest decimal digit */
    multiply(&test, test, aux);
    add(&acc, acc, test);
    divide(&num, num, 10);
    multiply(&aux, aux, 16);
    set_nonzero(&test, num);
    jump_if_one(test, BCD_LOOP);
    halt();

    label(PRIME);
    store(&acc, 0xace);
    halt();
  }

  printf("%x\n", acc);
}
