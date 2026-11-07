#include <stdio.h>

/*
 * The text is stored in a flat grid of cells, one row of STRIDE cells per
 * input line.  Input lines must be shorter than STRIDE characters and there
 * must be fewer than STRIDE lines.
 *
 * Each cell holds:  lit_character * STRIDE * STRIDE + glow_amount
 * so dividing by STRIDE gives a brightness level and dividing by
 * STRIDE * STRIDE recovers the character that was lit.
 */
#define STRIDE      126
#define CHAR_SCALE  (STRIDE * STRIDE)

/* the glow spreads 6 columns left/right and 3 rows up/down */
#define GLOW_DX     6
#define GLOW_DY     3
#define GLOW_POWER  1890        /* glow added = GLOW_POWER / distance^2 */
#define GLOW_MAX_COLUMN 121     /* no glow is drawn for columns >= this */

/* 256-colour background, black foreground, the character, then reset */
static const char ansi_cell[] = "\033[48;5;%dm\033[30m%c\033[0m";

/*
 * Room for the text plus a margin of GLOW_DY rows above and GLOW_DX
 * columns to the left so the glow never writes before the array.
 */
static int grid[7 << 14];
static int *const first_row = grid + GLOW_DY * STRIDE + GLOW_DX;

static int clamp(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

/* Map a brightness level to an xterm 256-colour cube index (16..231). */
static int glow_colour(int brightness)
{
    int red   = clamp(brightness,      0, 5);
    int green = clamp(brightness - 5,  0, 5);
    int blue  = clamp(brightness - 10, 0, 5);

    return 16 + 36 * red + 6 * green + blue;
}

int main(int argc, char **argv)
{
    const char *targets = argv[1];
    int *row = first_row;      /* start of the row being filled */
    int column = 0;            /* column of the current character */
    int width = 0;             /* length of the longest line */
    int ch, i, dx, dy;
    int *cell;

    (void)argc;

    while ((ch = getchar()) > 0) {
        /* is this one of the characters we were asked to light up? */
        int lit = 0;
        for (i = 0; targets[i]; i++) {
            if (targets[i] == ch) {
                lit = ch;
                break;
            }
        }

        if (ch < ' ') {         /* newline: move to the next row */
            row += STRIDE;
            column = -1;
        }

        if (lit && column < GLOW_MAX_COLUMN) {
            for (dy = -GLOW_DY; dy <= GLOW_DY; dy++) {
                for (dx = -GLOW_DX; dx <= GLOW_DX; dx++) {
                    int *target = &row[column + dx + dy * STRIDE];

                    if (dx || dy)
                        /* rows are about twice as tall as columns are wide */
                        *target += GLOW_POWER / (4 * dy * dy + dx * dx);
                    else
                        *target += lit * CHAR_SCALE;
                }
            }
        }

        column++;
        if (column > width)
            width = column;
    }

    /* print every row that was ended with a newline */
    for (cell = first_row; cell < row; cell += STRIDE) {
        for (i = 0; i < width; i++) {
            int brightness = cell[i] / STRIDE;
            int shown = brightness > STRIDE ? brightness / STRIDE : ' ';

            printf(ansi_cell, glow_colour(brightness), shown);
        }
        putchar('\n');
    }

    return 0;
}
