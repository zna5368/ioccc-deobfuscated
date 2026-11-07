#include <time.h>
#include <stdio.h>

/* average length of a lunar (synodic) month in seconds: 29.53 days */
#define SYNODIC_MONTH 2551443

/* a known new moon: 1970-01-07 20:35:31 UTC, in seconds since the epoch */
#define NEW_MOON_EPOCH 592531

/* radius of the moon disc in character cells */
#define RADIUS 44

/* number of phase steps in one month (two halves of 256 steps) */
#define PHASE_STEPS 512

int main(void)
{
    /*
     * phase: 0..511 position within the current lunar month
     *   0..255   waxing (new moon -> full moon)
     *   256..511 waning (full moon -> new moon)
     */
    int phase = (int)(((time(0) - NEW_MOON_EPOCH) % SYNODIC_MONTH * PHASE_STEPS)
                      / SYNODIC_MONTH);
    int waning = phase >> 8;           /* 0 when waxing, 1 when waning */
    int dark_part = ~phase & 255;      /* 255..0 within the current half */
    int y;

    /* rows are 4 units apart, columns 2 units apart, to look round on a terminal */
    for (y = 2 - RADIUS; y <= RADIUS; y += 4) {
        int x = 1 - RADIUS;
        int width, column;

        /* print left margin until the first column inside the circle */
        for (;;) {
            putchar(' ');
            if (x * x + y * y < RADIUS * RADIUS)
                break;
            x += 2;
        }

        /* row width covers the circle from x to -x in steps of 2 */
        width = 1 - x;

        for (column = 0; column < width; column++) {
            int in_shadow_part = column < (width * dark_part) >> 8;
            putchar("#."[in_shadow_part ^ waning]);
        }
        putchar('\n');
    }
    return 0;
}
