#include <stdio.h>

/*
 * This file works in two ways, chosen when the program starts:
 *
 *   Compiled by itself, it reads text from stdin and writes that text out as a
 *   double helix of DNA letters.
 *
 *   Included at the top of such a helix, the helix becomes the body of the
 *   function decode_helix(), and running it prints the original text.
 *
 * Each input byte is stored as four bases, two bits per base, high bits first.
 */

#define BASES_PER_BYTE    4
#define LETTERS_PER_BYTE  (2 * BASES_PER_BYTE)  /* each base has a partner */
#define FIRST_PLACE_VALUE 64                    /* 4 * 4 * 4 */
#define HELIX_PERIOD      16                    /* rows in one full twist */
#define HALF_PERIOD       (HELIX_PERIOD / 2)

/* Number of '~' bonds on each row of a half twist.  Zero marks a strand crossing. */
static const int bond_widths[HALF_PERIOD] = { 0, 2, 3, 4, 4, 3, 2, 0 };

/* Number of spaces before each row of a full twist. */
static const int row_indents[HELIX_PERIOD] = {
    5, 4, 4, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 4, 4, 5
};

/*
 * Only a helix that includes this file gives this pointer a value (see the GC
 * macro below).  Otherwise it stays NULL, which is how main() knows whether it
 * was compiled alone.
 */
int (*helix_decoder)(void);

int unused_value;       /* lets the helix text form valid C, see BASE below */
int letters_seen;
int decoded_byte;

static void put_repeated(int count, int character)
{
    for (; count > 0; count--)
        putchar(character);
}

/* Called for every letter in the helix, in reading order. */
void decode_base(int base)
{
    int on_first_strand = letters_seen % 2 == 0;

    letters_seen++;

    /* The second strand is only the complement of the first, so skip it. */
    if (!on_first_strand)
        return;

    decoded_byte = decoded_byte * 4 + base;

    /* The last base of a byte is letter number 7 out of every 8. */
    if (letters_seen % LETTERS_PER_BYTE == LETTERS_PER_BYTE - 1) {
        if (decoded_byte != 0)
            putchar(decoded_byte);
        decoded_byte = 0;
    }
}

static void generate_helix(void)
{
    static const char first_strand[] = "ACGT";
    static const char second_strand[] = "TGCA";
    int row = 0;
    int place_value = 0;
    int input_byte = 0;

    /* This names the original program, so the helix depends on prog.c. */
    puts("#include \"prog.c\"\n\n     AT");

    for (;;) {
        int bond_width;

        /* All of the current byte has been written, so fetch the next one. */
        if (place_value == 0) {
            input_byte = getchar();
            if (input_byte == EOF)
                input_byte = 0;
            place_value = FIRST_PLACE_VALUE;
        }

        row++;
        bond_width = bond_widths[row % HALF_PERIOD];
        put_repeated(row_indents[row % HELIX_PERIOD], ' ');

        /* Only stop at a crossing, once the input has run out. */
        if (bond_width == 0 && input_byte == 0)
            break;

        if (bond_width == 0) {
            puts(row % HALF_PERIOD != 0 ? "CG" : "TA");
        } else {
            int base = input_byte / place_value % 4;

            putchar(first_strand[base]);
            put_repeated(bond_width, '~');
            putchar(second_strand[base]);
            putchar('\n');
            place_value /= 4;
        }

        row %= HELIX_PERIOD;
    }

    puts("GC");
}

int main(void)
{
    if (helix_decoder)
        helix_decoder();
    else
        generate_helix();
    return 0;
}

/*
 * The rest of this file only matters to a helix that includes it.  Each piece
 * of helix text is one of these macros, and together they expand to a function:
 *
 *   AT      opens decode_helix()
 *   A~~T    one base pair
 *   CG, TA  strand crossings, which hold no data
 *   GC      closes decode_helix() and registers it with helix_decoder
 *
 * BASE ends with an open assignment and begins with the value it needs, so
 * the '~' characters between two letters are just a harmless expression:
 *
 *   A~~T  ->  unused_value; decode_base(0); unused_value =
 *             ~~unused_value; decode_base(3); unused_value =
 */
#define BASE(value) unused_value; decode_base(value); unused_value =

#define A BASE(0)
#define C BASE(1)
#define G BASE(2)
#define T BASE(3)

#define CG
#define TA

#define AT int decode_helix(void) { unused_value =
#define GC unused_value; return 0; } int (*helix_decoder)(void) = decode_helix;
