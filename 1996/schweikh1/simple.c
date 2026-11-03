#define _POSIX_SOURCE
#include <time.h>
#include <stdio.h>
#include <errno.h>

#define FIRST_YEAR 1582
#define LAST_YEAR 2199

/*
 * Print the date of Easter Sunday for every year from 1582 to 2199 using
 * Gauss's Easter algorithm.
 */
int main(void)
{
    int year;
    int cycle19, cycle4, cycle7;  /* a, b, c in Gauss's algorithm */
    int moon, weekday_shift;      /* d, e in Gauss's algorithm */
    int day, month;
    int century_m, century_n;     /* M, N in Gauss's algorithm */
    char text[16];
    /* tm_wday stays 0, so strftime always prints "Sun" */
    struct tm date = {0};

    for (year = FIRST_YEAR; year <= LAST_YEAR; ++year) {
        if (year < 1700) {
            century_m = 22;
            century_n = 2;
        } else if (year < 1800) {
            century_m = 23;
            century_n = 3;
        } else if (year < 1900) {
            century_m = 23;
            century_n = 4;
        } else if (year < 2100) {
            century_m = 24;
            century_n = 5;
        } else {
            century_m = 24;
            century_n = 6;
        }

        cycle19 = year % 19;
        cycle4 = year % 4;
        cycle7 = year % 7;
        moon = (19 * cycle19 + century_m) % 30;
        weekday_shift = (2 * cycle4 + 4 * cycle7 + 6 * moon + century_n) % 7;

        if (22 + moon + weekday_shift > 31) {
            month = 4;
            day = moon + weekday_shift - 9;
            if (day == 26)
                day = 19;
            else if (day == 25 && moon == 28 && cycle19 > 10)
                day = 18;
        } else {
            month = 3;
            day = 22 + moon + weekday_shift;
        }

        date.tm_year = year - 1900;
        date.tm_mon = month - 1;
        date.tm_mday = day;
        strftime(text, sizeof text, "%a %b %d %Y", &date);
        puts(text);
    }
    return 0;
}

/* definition matching the "extern int errno;" in the local errno.h */
int errno;
