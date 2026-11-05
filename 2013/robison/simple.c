#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * How a number is stored
 * ----------------------
 * A number is a string with one character for every unit of every digit. The
 * character says which place the unit belongs to: character 1 is the ones
 * place, character 2 is the radix place, character 3 is the radix squared
 * place and so on. In base 10 the number 203 is "\1\1\1\3\3".
 *
 * The characters can be in any order, which turns arithmetic into text work:
 *   - adding two numbers means joining their strings, then carrying
 *   - comparing two numbers means sorting both from high to low, then strcmp
 *   - multiplying by a power of the radix means raising every character
 *
 * A negative number also contains SIGN_MARK. The same character marks units
 * that are about to be deleted, because sorting moves it past every real place
 * so that one cut removes all of them.
 */

#define SIGN_MARK 127
#define SIGN_STRING "\177"
#define HIGHEST_PLACE 126
#define RADIX_AS_NUMBER "\2"    /* one unit in the second place */
#define END_OF_INPUT "\377"     /* EOF as it looks after storing it in a char */

static char radix;
static char digit_symbols[128] = "0123456789abcdefghij";
static char current;            /* the input character being looked at */

static char work[6000000];      /* the number being calculated */
static char quotient[2400];
static char previous[2400];     /* value of the digits read so far */
static char divisor_scaled[2400];
static char run[128];           /* the same place repeated radix times */

static int compare_low_to_high(const void *left, const void *right)
{
    return strncmp(left, right, 1);
}

static int compare_high_to_low(const void *left, const void *right)
{
    return compare_low_to_high(right, left);
}

static int compare_first_letters(const void *left, const void *right)
{
    return compare_low_to_high(*(char **)left, *(char **)right);
}

/* Sorts low to high, which also cuts off every unit marked for deletion. */
static void sort_and_delete_marked(char *number)
{
    char *mark;

    qsort(number, strlen(number), 1, compare_low_to_high);
    mark = strchr(number, SIGN_MARK);
    if (mark) {
        *mark = 0;
    }
}

static void sort_work_high_to_low(void)
{
    qsort(work, strlen(work), 1, compare_high_to_low);
}

/* Sorts number high to low and returns it without any sign. */
static char *magnitude_of(char *number)
{
    qsort(number, strlen(number), 1, compare_high_to_low);
    return *number == SIGN_MARK ? number + 1 : number;
}

/*
 * A place past the end of the supported range is stored as a negative char.
 * These values only come from going over the limit of about 125 digits, which
 * the program does not support and where it crashes instead of continuing.
 */
static int checked_place(char place)
{
    if (place < 0) {
        abort();
    }
    return place;
}

static void append_unit(char *number, char place)
{
    size_t length = strlen(number);

    number[length] = place;
    number[length + 1] = 0;
}

/* Carries in work: every radix units of a place become one unit of the next. */
static void normalize(void)
{
    int place;

    sort_and_delete_marked(work);
    for (place = 1; place < HIGHEST_PLACE; place++) {
        char *found;
        int carried = 0;

        memset(run, place, radix);
        while ((found = strstr(work, run))) {
            memset(found, SIGN_MARK, radix);
            *found = place + 1;
            carried = 1;
        }
        if (carried) {
            sort_and_delete_marked(work);
        }
    }
}

/* Adds left * right to work. */
static void multiply(const char *left, const char *right)
{
    const char *left_unit, *right_unit;

    for (left_unit = left; *left_unit; left_unit++) {
        for (right_unit = right; *right_unit; right_unit++) {
            /* radix^(a-1) * radix^(b-1) = radix^(a+b-2), which is place a+b-1 */
            append_unit(work, checked_place(*left_unit) + checked_place(*right_unit) - 1);
        }
    }
    normalize();
}

/*
 * Subtracts number from work, which must not be smaller. The units of number
 * are taken away from the left, highest place first. When a unit is missing,
 * the highest unit of work is broken up into radix - 1 units of every place
 * below it, down to the missing place, plus the one unit that is removed.
 */
static void subtract(const char *number)
{
    const char *unit;

    for (unit = number; *unit; unit++) {
        char *removed = strchr(work, *unit);

        if (!removed) {
            int place;

            sort_and_delete_marked(work);
            sort_work_high_to_low();
            removed = work;
            for (place = checked_place(*work) - 1; place >= *unit; place--) {
                memset(run, place, radix);
                strncat(work, run, radix - 1);
            }
        }
        *removed = SIGN_MARK;
    }
    normalize();
}

/*
 * Long division. work holds the dividend sorted high to low and is left with
 * the remainder, and the result goes into quotient. The divisor is scaled up
 * by the radix until it reaches the size of work, then it is subtracted as many
 * times as it fits at each size on the way back down.
 */
static void divide(const char *divisor)
{
    char *unit;
    int place = 1;

    /* Division by zero is not handled and crashes the program. */
    if (!*divisor) {
        abort();
    }

    strcpy(divisor_scaled, divisor);
    while (strncmp(work, divisor_scaled, 1) > 0) {
        for (unit = divisor_scaled; *unit; unit++) {
            *unit = checked_place(*unit) + 1;
        }
        place++;
    }

    *quotient = 0;
    while (place > 0) {
        while (strcmp(work, divisor_scaled) >= 0) {
            subtract(divisor_scaled);
            sort_work_high_to_low();
            append_unit(quotient, place);
        }
        place--;
        for (unit = divisor_scaled; *unit; unit++) {
            *unit = checked_place(*unit) - 1;
        }
    }
}

/*
 * Returns a copy of number on the heap, with enough space for adding a sign.
 * Appending two characters first makes strlen give the size of that copy.
 */
static char *copy_number(char *number)
{
    char *copy = malloc(strlen(strcat(number, SIGN_STRING " ")));

    *strstr(number, SIGN_STRING) = 0;
    return strcpy(copy, number);
}

/* Prints a number and frees it. */
static void print_number(char *number)
{
    char *magnitude = magnitude_of(number);
    int place = checked_place(*magnitude);

    if (magnitude > number) {
        putchar('-');
    }
    do {
        const char *unit;
        int count = 0;

        for (unit = magnitude; *unit; unit++) {
            count += *unit == place;
        }
        putchar(count < radix ? digit_symbols[count] : 0);
        place--;
    } while (place > 0);
    putchar('\n');
    free(number);
}

/* Reads the next character. Spaces and NUL bytes are skipped. */
static void advance(void)
{
    do {
        current = getchar();
    } while (current == ' ' || current == 0);
}

/* If the current character is in set, moves past it and returns it. */
static char accept(const char *set)
{
    const char *match = strchr(set, current);

    if (!match) {
        return 0;
    }
    advance();
    return *match;
}

static char *parse_sum(void);

/* factor: '-' factor | '(' sum ')' | digits */
static char *parse_factor(void)
{
    char *value;
    int valid;

    if (accept("-")) {
        char *sign;

        valid = 1;
        value = parse_factor();
        sign = strstr(value, SIGN_STRING);
        if (sign) {
            *sign = 0;
        } else {
            strcat(value, SIGN_STRING);
        }
    } else if (accept("(")) {
        value = parse_sum();
        valid = accept(")");
    } else {
        char symbol;

        valid = 0;
        *work = 0;
        while ((symbol = accept(digit_symbols))) {
            strcpy(previous, work);
            memset(work, 0, radix);
            memset(work, 1, strchr(digit_symbols, symbol) - digit_symbols);
            multiply(previous, RADIX_AS_NUMBER);
            valid = 1;
        }
        value = copy_number(work);
    }
    if (!valid) {
        puts(strcpy(work, "syntax error"));
    }
    return value;
}

/* term: factor { ('*' | '/' | '%') factor } */
static char *parse_term(void)
{
    char *left = parse_factor();
    char operator;

    while ((operator = accept("/*%"))) {
        char *right = parse_factor();
        char *left_magnitude = magnitude_of(left);
        char *right_magnitude = magnitude_of(right);
        int left_negative = left_magnitude != left;
        int right_negative = right_magnitude != right;
        char *result = work;

        *work = 0;
        if (operator == '*') {
            multiply(left_magnitude, right_magnitude);
        } else {
            strcpy(work, left_magnitude);
            divide(right_magnitude);
            if (operator == '/') {
                result = quotient;
            } else {
                /* A remainder takes the sign of the dividend alone. */
                right_negative = 0;
            }
        }
        free(left);
        left = copy_number(result);
        if (left_negative != right_negative) {
            strcat(left, SIGN_STRING);
        }
        free(right);
    }
    return left;
}

/* sum: term { ('+' | '-') term } */
static char *parse_sum(void)
{
    char *left = parse_term();
    char operator;

    while ((operator = accept("+-"))) {
        char *right = parse_term();
        char *left_magnitude = magnitude_of(left);
        char *right_magnitude = magnitude_of(right);
        int left_negative = left_magnitude != left;
        /* a - b is a + (-b), so subtracting flips the sign of the right side. */
        int right_negative = (right_magnitude != right) != (operator == '-');
        int result_negative = right_negative;

        if (left_negative == right_negative) {
            strcat(strcpy(work, left_magnitude), right_magnitude);
            normalize();
        } else if (strcmp(left_magnitude, right_magnitude) < 0) {
            strcpy(work, right_magnitude);
            subtract(left_magnitude);
        } else {
            strcpy(work, left_magnitude);
            subtract(right_magnitude);
            result_negative = left_negative;
        }
        free(left);
        free(right);
        left = copy_number(work);
        if (result_negative) {
            strcat(left, SIGN_STRING);
        }
    }
    return left;
}

/*
 * The radix is an optional argument. Sorting the arguments puts it in first
 * position only when it sorts before the program name, so "./simple 16" picks
 * the name and gets radix 0 while "./simple +16" works. Base 0 tells strtol to
 * also accept 0x and 0 prefixes.
 */
int main(int argc, char **argv)
{
    qsort(argv, argc, sizeof *argv, compare_first_letters);
    radix = 1 < argc ? strtol(*argv, 0, 0) : 10;

    /*
     * Normalizing does nothing to the symbols for a radix of 2 or more. A radix
     * of 1 merges all of them into one symbol, and a radix of 0 never finishes.
     */
    strcpy(work, digit_symbols);
    normalize();
    strcpy(digit_symbols, work);

    advance();
    while (!accept(END_OF_INPUT)) {
        print_number(parse_sum());
        while (!accept(END_OF_INPUT "\n")) {
            advance();
        }
    }
    return 0;
}
