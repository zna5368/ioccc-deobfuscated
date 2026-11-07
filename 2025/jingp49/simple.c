#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FRAMES      280     /* length of the whole title sequence */
#define FRAME_USEC  90000   /* delay between frames */
#define BRIGHTEST   11      /* highest index into the brightness map */
#define ZOOM        0.878f  /* how much of the old picture the "camera" sees */
#define LOGO_SHRINK 0.898   /* logo scale factor applied every 5th frame */

/* ASCII brightness map, from dark (index 0) to bright (index 11) */
char shade[BRIGHTEST + 1] = {' ', ' ', ',', ':', ';', '!', '?', '{', '$', '%', '#', '@'};

/* 24x15 "DOCTOR WHO" logo */
#define LOGO_W 24
#define LOGO_H 15
static const char *logo[LOGO_H] = {
    "##..###.###.###.###.###.",
    "#.#.#.#.#....#..#.#.#.#.",
    "#.#.#.#.#....#..#.#.###.",
    "#.#.#.#.#....#..#.#.##..",
    "##..###.###..#..###.#.#.",
    "........................",
    "##.#.##.##...##..#####..",
    "##.#.##.##...##.#######.",
    "##.#.##.##...##.##...##.",
    "##.#.##.#######.##...##.",
    "##.#.##.#######.##...##.",
    "##.#.##.##...##.##...##.",
    ".#####..##...##.##...##.",
    ".#####..##...##.#######.",
    ".##.##..##...##..#####..",
};

/* 1 if the logo pixel is lit, 0 otherwise (rows past the bitmap are empty) */
static int logo_pixel(int row, int col)
{
    if (row >= LOGO_H || col >= LOGO_W)
        return 0;
    return logo[row][col] == '#';
}

/*
 * "Point the camera at the monitor": read the picture at a fractional
 * position and average the 2x2 block of cells around it.
 */
static unsigned char zoom_sample(const unsigned char *src, int width, float row, float col)
{
    return (src[(int)row * width + (int)col] +
            src[(int)(row + 1) * width + (int)col] +
            src[(int)row * width + (int)(col + 1)] +
            src[(int)(row + 1) * width + (int)(col + 1)]) >> 2;
}

/* randomly nudge a brightness level up or down to give the picture life */
static int flicker(int level)
{
    int r = rand();

    if (level == 0) {
        level++;                /* black always brightens */
    } else if (level == BRIGHTEST) {
        if (r % 10000 < 11)
            level--;
    } else {
        if (r % 400 < 7)
            level++;
        else if (r % 400 > 393)
            level--;
    }
    return level;
}

/* cross fade between two brightness levels */
static char blend(int from, int to, float fade)
{
    return shade[(int)(from * (1 - fade) + to * fade)];
}

int main(int argc, char **argv)
{
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int size = sizeof(unsigned char) * width * height;
    int seed;                   /* deliberately left uninitialized */
    int frame, i, j, p, mirror;
    int logo_top, logo_left, zoom_top, zoom_left;
    float fade = 0.0f;

    unsigned char *field_a = malloc(size);  /* first feedback picture */
    unsigned char *zoom_a = malloc(size);   /* field_a seen through the camera */
    unsigned char *field_b = malloc(size);  /* second feedback picture */
    unsigned char *zoom_b = malloc(size);   /* field_b seen through the camera */
    unsigned char *logo_now = malloc(size); /* logo currently shown */
    unsigned char *logo_next = malloc(size);/* logo after shrinking */

    srand(seed);

    /* start with two bright noisy pictures and no logo */
    for (i = 0; i < height; ++i) {
        for (j = 0; j < width; ++j) {
            p = i * width + j;
            field_a[p] = 9 + rand() % 3;
            field_b[p] = 9 + rand() % 3;
            logo_now[p] = 0;
            logo_next[p] = 0;
        }
    }

    /* scale the logo to fill the middle half of the screen */
    logo_top = height / 4;
    logo_left = width / 4;
    for (i = 0; i < height - logo_top * 2; ++i) {
        for (j = 0; j < width - logo_left * 2 - 1; ++j) {
            p = (i + logo_top) * width + j + logo_left;
            logo_now[p] = logo_pixel(i * 30 / height, j * 48 / width) * BRIGHTEST;
            logo_next[p] = logo_now[p];
        }
    }

    /* margins so the zoomed picture stays centred */
    zoom_left = (unsigned char)((width * (1 - ZOOM)) / 2);
    zoom_top = (unsigned char)((height * (1 - ZOOM)) / 2);

    for (frame = 0; frame < FRAMES; ++frame) {
        printf("\x1b[H");       /* cursor to top left */

        /* pass 1: film the current pictures (and shrink the logo) */
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; ++j) {
                p = i * width + j;
                if (frame < 110 || (frame > 120 && frame < 170))
                    zoom_a[p] = zoom_sample(field_a, width,
                                            (float)i * ZOOM + 2 * zoom_top,
                                            (float)j * ZOOM + zoom_left);
                if ((frame > 70 && frame < 140) || (frame > 170 && frame < 280))
                    zoom_b[p] = zoom_sample(field_b, width,
                                            (float)i * ZOOM + zoom_top,
                                            (float)j * ZOOM + zoom_left);
                if (frame > 175 && frame < 280 && frame % 5 == 0)
                    logo_next[(int)(i * LOGO_SHRINK + zoom_top) * width +
                              (int)(j * LOGO_SHRINK + zoom_left)] = logo_now[p];
            }
        }

        /* pass 2: feed the film back to the pictures and draw the frame */
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; ++j) {
                p = i * width + j;
                mirror = i * width + width - j;

                if (frame < 110 || (frame > 120 && frame < 170))
                    field_a[p] = (unsigned char)flicker(zoom_a[p]);
                if ((frame > 70 && frame < 140) || (frame > 170 && frame < 280))
                    field_b[p] = (unsigned char)flicker(zoom_b[p]);

                if (frame < 89) {
                    /* first tunnel */
                    putchar(shade[field_a[p]]);
                } else if (frame > 89 && frame < 120) {
                    /* fade into the second tunnel */
                    putchar(blend(field_a[p], field_b[p], fade));
                } else if (frame == 120) {
                    /* blank frame: refill the first picture */
                    field_a[p] = 7 + rand() % 3;
                } else if (frame > 120 && frame < 140) {
                    /* mirrored fade back into the first tunnel */
                    if (j < width / 2)
                        putchar(blend(field_b[p], field_a[p], fade));
                    else
                        putchar(blend(field_b[mirror], field_a[mirror], fade));
                } else if (frame > 140 && frame < 170) {
                    /* mirrored tunnel with the logo flashing in */
                    if (j < width / 2) {
                        if (logo_now[p]) {
                            field_a[p] = (unsigned char)(logo_now[p] * fade * (frame % 2 == 0));
                            putchar(shade[(int)(logo_now[p] * fade * 2)]);
                        } else {
                            putchar(shade[field_a[p]]);
                        }
                    } else {
                        if (logo_now[mirror]) {
                            field_a[p] = (unsigned char)(logo_now[p] * fade * ((frame & 1) == 0));
                            putchar(shade[(int)(logo_now[mirror] * fade * 2)]);
                        } else {
                            putchar(shade[field_a[mirror]]);
                        }
                    }
                } else if (frame == 170) {
                    /* blank frame: refill the second picture */
                    field_b[p] = 8 + rand() % 4;
                } else if (frame > 170 && frame < 280) {
                    /* shrinking logo over the fade into the second tunnel */
                    logo_now[p] = logo_next[p];
                    if (logo_now[p])
                        putchar(shade[logo_now[p]]);
                    else
                        putchar(blend(field_a[p], field_b[p], fade));
                }
            }
            printf("\n");
        }

        /* advance the cross fade for the current scene */
        if (frame > 89 && frame < 120) {
            if (fade < 1.0f) fade += 0.05f; else fade = 1.0f;
        } else if (frame == 120) {
            fade = 0.0f;
        } else if (frame > 120 && frame < 140) {
            if (fade < 1.0f) fade += 0.02502f; else fade = 1.0f;
        } else if (frame == 140) {
            fade = 0.0f;
        } else if (frame > 140 && frame < 170) {
            if (fade < 0.5f) fade += 0.0194f; else fade = 0.5f;
        } else if (frame == 170) {
            fade = 0.0f;
        } else if (frame > 170 && frame < 280) {
            if (fade < 1.0f) fade += 0.05f; else fade = 1.0f;
        }

        usleep(FRAME_USEC);
    }

    free(field_a);
    free(zoom_a);
    free(field_b);
    free(zoom_b);
    free(logo_now);
    free(logo_next);
    return 0;
}
