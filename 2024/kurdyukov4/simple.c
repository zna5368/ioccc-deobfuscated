#include <stdio.h>
#include <math.h>

/*
 * Reads binary data from stdin and writes a C program to stdout that, when
 * compiled and run, prints that same data -- without using a single literal.
 *
 * The generated program looks like:
 *
 *     #include <stdio.h>
 *     int a;int main(){int b=-~a,c=b*b+b,d=c*c,...,Z[]={...};fwrite(Z,b,N,stdout);}
 *
 * The global "a" is zero, "b" is -~a == 1, and every other number n is built
 * as  r*r + d  or  r*r - d  where r = round(sqrt(n)) and d = |n - r*r|, both of
 * which are smaller numbers built the same way.
 */

#define NAME_COUNT  (1 << 16)   /* numbers below this get their own variable */
#define VALUES_SIZE (1 << 28)   /* size of the text buffer holding Z[]       */

/* variable name for each number 0..65535 (up to 3 chars + NUL), "" if unused */
char names[NAME_COUNT][4];
/* text of the Z[] initializer: ",v1,v2,v3,..." */
char values[VALUES_SIZE];
/* the most recently created variable name */
char *last_name = names[1];
char *values_end = values;
/* expression text "r*r+d" for the number being built */
char expression[12];

/*
 * Produce the name that follows prev in the sequence:
 *   a..z A..Y              (single letters; 'Z' is reserved for the array)
 *   jA..zA AA..ZA jB ...   (first letter j..z A..Z avoids "do", "if" ...)
 *   jAa ... jA0 ...        (third char a..z then 0..9; avoids "int", "for")
 */
static void next_name(const char *prev, char *name)
{
    char first = prev[0], second = prev[1], third = prev[2];
    int first_wraps = second == 0 ? first == 'Y' : first == 'Z';

    if (!first_wraps) {
        first = first == 'z' ? 'A' : first + 1;
    } else {
        first = 'j';
        if (second == 'z') {
            second = 'A';
            if (third == 0)
                third = 'a';
            else if (third == 'z')
                third = '0';
            else
                third = third + 1;
        } else if (second == 'Z') {
            second = 'a';
        } else if (second == 0) {
            second = 'A';
        } else {
            second = second + 1;
        }
    }
    name[0] = first;
    name[1] = second;
    name[2] = third;
}

/*
 * Return C text that evaluates to value. Numbers below 65536 become variables
 * (printing their declaration the first time they are needed); larger ones
 * are returned as an inline expression.
 */
static char *number_expr(unsigned value)
{
    char *name = 0;
    char *root_name, *diff_name;
    int root, diff, sign;

    if (value < NAME_COUNT) {
        name = names[value];
        if (name[0] != 0)
            return name;            /* already declared */
    }

    root = sqrt(value) + 0.5;
    diff = (int)(value - (unsigned)root * (unsigned)root);
    /* root may be 65536; masking makes it 0 and the wrap-around still works */
    root_name = number_expr(root & 65535);
    diff_name = number_expr(diff < 0 ? -diff : diff);

    if (diff == 0) {
        sprintf(expression, "%s*%s", root_name, root_name);
    } else {
        sign = diff < 0 ? '-' : '+';
        sprintf(expression, "%s*%s%c%s", root_name, root_name, sign, diff_name);
    }

    if (name == 0)
        return expression;          /* too big for a variable: inline it */

    next_name(last_name, name);
    printf(",%s=%s", name, expression);
    last_name = name;
    return name;
}

/* add ",<expr>" for value to the Z[] initializer */
static void append_value(unsigned value)
{
    values_end += sprintf(values_end, ",%s", number_expr(value));
}

int main()
{
    int word, count, ch, total = 0;
    char *length_name;

    names[0][0] = 'a';              /* 0 is the global "int a"       */
    names[1][0] = 'b';              /* 1 is "int b=-~a" (~0 is -1)  */

    printf("#include <stdio.h>\nint a;int main(){int b=-~a");

    /* read the input 4 bytes at a time, each chunk becomes one int of Z[] */
    do {
        word = 0;
        for (count = 0; count < 4 && (ch = getchar()) >= 0; total++)
            ((char *)&word)[count++] = ch;
        if (count > 0)
            append_value(word);
    } while (count == 4 && values_end - values + 33 < VALUES_SIZE);

    /* build the byte count, then cut it off the end of the value list */
    length_name = values_end;
    append_value(total);
    *length_name++ = 0;             /* replace its ',' with a terminator */

    printf(",Z[]={%s};fwrite(Z,b,%s,stdout);}\n", values + 1, length_name);
    return 0;
}
