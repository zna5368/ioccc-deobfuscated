#include<stdio.h>
#include<setjmp.h>

/*
 * All working storage lives in one block so that the areas sit directly after
 * each other. An over-long command line runs from the line buffer into the
 * token table, and a byte above 0x7f is a negative index that reads the token
 * table backwards into the line buffer. Both quirks depend on this layout.
 */
#define MEMORY_SIZE 65536    /* far more than the 512 bytes in normal use */
#define LINE_OFFSET 0        /* 128 bytes: current statement or output text */
#define ARGUMENTS_OFFSET 64  /* command line text, in the upper half of it */
#define TOKENS_OFFSET 128    /* 128 bytes: maps a character to its token */
#define HEX_DIGITS_OFFSET 256 /* 32 bytes: "0123456789abcdef" */
#define REGISTERS_OFFSET 288 /* 28 long longs: see below */

#define LINE_SIZE 128
#define REGISTER_COUNT 26
#define MIN_HEX_DIGITS 8
#define MAX_HEX_DIGITS 16

/* The position of each operator in OPERATORS, plus one, is its token value. */
#define OPERATORS "- +^|*/%&<>~?.()!"
enum token {
    TOKEN_END,
    TOKEN_MINUS,
    TOKEN_SPACE,
    TOKEN_PLUS,
    TOKEN_XOR,
    TOKEN_OR,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_AND,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_COMPLEMENT,
    TOKEN_BYTE_SWAP,
    TOKEN_DOT,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_NOT,
    TOKEN_DIGIT,
    TOKEN_LETTER,
    TOKEN_ASSIGN
};

static const char syntax_error_text[] = "syntax error";

static long long memory[MEMORY_SIZE / sizeof(long long)];
static char *line;              /* line buffer */
static char *tokens;            /* token table */
static char *hex_digits;        /* hex conversion table */
static long long *registers;    /* registers[1] to [26] are the variables a to z */

/* True for a silent "x = expression". Stored in the unused register slot 0. */
#define is_assignment registers[0]

static int (*next_char)(void);  /* getchar, or next_argument_char */
static char *argument_cursor;   /* next unread command line character */
static char *cursor;            /* current position in the line buffer */
static char *line_limit;        /* where reading of a statement must stop */

/*
 * While reading: the last character read plus one, so zero means end of input.
 * While printing: the part of the number that is still to be converted. It
 * ends up zero, which read_statement relies on after listing the registers.
 */
long long work;

static long long last_result;   /* the "." value */
static jmp_buf error_recovery;

static long long expression(void);
static long long factor(void);

/* Abandon the current statement and return to the setjmp in main. */
static void syntax_error(void)
{
    is_assignment = 0;
    longjmp(error_recovery, 1);
}

/* Print a string and a newline. */
static void print_line(const char *text)
{
    while (*text)
        putchar(*text++);
    putchar('\n');
}

/* A getchar replacement that reads the saved command line arguments. */
static int next_argument_char(void)
{
    return *argument_cursor ? *argument_cursor++ : EOF;
}

/* Lay out memory and fill in the token and hex digit tables. */
static void initialize_memory(void)
{
    const char *operators = OPERATORS;
    int i;

    line = (char *)memory + LINE_OFFSET;
    tokens = (char *)memory + TOKENS_OFFSET;
    hex_digits = (char *)memory + HEX_DIGITS_OFFSET;
    registers = (long long *)((char *)memory + REGISTERS_OFFSET);

    for (i = 0; operators[i]; i++)
        tokens[(int)operators[i]] = i + 1;
    tokens['\t'] = TOKEN_SPACE;
    tokens['='] = TOKEN_ASSIGN;

    for (i = 0; i < 10; i++) {
        hex_digits[i] = '0' + i;
        tokens['0' + i] = TOKEN_DIGIT;
    }
    for (i = 0; i < 6; i++)
        hex_digits[10 + i] = 'a' + i;
    for (i = 0; i < REGISTER_COUNT; i++)
        tokens['a' + i] = tokens['A' + i] = TOKEN_LETTER;
}

/*
 * Join the arguments with spaces in the upper half of the line buffer, and
 * choose where input comes from: the arguments if any, otherwise stdin.
 */
static void select_input(char **argv)
{
    char *destination = argument_cursor = line + ARGUMENTS_OFFSET;
    const char *source;

    while ((source = *++argv)) {
        while ((*destination++ = *source++))
            ;
        destination[-1] = ' ';
    }
    next_char = destination != argument_cursor ? next_argument_char : getchar;
}

/* Return the next token that is not whitespace. */
static int next_token(void)
{
    int token;

    do
        token = tokens[(int)*cursor++];
    while (token == TOKEN_SPACE);
    return token;
}

/* Swap the two bytes of a 16 bit value. */
static unsigned long long swap_16_bits(unsigned short value)
{
    return value >> 8 | (value & 0xff) << 8;
}

/* Reverse the four bytes of a 32 bit value. */
static unsigned long long swap_32_bits(unsigned int value)
{
    return (swap_16_bits(value) << 16) + swap_16_bits(value >> 16);
}

/* Reverse the bytes of a value, using the smallest of 2, 4 or 8 bytes. */
static long long swap_bytes(unsigned long long value)
{
    if (!(value >> 16))
        return swap_16_bits(value);
    if (!(value >> 32))
        return swap_32_bits(value);
    return swap_32_bits(value) << 32 | swap_32_bits(value >> 32);
}

/*
 * Write work in decimal, after the sign if there is one, and return the end of
 * the text. The digits are built backwards in the line buffer and moved down.
 */
static char *format_decimal(int add_one)
{
    char *digit = line + 32;
    char *out;

    while (work) {
        *digit-- = '0' + work % 10;
        work /= 10;
    }
    out = line + (*line == '-');
    while (digit < line + 32)
        *out++ = *++digit;
    if (add_one)
        ++out[-1];
    return out;
}

/*
 * Check that the statement has ended, remember the value in ".", and write it
 * to the line buffer as "decimal<tab>0xhex" with at least eight hex digits.
 *
 * line_limit moves to the end of this text, so the rest of a statement that
 * lists registers is cut short.
 */
static void format_result(unsigned long long value)
{
    char *out = line;
    char *hex_digit;
    char *hex_end;
    int add_one = 0;

    if (next_token() != TOKEN_END)
        syntax_error();

    last_result = work = value;
    *out = ' ';
    if (work < 0) {
        *out++ = '-';
        /*
         * The most negative number has no positive partner, so print one less
         * and fix the last digit. An optimizing compiler may assume that
         * negation never overflows and remove this test, as it does for the
         * original program, and then that number prints as junk characters.
         */
        if (work == -work) {
            add_one = 1;
            work--;
        } else {
            work = -work;
        }
    }
    if (!work)
        *out++ = '0';
    else
        out = format_decimal(add_one);

    *out++ = '\t';
    *out++ = '0';
    *out++ = 'x';

    /* Build the hex digits backwards, then move them down to out. */
    hex_digit = hex_end = line_limit = out + MAX_HEX_DIGITS;
    while (value) {
        *hex_digit-- = hex_digits[value & 0xf];
        value >>= 4;
    }
    while (hex_digit - MIN_HEX_DIGITS > out)
        *hex_digit-- = '0';
    while (hex_digit < hex_end)
        *out++ = *++hex_digit;
    *out = '\0';
}

/*
 * Handle "$": print every register that is not zero. The formatted text
 * overwrites the line buffer, so the end of the statement is marked again.
 */
static void list_registers(char *statement_end)
{
    int i;

    *statement_end = '\0';
    for (i = 1; i <= REGISTER_COUNT; i++) {
        if (registers[i]) {
            format_result(registers[i]);
            putchar('a' + i - 1);
            putchar('\t');
            print_line(line);
            cursor = statement_end;
            *cursor = '\0';
        }
    }
}

/*
 * Store the next input character at the cursor. Returns zero at the end of
 * input, and also for the byte 0xff because the result is cut down to a char.
 */
static char read_char(void)
{
    *cursor = work = next_char();
    return ++work;
}

/*
 * Read one statement into the line buffer, stopping at a newline or semicolon
 * and removing any comment. Blank statements are skipped. Returns the length,
 * which is zero when the input has run out.
 */
static long long read_statement(void)
{
    char *comment;
    char *first_text;

    do {
        comment = NULL;
        first_text = NULL;
        cursor = line;
        line_limit = line + LINE_SIZE;

        while (cursor < line_limit && read_char()) {
            if (*cursor == '$') {
                list_registers(cursor);
            } else if (*cursor == '\n') {
                if (comment)
                    cursor = comment;
                line_limit = cursor;
            } else if (comment) {
                /* inside a comment: the next character replaces this one */
            } else if (*cursor == '#') {
                comment = cursor;
            } else if (*cursor == ';') {
                line_limit = cursor;
            } else if (!first_text && tokens[(int)*cursor] != TOKEN_SPACE) {
                first_text = cursor++;
            } else {
                cursor++;
            }
        }
        *cursor = '\0';
    } while (work && !first_text);

    return cursor - line;
}

/*
 * Return what must be subtracted from a character to get its digit value in
 * the given base, or zero if the character is not a digit in that base.
 * Octal numbers are not strict: they also accept the digits 8 and 9.
 */
static int digit_origin(int character, long long base)
{
    if (tokens[character] == TOKEN_DIGIT)
        return '0';
    if (base > 10 && tokens[character] == TOKEN_LETTER && (character | 0x20) <= 'f')
        return (character & 0x20 ? 'a' : 'A') - 10;
    return 0;
}

/*
 * Convert a number at the cursor. The base is decimal, or octal after a leading
 * 0, or hex after a leading 0x. The letter test is loose: any character from
 * 'x' to '{', in either case, introduces a hex number.
 */
static long long parse_number(void)
{
    long long base = 10;
    long long number = 0;
    int character;
    int origin;

    if (*cursor == '0') {
        base = 8;
        cursor++;
        if ((*cursor | 0x20) >= 'x' && (*cursor | 0x20) <= '{') {
            base = 16;
            cursor++;
        }
    }

    while ((character = *cursor++) && (origin = digit_origin(character, base)))
        number = number * base + character - origin;

    cursor--;
    return number;
}

/* The value of the register named by the character before the cursor. */
static long long *current_register(void)
{
    return &registers[(cursor[-1] | 0x20) - 'a' + 1];
}

/*
 * factor: - + ~ ? ! factor, ( expression ), ".", a number or a register.
 * A stray ")" at the start of a factor happens to behave the same as "!".
 */
static long long factor(void)
{
    long long value;

    switch (next_token()) {
    case TOKEN_MINUS:
        return -factor();
    case TOKEN_PLUS:
        return factor();
    case TOKEN_COMPLEMENT:
        return ~factor();
    case TOKEN_BYTE_SWAP:
        return swap_bytes(factor());
    case TOKEN_NOT:
    case TOKEN_RIGHT_PAREN:
        return !factor();
    case TOKEN_DOT:
        return last_result;
    case TOKEN_LEFT_PAREN:
        value = expression();
        if (next_token() != TOKEN_RIGHT_PAREN)
            syntax_error();
        return value;
    case TOKEN_DIGIT:
        cursor--;
        return parse_number();
    case TOKEN_LETTER:
        return *current_register();
    }
    syntax_error();
    return 0;
}

/*
 * Finish a shift that started with first_token: the operators are <<, >> which
 * keeps the sign, and >>> which does not.
 */
static long long shift(long long value, int first_token)
{
    if (tokens[(int)*cursor++] != first_token)
        syntax_error();

    if (first_token == TOKEN_LESS)
        return value << factor();

    if (tokens[(int)*cursor++] != TOKEN_GREATER) {
        cursor--;
        return value >> factor();
    }
    return (unsigned long long)value >> factor();
}

/* term: factors joined by * / % & << >> >>> */
static long long term(void)
{
    long long value = factor();
    int token;

    while ((token = next_token()) >= TOKEN_MULTIPLY && token <= TOKEN_GREATER) {
        switch (token) {
        case TOKEN_MULTIPLY:
            value *= factor();
            break;
        case TOKEN_DIVIDE:
            value /= factor();
            break;
        case TOKEN_MODULO:
            value %= factor();
            break;
        case TOKEN_AND:
            value &= factor();
            break;
        default:
            value = shift(value, token);
            break;
        }
    }
    cursor--;
    return value;
}

/* expression: terms joined by + - ^ | */
static long long expression(void)
{
    long long value = term();
    int token;

    while ((token = next_token()) >= TOKEN_MINUS && token <= TOKEN_OR) {
        switch (token) {
        case TOKEN_MINUS:
            value -= term();
            break;
        case TOKEN_PLUS:
            value += term();
            break;
        case TOKEN_XOR:
            value ^= term();
            break;
        case TOKEN_OR:
            value |= term();
            break;
        }
    }
    cursor--;
    return value;
}

/*
 * Choose where the statement's value goes: a register for "x = expression",
 * otherwise ".". Leaves the cursor at the start of the expression.
 */
static long long *assignment_target(void)
{
    long long *target;

    if (next_token() == TOKEN_LETTER) {
        target = current_register();
        if (next_token() == TOKEN_ASSIGN) {
            is_assignment = 1;
            return target;
        }
    }
    is_assignment = 0;
    cursor = line;
    return &last_result;
}

int main(int argc, char **argv)
{
    long long *target;

    *argv = basename(*argv);
    initialize_memory();
    select_input(argv);

    while (read_statement()) {
        /* A program name that starts with e echoes each statement. */
        if (**argv == 'e')
            print_line(line);

        if (!setjmp(error_recovery)) {
            cursor = line;
            target = assignment_target();
            *target = expression();
            format_result(*target);
            if (!is_assignment)
                print_line(line);
        } else {
            print_line(syntax_error_text);
        }
    }
    return 0;
}
