/*
 * Universal machine for Binary Lambda Calculus (BLC).
 *
 * The program at the start of the input is parsed into the term space, and
 * then a Krivine machine applies it to the rest of the input.  The result is
 * a lazy list that the machine prints as it is evaluated.
 *
 * With no arguments the machine works on bytes (BLC8).  With one argument it
 * works on bits, reading the low bit of each input character and printing
 * the characters 0 and 1.
 *
 * Each term in the term space is an opcode followed by its operand, if any:
 *
 *   OP_IO               I/O hook, described at boot_program
 *   OP_VAR index        variable, as a de Bruijn index counted from 0
 *   OP_APP size f x     application of f, which is size entries long, to x
 *   OP_LAM body         lambda abstraction
 *
 * Lists use the usual lambda encoding.  The list with head h and tail t is
 * \f. f h t and the empty list is \a \b. b.  A bit is a boolean: the bit 0 is
 * true (\a \b. a) and the bit 1 is false (\a \b. b).  A byte is a list of
 * eight bits with the most significant bit first.
 */

#define TERM_SPACE_SIZE 500000

#define STDIN 0
#define STDOUT 1

#define EXIT_OUT_OF_TERM_SPACE 5
#define EXIT_OUT_OF_MEMORY 6

enum opcode { OP_IO, OP_VAR, OP_APP, OP_LAM };

/*
 * The term that runs first.  In the comments T7 means the term that starts at
 * index 7, and P is the program parsed from the input.
 *
 *   T0  = T2 T22                        print the result of running P
 *   T2  = \list. list T14               print a list of bits (bit mode)
 *         \list. list T7                print a list of bytes (byte mode)
 *   T7  = \byte. byte T14 T13           print the bits of one byte
 *   T9  = \rest. rest T14               continue with the remaining bits
 *   T14 = \bit. bit T20 T21             pick the hook for this bit
 *   T22 = P input                       apply P to the input list
 *
 * Printing is continuation passing.  Applying a list to a function passes it
 * the head and the tail, and applying an empty list to a function skips it.
 *
 * The terms overlap to save space.  The size at index 9 is also the OP_LAM
 * of T9, and the variable index at 13 is also the hook T13.  In the same way
 * the variable index at 20 is also the hook T20.
 *
 * The first four entries double as the closure at the end of every
 * environment, so that no environment needs a NULL check.  The value 20 at
 * index 1 is its reference count and is large enough that it is never freed.
 */
#define BOOT_SIZE 24
#define TERM_PRINT_LIST 2     /* T2 */
#define PRINT_SIZE_INDEX 4    /* size of the function in T2's application */
#define TERM_NEXT_BIT 9       /* T9 */
#define TERM_END_OF_BYTE 13   /* I/O hook reached when a byte's list ends */
#define TERM_OUTPUT_ZERO 20   /* I/O hook for the bit 0 */
#define TERM_OUTPUT_ONE 21    /* I/O hook for the bit 1 */
#define TERM_RUN_PROGRAM 22   /* T22 */
#define PROGRAM_SIZE_INDEX 23 /* size of the function in T22's application */

static const long boot_program[BOOT_SIZE] = {
    OP_APP, 20,             /*  0: T2 T22 */
    OP_LAM,                 /*  2: \list. */
    OP_APP, 9,              /*  3: list T14, reduced to list T7 for bytes */
    OP_VAR, 0,              /*  5: list */
    OP_LAM,                 /*  7: \byte. */
    OP_APP, OP_LAM,         /*  8: (byte T14) T13, and 9 starts \rest. */
    OP_APP, 2,              /* 10: byte T14, or rest T14 */
    OP_VAR, OP_IO,          /* 12: byte, or rest, and 13 is the hook */
    OP_LAM,                 /* 14: \bit. */
    OP_APP, 4,              /* 15: (bit T20) T21 */
    OP_APP, 1,              /* 17: bit T20 */
    OP_VAR, OP_IO,          /* 19: bit, and 20 is the hook */
    OP_IO,                  /* 21: the hook */
    OP_APP, 0               /* 22: P input, the size is filled in by main */
};

/* \f. f head tail, where head follows this header and tail comes after it */
#define CONS_SIZE 7
#define CONS_TAIL_SIZE_FROM_END 5 /* position of the 8 counted from the end */
#define BYTE_TAIL_SIZE 96         /* distance to the tail of a byte's cell */

static const long cons_header[CONS_SIZE] = {
    OP_LAM,
    OP_APP, 8,
    OP_APP, 2,
    OP_VAR, 0
};

/* \a \b. followed by a variable index: 1 is true (a) and 0 is false (b) */
#define BOOLEAN_SIZE 3

static const long boolean_header[BOOLEAN_SIZE] = { OP_LAM, OP_LAM, OP_VAR };

/* An unevaluated term with its environment */
struct closure {
    struct closure *next; /* next in an environment, stack or free list */
    long reference_count;
    struct closure *environment;
    long term;
};

static long terms[TERM_SPACE_SIZE];
static long mode;      /* 7 for bytes and 0 for bits */
static long bits_left; /* bits unread in input_char, negative after EOF */
static struct closure *stack;
static struct closure *environment = (struct closure *)terms;
static long term_index;
static long *terms_end = terms;
static struct closure *free_closures;
static char input_char;
static char output_char;

/*
 * Take the next input bit and return whether it is 0.  This is what both of
 * its callers need: the BLC encoding starts a lambda with 0, and a bit 0
 * becomes the variable index 1.
 */
static int next_bit_is_zero(void)
{
    if (bits_left != 0) {
        bits_left--;
    } else {
        bits_left = mode | read(STDIN, &input_char, 1) - 1;
    }
    return ~input_char >> bits_left & 1;
}

static void append_terms(const long *source, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (terms_end - terms >= TERM_SPACE_SIZE) {
            exit(EXIT_OUT_OF_TERM_SPACE);
        }
        *terms_end++ = source[i];
    }
}

/* A variable is 1^n 0, and the first 1 has already been read */
static int parse_variable(long *term)
{
    if (next_bit_is_zero()) {
        terms_end += 2;
    } else {
        terms_end[1]++;
        parse_variable(term);
    }
    return terms_end - term;
}

/* Read one BLC encoded term into the term space and return its size */
static int parse_term(void)
{
    long *term = terms_end;

    if (!next_bit_is_zero()) {
        *term = OP_VAR;
        return parse_variable(term);
    }
    if (next_bit_is_zero()) {
        *term = OP_LAM; /* 00 body */
        terms_end++;
    } else {
        *term = OP_APP; /* 01 function argument */
        terms_end += 2;
        term[1] = parse_term();
    }
    parse_term();
    return terms_end - term;
}

/*
 * Add the list of bits still unread in input_char.  Every bit but the last
 * one is followed by another call with more_bits set, and the call that finds
 * no bits left ends the list.
 */
static void expand_bits(int more_bits)
{
    if (bits_left != 0) {
        append_terms(cons_header, CONS_SIZE);
    }
    append_terms(boolean_header, BOOLEAN_SIZE);
    *terms_end++ = bits_left != 0 && next_bit_is_zero();
    if (more_bits) {
        expand_bits(bits_left);
    }
}

/*
 * The input list is built only when the program looks at it.  The OP_IO that
 * follows the last parsed term stands for the unread input, and it is
 * replaced by a list cell for one bit or byte, whose tail is the next OP_IO.
 * After EOF it is replaced by the empty list.
 */
static void expand_input(void)
{
    int have_byte;

    next_bit_is_zero(); /* fetch a character to find out whether EOF is here */
    have_byte = bits_left > 0;
    bits_left++; /* put the bit back */
    if (have_byte) {
        append_terms(cons_header, CONS_SIZE);
        terms_end[-CONS_TAIL_SIZE_FROM_END] = BYTE_TAIL_SIZE;
    }
    expand_bits(have_byte);
}

/* Print or collect a bit and return the term to continue with */
static long output_hook(void)
{
    int byte_ended = term_index == TERM_END_OF_BYTE;
    int bit = term_index != TERM_OUTPUT_ZERO;

    if (byte_ended != (mode & 1)) {
        output_char = output_char * 2 | bit;
        return TERM_NEXT_BIT;
    }
    if (mode == 0) {
        output_char = '0' + bit;
    }
    /* write returns 1 for the one character, giving TERM_PRINT_LIST */
    return write(STDOUT, &output_char, 1) + 1;
}

/* Drop a reference and recycle the closure when nothing else needs it */
static void release(struct closure *closure)
{
    if (--closure->reference_count == 0) {
        release(closure->next);
        closure->next = free_closures;
        free_closures = closure;
        release(closure->environment);
    }
}

/* Push the argument of an application on the stack */
static void apply(long function_size)
{
    struct closure *argument;

    argument = free_closures ? free_closures : calloc(1, sizeof(struct closure));
    if (!argument) {
        exit(EXIT_OUT_OF_MEMORY);
    }
    free_closures = argument->next;
    argument->next = stack;
    stack = argument;
    argument->environment = environment;
    argument->reference_count = 1;
    environment->reference_count++;
    term_index++;
    argument->term = term_index + function_size;
}

/* Pop the stack into the environment, or finish when nothing is left to do */
static void lambda(void)
{
    struct closure *argument = stack;

    if (!argument) {
        /*
         * The empty list ends here, on the second lambda of \a \b. b, so the
         * next entry is the variable index 0 and the exit status is 0.
         */
        exit(terms[term_index + 1]);
    }
    stack = argument->next;
    argument->next = environment;
    environment = argument;
}

/* Continue with the closure that a variable refers to */
static void variable(long index)
{
    struct closure *old_environment = environment;

    while (index--) {
        environment = environment->next;
    }
    term_index = environment->term;
    environment = environment->environment;
    environment->reference_count++;
    release(old_environment);
}

int main(int argc, char **argv)
{
    int opcode;
    long operand;

    append_terms(boot_program, BOOT_SIZE);
    mode = (argc - 2) & 7;
    terms[PRINT_SIZE_INDEX] -= mode;
    terms[PROGRAM_SIZE_INDEX] = parse_term();
    bits_left = 0;

    for (;;) {
        opcode = terms[term_index++];
        operand = terms[term_index];

        switch (opcode) {
        case OP_APP:
            apply(operand);
            break;
        case OP_IO:
            term_index--;
            if (term_index < TERM_RUN_PROGRAM) {
                term_index = output_hook();
            } else {
                expand_input();
            }
            break;
        case OP_LAM:
            lambda();
            break;
        default:
            variable(operand);
            break;
        }
    }
}
