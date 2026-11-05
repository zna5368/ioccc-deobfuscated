#include <stdio.h>
#include <stdlib.h>

/* The number of characters shown after the one that caused a nesting error. */
#define CONTEXT_AFTER_ERROR 3

/*
 * The original's table held this many entries, and the low byte of its size
 * became the first "previous character": '@' on 64-bit, ' ' on 32-bit.
 */
#define ORIGINAL_TABLE_ENTRIES 132
#define INITIAL_PREVIOUS_CHAR \
    ((unsigned char)(ORIGINAL_TABLE_ENTRIES * 2 * sizeof(void *)))

enum char_kind { PLAIN, OPENER, CLOSER, QUOTE };

/* Which characters pair up with each other. */
enum char_group {
    TOP_LEVEL,      /* not inside any bracket */
    ROUND,          /* ( ) */
    SQUARE,         /* [ ] */
    CURLY,          /* { } */
    NEVER_OPENED,   /* a closer that has no opener */
    DOUBLE_QUOTE,   /* " */
    SINGLE_QUOTE    /* ' */
};

struct char_class {
    enum char_kind kind;
    enum char_group group;
};

/*
 * Every byte not listed is PLAIN. The NUL and 0x80 to 0x83 rows are in the
 * original table too, so those bytes act like the characters listed here.
 */
static const struct char_class char_classes[256] = {
    ['\0'] = {OPENER, ROUND},
    ['('] = {OPENER, ROUND},
    [')'] = {CLOSER, ROUND},
    ['['] = {OPENER, SQUARE},
    [']'] = {CLOSER, SQUARE},
    ['{'] = {OPENER, CURLY},
    ['}'] = {CLOSER, CURLY},
    ['"'] = {QUOTE, DOUBLE_QUOTE},
    ['\''] = {QUOTE, SINGLE_QUOTE},
    [0x80] = {QUOTE, SINGLE_QUOTE},
    [0x81] = {OPENER, SQUARE},
    [0x83] = {CLOSER, NEVER_OPENED},
};

static unsigned char previous_char = INITIAL_PREVIOUS_CHAR;
static int in_string = 0;
static enum char_group string_quote;

/*
 * Print the next few characters of the input. Reaching the end of the input
 * prints the last character once more and stops.
 */
static void print_context_after_error(void)
{
    int shown;

    for (shown = 0; shown < CONTEXT_AFTER_ERROR; shown++) {
        int input = getchar();

        if (input == EOF) {
            putchar(previous_char);
            return;
        }
        putchar(input);
        previous_char = (unsigned char)input;
    }
}

/*
 * Read the input until the bracket group that is currently open gets closed.
 * Each opening bracket starts a new call. Returns 1 if the nesting is correct
 * and 0 if an error was found.
 */
static int check_nesting(enum char_group open_group)
{
    for (;;) {
        int input = getchar();
        unsigned char current;
        struct char_class class;

        if (input == EOF) {
            /* Unclosed brackets or an unclosed string are errors. */
            return open_group == TOP_LEVEL && !in_string;
        }
        current = (unsigned char)input;
        class = char_classes[current];

        if (class.kind == QUOTE) {
            /* The other kind of quote is ignored inside a string. */
            if (!in_string) {
                in_string = 1;
                string_quote = class.group;
            } else if (class.group == string_quote) {
                in_string = 0;
            }
        } else if (class.kind == OPENER && !in_string) {
            previous_char = current;
            if (!check_nesting(class.group)) {
                return 0;
            }
            continue;
        } else if (class.kind == CLOSER && !in_string) {
            if (class.group == open_group) {
                previous_char = current;
                return 1;
            }
            putchar(previous_char);
            putchar(current);
            previous_char = current;
            print_context_after_error();
            return 0;
        }
        previous_char = current;
    }
}

int main()
{
    /* The exit code is the length of the printed message: 3 or 10. */
    return printf(check_nesting(TOP_LEVEL) ? "OK\n" : "\n ^ Error\n");
}
