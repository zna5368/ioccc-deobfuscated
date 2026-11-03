#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Copy standard input to standard output, replacing $NAME$ with the value
 * of the environment variable NAME. Backslash escapes of up to three
 * digits in the value (for example \33) are turned into the byte they
 * encode in octal.
 */

#define DOLLAR '$'
#define BACKSLASH '\\'
#define SPACE ' '

#define LINE_SIZE 256
#define OUTPUT_SIZE 10240

#define IS_DIGIT(ch) ((ch) >= '0' && (ch) <= '9')

char line[LINE_SIZE];
char expanded[OUTPUT_SIZE];
int length;

/*
 * Copy an environment variable value to out, decoding backslash escapes.
 * A backslash followed by up to three digits becomes the single byte with
 * that octal value (the digits 8 and 9 are accepted too). A backslash that
 * is not followed by a digit is copied unchanged. Returns the new end of out.
 */
char *copy_value(char *out, char *value)
{
    unsigned char byte;

    while (*value) {
        if (*value != BACKSLASH) {
            *out++ = *value++;
            continue;
        }

        value++;
        if (!IS_DIGIT(*value)) {
            *out++ = BACKSLASH;
            continue;
        }

        byte = *value++ - '0';
        if (IS_DIGIT(*value)) {
            byte = (byte << 3) + (*value++ - '0');
            if (IS_DIGIT(*value))
                byte = (byte << 3) + (*value++ - '0');
        }
        *out++ = byte;
    }
    return out;
}

/*
 * Expand all $NAME$ references in line. Sets length to the number of bytes
 * in the result and returns a pointer to the result.
 */
char *expand_line(void)
{
    char *in, *out, *close, *value;

    length = strlen(line);
    if (!strchr(line, DOLLAR))
        return line;

    in = line;
    out = expanded;
    while (*in) {
        if (*in != DOLLAR) {
            *out++ = *in++;
            continue;
        }

        in++; /* skip the opening $ */
        close = strchr(in, DOLLAR);
        if (!close) {
            /* lone $ with nothing to close it */
            *out++ = DOLLAR;
            continue;
        }

        *close = 0; /* in now holds just the variable name */
        if (strchr(in, SPACE)) {
            /* names with spaces are not variables; the closing $ may
               start another reference */
            *close = DOLLAR;
            *out++ = DOLLAR;
            continue;
        }

        value = getenv(in);
        if (value) {
            out = copy_value(out, value);
            in = close + 1;
        } else {
            /* unknown variable: keep $NAME$ unchanged */
            *close = DOLLAR;
            *out++ = DOLLAR;
            while (*in != DOLLAR)
                *out++ = *in++;
            in++;
            *out++ = DOLLAR;
        }
    }

    length = out - expanded;
    return expanded;
}

int main()
{
    char *result;

    while (fgets(line, LINE_SIZE - 1, stdin)) {
        result = expand_line();
        fwrite(result, length, 1, stdout);
    }
    return 0;
}
