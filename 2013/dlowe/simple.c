#include <stdio.h>
#include <stdlib.h>

/* Values are scaled onto the bar levels 0 to TOP_LEVEL */
#define TOP_LEVEL 8

/*
 * Unicode block elements U+2581 to U+2589, one for each bar level. The scaling
 * only reaches level 7 for values just below the maximum, and the maximum
 * itself is level 8, so it is drawn with U+2589 instead of the full block.
 */
static const char *const BARS[TOP_LEVEL + 1] = {
    "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█", "▉"
};

/* Text that is not a number parses as 0 */
static float parse_number(const char *text)
{
    return strtof(text, NULL);
}

/*
 * Scale value from the range lowest to highest onto a bar level. This has to be
 * floating point because the range is empty when all the numbers are equal, and
 * 0 / 0 is then NaN and not a crash. The lowest bar is used for any height that
 * is not a real level, such as NaN or an overflow to infinity.
 */
static int bar_level(float value, float lowest, float highest)
{
    float height = TOP_LEVEL * (value - lowest) / (highest - lowest);

    if (height >= 0 && height <= TOP_LEVEL) {
        return (int)height;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int lowest = 1;  /* index in argv of the smallest number */
    int highest = 1; /* index in argv of the largest number */
    int i;

    for (i = 2; i < argc; i++) {
        float number = parse_number(argv[i]);

        if (number < parse_number(argv[lowest])) {
            lowest = i;
        } else if (number > parse_number(argv[highest])) {
            highest = i;
        }
    }

    for (i = 1; i < argc; i++) {
        int level = bar_level(parse_number(argv[i]),
                              parse_number(argv[lowest]),
                              parse_number(argv[highest]));

        printf("%s", BARS[level]);
    }

    return 0;
}
