#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Mile: turn stdin into a C program made almost entirely of preprocessor
 * directives.  The generated file includes itself recursively and runs a
 * 16-bit LFSR (seeded with -DPIN=...) inside the preprocessor to rebuild the
 * input bytes at compile time.
 *
 * Every macro in the generated code is a single random letter.  The macro
 * slots are:
 *   0..16  step counter bits (how many LFSR steps until the next byte)
 *   17     "first byte" flag
 *   18..33 LFSR state bits (initialised from the PIN)
 *   34     LFSR feedback bit
 *   35     "emit a byte" mode flag
 */
#define COUNTER        0
#define COUNTER_BITS   17
#define FIRST_BYTE     17
#define STATE          18
#define STATE_BITS     16
#define FEEDBACK       34
#define EMIT_MODE      35
#define NUM_MACROS     36

static char macro_name[NUM_MACROS];
static int byte_bit_source[STATE_BITS];   /* which state bit becomes output bit i */
static int lfsr;

/* Fill arr with 0..n-1 and shuffle it (elements 0 and 1 are never swapped). */
static void shuffle(int n, int *arr)
{
    int i, j, tmp;

    for (i = 0; i < n; i++)
        arr[i] = i;
    for (i = n - 1; i > 1; i--) {
        j = rand() % (i + 1);
        tmp = arr[j];
        arr[j] = arr[i];
        arr[i] = tmp;
    }
}

/* Advance the 16-bit Fibonacci LFSR with taps at bits 0, 2, 3 and 5. */
static void lfsr_step(void)
{
    int feedback = (lfsr ^ lfsr >> 2 ^ lfsr >> 3 ^ lfsr >> 5) & 1;

    lfsr = (lfsr >> 1 | feedback << 15) & 0xffff;
}

/* Build a byte by picking 8 of the LFSR state bits. */
static int lfsr_byte(void)
{
    int bit, byte = 0;

    for (bit = 0; bit < 8; bit++)
        if (lfsr & (1 << byte_bit_source[bit]))
            byte |= 1 << bit;
    return byte;
}

static void directive(const char *keyword, int macro)
{
    printf("#%s %c\n", keyword, macro_name[macro]);
}

/*
 * Emit nested #ifdef blocks that define FEEDBACK when an odd number of the
 * tap macros are defined.  "parity" is the parity of the taps seen so far.
 */
static void emit_parity(const int *taps, int count, int parity)
{
    if (count == 1) {
        directive(parity ? "ifndef" : "ifdef", taps[0]);
        directive("define", FEEDBACK);
        puts("#endif");
        return;
    }
    directive("ifdef", taps[0]);
    emit_parity(taps + 1, count - 1, !parity);
    puts("#else");
    emit_parity(taps + 1, count - 1, parity);
    puts("#endif");
}

int main(int argc, char **argv)
{
    static const int taps[] = { STATE + 2, STATE + 3, STATE + 5, STATE + 0 };
    int order[NUM_MACROS];
    int pin = 0, c, i, bit, steps, min_steps;
    int byte_count, top_bit;

    if (argc != 2 || (pin = atoi(argv[1])) < 1 || pin > 65536) {
        puts("usage: [PIN] < input.txt > output.c\n\n"
             "where [PIN] is an integer in the range of 1..65535");
        return (pin & 1) ? 255 : 0;
    }
    lfsr = pin;

    srand(time(NULL));
    shuffle(STATE_BITS, byte_bit_source);
    shuffle(NUM_MACROS, order);
    for (i = 0; i < NUM_MACROS; i++)
        macro_name[i] = order[i] < 26 ? 'a' + order[i] : 'A' + order[i] - 26;

    /* header: require PIN, then load the PIN bits into the LFSR state */
    puts("#ifndef PIN\n#error Please compile with -DPIN=<number>");
    puts("#else");
    puts("#ifndef _");
    puts("#define _ __FILE__");
    for (bit = 0; bit < STATE_BITS; bit++) {
        printf("#if PIN %% %d > %d\n", 2 << bit, (1 << bit) - 1);
        directive("define", STATE + bit);
        puts("#endif");
    }
    puts("static const unsigned char O[] = {");

    /* for every input byte, record how many LFSR steps reach it */
    top_bit = 8;
    for (byte_count = 0; (c = getchar()) != EOF; byte_count++) {
        min_steps = 1 + rand() % 16;
        for (steps = 0; steps < min_steps || c != lfsr_byte(); steps++)
            lfsr_step();
        if (byte_count == 0)
            steps |= 1 << FIRST_BYTE;

        shuffle(COUNTER_BITS + 1, order);
        for (i = 0; i < COUNTER_BITS + 1; i++) {
            bit = order[i];
            if (steps & (1 << bit)) {
                directive("define", COUNTER + bit);
                if (bit < COUNTER_BITS && bit > top_bit)
                    top_bit = bit;
            }
        }
        puts("#include _");
    }

    /* end of the array and the run-time program */
    puts("};");
    printf("#include<stdio.h>\n"
           "int main() { return !fwrite(O, %d, %d, stdout); }\n", byte_count, 1);

    /* recursive part: reached when the file includes itself */
    puts("#else");
    directive("ifdef", FIRST_BYTE);
    directive("undef", FIRST_BYTE);
    directive("define", EMIT_MODE);
    puts("#include _");
    for (bit = 0; bit < 8; bit++) {
        directive("ifdef", STATE + byte_bit_source[bit]);
        directive("define", COUNTER + bit);
        puts("#endif");
    }
    directive("undef", EMIT_MODE);
    puts("#include _");
    puts("0,");
    directive("define", FIRST_BYTE);
    puts("#else");

    /* decrement the step counter, starting at its highest used bit */
    for (bit = top_bit; bit > 0; bit--) {
        directive("ifdef", COUNTER + bit);
        directive("undef", COUNTER + bit);
        puts("#include _");
        directive("define", COUNTER + bit - 1);
        puts("#include _");
        directive("define", COUNTER + bit - 1);
        puts("#endif");
    }
    directive("ifdef", COUNTER);
    directive("undef", COUNTER);

    /* one LFSR step: compute the feedback bit, then shift the state */
    directive("ifdef", EMIT_MODE);
    directive("undef", FEEDBACK);
    emit_parity(taps, 4, 0);
    directive("undef", STATE);
    for (bit = 1; bit <= STATE_BITS; bit++) {
        directive("ifdef", STATE + bit);
        directive("define", STATE + bit - 1);
        puts("#endif");
        if (bit < STATE_BITS)
            directive("undef", STATE + bit);
    }
    puts("#else");
    puts("-~");
    puts("#endif");
    puts("#endif");
    puts("#endif");
    puts("#endif");
    puts("#endif");

    return 0;
}
