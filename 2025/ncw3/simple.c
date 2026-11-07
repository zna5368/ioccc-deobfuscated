#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#define FRAMES        800     /* number of animation frames */
#define COLUMNS       120     /* characters per line */
#define ROWS          40      /* text lines; each holds two pixels */
#define MAX_ITER      100     /* Mandelbrot iteration limit */
#define FRAME_DELAY   16000   /* microseconds between frames */

/* point of the Mandelbrot set that the animation zooms into */
static const double center_x = -.74364388703715;
static const double center_y = .13182590420533;

/* animation state shared by the drawing routines */
static double step_x, step_y;   /* complex-plane offset of one column */
static double color_phase;      /* rotates both the view and the palette */

/*
 * Count how many iterations of z = z*z + c it takes for |z|^2 to exceed 4,
 * or MAX_ITER if the point never escapes.
 */
static double mandelbrot(double cx, double cy)
{
    double zx = 0, zy = 0;
    double count = MAX_ITER;
    double iter;

    for (iter = 0; iter < MAX_ITER; iter++) {
        double zx2 = zx * zx;
        double zy2 = zy * zy;
        double new_zx, new_zy;

        if (4 < zx2 + zy2) {
            count = iter;
            break;
        }
        new_zx = zx2 - zy2 + cx;
        new_zy = 2 * zx * zy + cy;
        zy = new_zy;
        zx = new_zx;
    }
    return count;
}

/* one colour channel: a cosine wave mapped onto the range 1..255 */
static int channel(double angle)
{
    return (int)trunc(cos(angle) * 127 + 128);
}

/* map an iteration count onto an RGB colour, cycling with color_phase */
static void palette(double count, int rgb[3])
{
    double angle = count * .06283 + color_phase;

    rgb[0] = channel(angle + 4);
    rgb[1] = channel(angle + 2);
    rgb[2] = channel(angle);
}

/*
 * Print one character cell: the upper half block in the top colour
 * on a background of the bottom colour, giving two pixels per cell.
 */
static void print_cell(const int top[3], const int bottom[3])
{
    printf("\033[38;2;%d;%d;%dm", top[0], top[1], top[2]);
    printf("\033[48;2;%d;%d;%dm", bottom[0], bottom[1], bottom[2]);
    fputs("▀\033[0m", stdout);
}

/* draw one frame of COLUMNS x (2 * ROWS) pixels */
static void draw_frame(void)
{
    /* the "down" direction is the column step rotated by 90 degrees */
    double row_x = center_x - 60 * step_x + 40 * step_y;
    double row_y = center_y - 40 * step_x - 60 * step_y;
    int row, col;

    for (row = 0; row < ROWS; row++) {
        double x = row_x, y = row_y;

        for (col = 0; col < COLUMNS; col++) {
            int top[3], bottom[3];
            double top_count = mandelbrot(x, y);
            double bottom_count = mandelbrot(x - step_y, y + step_x);

            palette(bottom_count, bottom);
            palette(top_count, top);
            print_cell(top, bottom);

            x = x + step_x;
            y = y + step_y;
        }

        /* move down two pixels to the next text line */
        row_x = row_x - step_y;
        row_y = row_y + step_x;
        row_x = row_x - step_y;
        row_y = row_y + step_x;
        putchar('\n');
    }
}

int main(void)
{
    double scale = 5;
    int frame;

    fputs("\033[2J\033[H", stdout);    /* clear screen, cursor home */

    for (frame = 0; frame < FRAMES; frame++) {
        step_x = scale * cos(color_phase);
        step_y = scale * sin(color_phase);

        fputs("\033[H\033[?25l", stdout);  /* cursor home, hide cursor */
        draw_frame();
        fputs("\033[?25h", stdout);        /* show cursor */

        /* zoom in for the first half, then back out */
        scale = scale * (.97 + (400 < frame) * .06);

        usleep(FRAME_DELAY);
        color_phase = color_phase + .05;
    }
    exit(0);
}
