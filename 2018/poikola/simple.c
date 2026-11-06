#include <stdio.h>
#include <unistd.h>
#include <time.h>

/*
 * The original program reads its own source file (prog.c) and only plays the
 * animation if the Fletcher-16 checksum of that file equals this value.  The
 * same value is also used as the base delay of the pause before the animation.
 */
#define SOURCE_FILE "prog.c"
#define EXPECTED_CHECKSUM 41456

/* approximately pi / 180 (kept bit-exact with the original constant) */
#define DEGREES_TO_RADIANS 0x1.1df46a226e211p-6
/* terminal cells are taller than wide: rows are divided by about 13/7 */
#define ROW_SCALE_APPROX 1.8571428571

#define CENTER_COLUMN 66
#define CENTER_ROW 14
#define TAYLOR_TERMS 8

/* every star colour is a small offset from this blue-white base colour */
#define BASE_COLOR (5 * 2172211)	/* 0xA5B9FF */

struct star {
    const char *name;		/* exactly 6 characters, padded with a tab */
    int x;			/* position relative to the rotation center */
    int y;
    int color;			/* 0xRRGGBB */
};

/* The Big Dipper, coloured by spectral class (O B A F G K M) */
static const struct star big_dipper[] = {
    { "Dubhe\t", 47, -11, BASE_COLOR + 5911774 },	/* 0xFFEEDD, K */
    { "Merak\t", 55, 10, BASE_COLOR + 1052160 },	/* 0xB5C7FF, A */
    { "Megrez", 11, 13, BASE_COLOR + 1709568 },		/* 0xBFCFFF, A */
    { "Alioth", -13, 17, BASE_COLOR + 1314816 },	/* 0xB9C9FF, A */
    { "Mizar\t", -32, 19, BASE_COLOR + 1446400 },	/* 0xBBCBFF, A */
    { "Phecda", 25, 26, BASE_COLOR + 1314816 },		/* 0xB9C9FF, A */
    { "Alkaid", -54, 37, BASE_COLOR + 0 },		/* 0xA5B9FF, B */
};
#define STAR_COUNT ((int)(sizeof(big_dipper) / sizeof(big_dipper[0])))

static const char easter_message[] = "Lets find some Easter eggs";

/*
 * Exponentiation by squaring.  Note that, exactly like the original, every
 * exponent greater than 2 gets one extra factor of base (even when the
 * exponent is even), so power(x, 4) is really x^5.
 */
static double power(double base, int exponent)
{
    double half;

    if (exponent == 0)
	return 1;
    if (exponent == 1)
	return base;
    half = power(base, exponent / 2);
    if (exponent > 2)
	return half * half * base;
    return half * half;
}

/* n! using int arithmetic (overflows for n > 12, as in the original) */
static int factorial(int n)
{
    if (n < 2)
	return 1;
    return n * factorial(n - 1);
}

/* convert a string of decimal digits to a number */
static int parse_number(char digits[])
{
    int i, value = 0;

    for (i = 0; digits[i]; i++)
	value = value * 10 + digits[i] - '0';
    return value;
}

/* cosine of an angle in degrees using a Taylor series */
static double cosine_degrees(int degrees)
{
    double sum = 0;
    int i;

    degrees %= 360;
    for (i = 0; i < TAYLOR_TERMS; i++)
	sum += power(-1, i) * power(DEGREES_TO_RADIANS * degrees, 2 * i) / factorial(2 * i);
    return sum;
}

/* Fletcher-16 checksum of the given buffer */
static int fletcher16(char *data, ssize_t length)
{
    unsigned short sum1 = 0, sum2 = 0;
    int i;

    for (i = 0; i < length; ++i) {
	sum1 = (sum1 + data[i]) % 255;
	sum2 = (sum2 + sum1) % 255;
    }
    return sum2 << 8 | sum1;
}

int main(int argc, char **argv)
{
    time_t now = time(NULL);
    struct tm today = *localtime(&now);
    int year = 1900 + today.tm_year;
    int century = year / 100;
    int exit_status = century;
    int endian_test = 1;
    int angle, i;
    FILE *source;
    char *contents = NULL;
    size_t capacity;
    ssize_t length = 1;

    /*
     * Date of Easter (Anonymous Gregorian algorithm from Nature, 1876).
     * Note: the standard algorithm uses year % 19 when computing epact; the
     * original program uses year % century instead.
     */
    int century_quarter = century / 4;
    int century_remainder = century % 4;
    int lunar_correction = (century - (century + 8) / 25 + 1) / 3;
    int epact = (19 * (year % century) + century - century_quarter
		 - lunar_correction + 15) % 30;
    int year_quarter = (year % 100) / 4;
    int year_remainder = (year % 100) % 4;
    int weekday = (32 + 2 * century_remainder + 2 * year_quarter - epact
		   - year_remainder) % 7;
    int moon_shift = ((year % 19) + 11 * epact + 22 * weekday) / 451;
    int month_and_day = epact + weekday - 7 * moon_shift + 114;
    int easter_month = month_and_day / 31;
    int easter_day = month_and_day % 31 + 1;

    source = fopen(SOURCE_FILE, "r");

    /* black background, clear screen, cursor home, hide cursor */
    printf("\x1b[40m\x1b[2J\x1b[;H\x1b[?25l");
    if (*(char *)&endian_test)
	printf("%s\n", "25th IOCCC!");

    if (source != NULL)
	length = getdelim(&contents, &capacity, '\0', source);

    if (source != NULL && fletcher16(contents, length) == EXPECTED_CHECKSUM) {
	printf("\x1b[1;12H  ");
	usleep(century * EXPECTED_CHECKSUM);
	fflush(stdout);
	exit_status = 0;

	/* rotate the constellation from 178 degrees down to -1 degrees */
	for (angle = 178; angle >= -1; angle--) {
	    printf("\x1b[40m\x1b[2J");
	    for (i = 0; i < STAR_COUNT; i++) {
		const struct star *s = &big_dipper[i];
		int column = s->x * cosine_degrees(angle)
		    - s->y * cosine_degrees(90 - angle) + CENTER_COLUMN;
		int row = (s->x * cosine_degrees(90 - angle)
			   + s->y * cosine_degrees(angle) + CENTER_ROW) / ROW_SCALE_APPROX;

		if (row > 0 && column > 0) {
		    printf("\x1b[38;2;%d;%d;%dm", (s->color >> 16) & 255,
			   (s->color >> 8) & 255, s->color & 255);
		    /* far away stars are dots, near ones get a name */
		    printf("\x1b[%d;%dH%c %s", row, column,
			   angle > 90 ? '.' : 'o', angle > 45 ? "" : s->name);
		}
	    }
	    usleep(factorial(8));
	    fflush(stdout);
	}
    }

    /* cursor home, reset colours, show cursor */
    printf("\x1b[H\x1b[0m\x1b[?25h");
    if ((easter_month ^ (today.tm_mon + 1)) == easter_day - today.tm_mday)
	printf("%s\n", easter_message);
    /* year the program was compiled */
    printf("%d\n", parse_number(__DATE__ + 7));
    return exit_status;
}
