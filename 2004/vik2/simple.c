#include <stdio.h>

/*
 * An 11-bit ALU in the style of the preprocessor ALU of the original entry.
 * In the original every bit is a symbol that is either #defined (1) or
 * #undef'ined (0); here every register is an unsigned integer instead.
 */
#define BITS 11
#define MASK ((1u << BITS) - 1)

/*
 * The original stops as soon as the candidate has the bit for 32 set
 * (symbol _5) or the bit for 1024 (symbol _r), so it lists primes below 32.
 */
#define LIMIT_BIT 32
#define OVERFLOW_BIT 1024

static unsigned X, Y;     /* working registers */
static int CARRY, EQ, LT; /* bits of the status register SR */

/* one bit full adder: the core of the ALU */
static int add_bit(int a, int b, int *carry)
{
    int sum = a ^ b ^ *carry;
    *carry = (a & b) | (a & *carry) | (b & *carry);
    return sum;
}

/* CLR_X */
static void clr_x(void) { X = 0; }

/* CLR_Y */
static void clr_y(void) { Y = 0; }

/* ADD: X = X + Y, CARRY is set when the result does not fit in 11 bits */
static void add(void)
{
    unsigned result = 0;
    int bit;

    CARRY = 0;
    for (bit = 0; bit < BITS; bit++)
        result |= (unsigned)add_bit(X >> bit & 1, Y >> bit & 1, &CARRY) << bit;
    X = result;
}

/* INC2: X = X + 2 */
static void inc2(void)
{
    clr_y();
    Y = 2;
    add();
}

/* SUB: X = X - Y done as X + ~Y + 1, CARRY is set when there is no borrow */
static void sub(void)
{
    int first_carry;

    Y = ~Y & MASK;
    add();
    first_carry = CARRY;
    clr_y();
    Y = 1;
    add();
    if (first_carry)
        CARRY = !CARRY;
}

/* TST: compare X with Y (X is destroyed) */
static void tst(void)
{
    sub();
    EQ = (X == 0);
    LT = !CARRY;
}

/* add 2 to a memory variable by passing it through the ALU */
static unsigned plus_two(unsigned value)
{
    clr_x();
    X = value;
    inc2();
    return X;
}

/*
 * The sum of the first k odd numbers is k*k. Adding odd numbers to and
 * subtracting odd numbers from a running sum finds a*a - b*b == candidate,
 * which is (a - b) * (a + b). The first match found has the two factors
 * closest together, so the candidate is prime only when a - b == 1, which
 * means the last odd numbers used differ by exactly 2.
 */
int main()
{
    unsigned candidate = 3;      /* odd number being tested */
    unsigned added_odd = MASK;   /* last odd number added (2a - 1) */
    unsigned removed_odd = MASK; /* last odd number subtracted (2b - 1) */
    unsigned sum = 0;            /* a*a - b*b */

    printf("%d\n", 1 + 1);

    while (!(candidate & (LIMIT_BIT | OVERFLOW_BIT))) {
        clr_x();
        X = sum;
        clr_y();
        Y = candidate;
        tst();

        if (EQ) {
            /* is the subtracted odd number + 2 the added odd number? */
            clr_x();
            X = plus_two(removed_odd);
            clr_y();
            Y = added_odd;
            tst();
            if (EQ)
                printf("%d\n", candidate);

            candidate = plus_two(candidate);
            clr_x();
            sum = 0;
            /* MASK is -1 in 11 bits so the first +2 gives 1 */
            added_odd = MASK;
            removed_odd = MASK;
        } else if (LT) {
            /* sum too small: add the next odd number */
            added_odd = plus_two(added_odd);
            clr_x();
            X = sum;
            clr_y();
            Y = added_odd;
            add();
            sum = X;
        } else {
            /* sum too big: subtract the next odd number */
            removed_odd = plus_two(removed_odd);
            clr_x();
            X = sum;
            clr_y();
            Y = removed_odd;
            sub();
            sum = X;
        }
    }
    return 0;
}
