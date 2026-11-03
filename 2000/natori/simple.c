#include <stdio.h>
#include <math.h>

/*
 * Draw the Moon as it currently looks from the northern hemisphere.
 *
 * The picture is an ellipse 43 characters wide and 23 lines tall (text
 * characters are about twice as tall as they are wide, so this looks round).
 * Each character inside the disc is lit ('#') or dark (' ') depending on
 * the current phase of the Moon.
 */

#define WIDTH 43              /* columns: x runs from +21 down to -21     */
#define HEIGHT 23             /* rows: row runs from -22 up to 0         */
#define HALF_WIDTH 21

#define NEW_MOON_EPOCH 607728 /* a known new moon, seconds after 1970-01-01 */
#define SYNODIC_MONTH 2551443 /* length of a lunar cycle in seconds (29.53 days) */
#define SECONDS_PER_RADIAN 405859. /* roughly SYNODIC_MONTH / (2 * pi) */

#define THREE_HALVES_PI 4.7   /* approximately 3*pi/2 */
#define HALF_PI 1.57          /* approximately pi/2   */

/* Angle of the Moon in its cycle, 0 (new moon) to about 2*pi. */
double moon_phase_angle(void)
{
    /* time() is deliberately left undeclared, as in the original,
       so its result is treated as an int */
    int seconds_into_cycle = (time(0) - NEW_MOON_EPOCH) % SYNODIC_MONTH;
    return seconds_into_cycle / SECONDS_PER_RADIAN;
}

/*
 * Decide whether the point at horizontal offset x on the given row is lit.
 *
 * 3 - 22*row - row*row equals 124 - (row + 11)^2, so its square root is
 * half the width of the disc (in units of half a column) on this row.
 * The ratio is the horizontal position scaled so the disc edge is at +-2.
 */
int is_lit(int x, int row)
{
    double ratio = x / sqrt(3 - row * 22 - row * row);

    if (ratio * ratio >= 4)
        return 0; /* outside the disc */

    /* acos maps the horizontal position to a longitude on the sphere; the
       point is lit when it is within a quarter turn of the sunlit side */
    return fabs(moon_phase_angle() - THREE_HALVES_PI + acos(ratio / 2)) < HALF_PI;
}

int main(int argc, char **argv, char **envp)
{
    int row, column;

    putchar('\n');
    for (row = -(HEIGHT - 1); row <= 0; row++) {
        for (column = 0; column < WIDTH; column++)
            putchar(" #"[is_lit(HALF_WIDTH - column, row)]);
        putchar('\n');
    }
    return '\n';
}
