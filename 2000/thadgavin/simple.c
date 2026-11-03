#include <math.h>

/*
 * Screen size in pixels.  The curses version shrinks this to fit the
 * terminal (one character per pixel).
 */
int screen_width = 320;
int screen_height = 200;

/* 256 palette entries of 4 bytes each: red, green, blue, unused */
#define PALETTE_COLORS 256
#define PALETTE_BYTES (PALETTE_COLORS * 4)

/* 44/7 is the author's approximation of 2*pi (one full turn) */
#define FULL_TURN (44.0 / 7)

/* Two palettes to fade between, and the mix of the two that gets shown */
unsigned char palette_a[PALETTE_BYTES];
unsigned char palette_b[PALETTE_BYTES];
unsigned char palette_mixed[PALETTE_BYTES];

/* Indexed-color pixels, one byte per pixel (used by DOS and curses) */
unsigned char framebuffer[320 * 200];
unsigned char *pixels = framebuffer;

/* For every pixel: its distance from the screen center and its angle */
double polar_coords[320 * 200 * 2];

/* Parameters that shape the pattern, changed a little every frame */
double ring_frequency = 0;  /* how many rings appear as distance grows */
double spin = 3;            /* rotates the spiral arms */
double wobble_amount = 0;   /* strength of the radial wobble */
double wobble_phase = 0;    /* moves the wobble outward over time */
double wobble_length = 8;   /* distance between wobble waves */

/* Cross-fade between palette_a (blend = 1) and palette_b (blend = 0) */
double blend = 1;
double blend_step = 11.0 / 630;

double elapsed = 0;

/* Seeds of the pseudo-random generator for each color channel */
double seed_red = 3, seed_green = 7, seed_blue = 13;

/* Turn a seed into the next pseudo-random value in [0, 1) */
double next_random(double seed)
{
    return (double)((int)(seed * 123456789) % 1024) / 1024;
}

/*
 * Fill a palette with a fresh random color ramp: entry i gets each channel
 * equal to (random brightness) * i / 3, so colors grow from black.
 */
void random_palette(unsigned char *palette)
{
    int i;

    seed_red = next_random(seed_red);
    seed_green = next_random(seed_green);
    seed_blue = next_random(seed_blue);

    for (i = 0; i < PALETTE_BYTES; i += 4) {
        palette[i + 0] = seed_red * (i + 0) / 12;
        palette[i + 1] = seed_green * (i + 1) / 12;
        palette[i + 2] = seed_blue * (i + 2) / 12;
    }
}

#ifdef __DJGPP__
#include <sys/movedata.h>
#include <dpmi.h>
#include <pc.h>

void init_screen(void)
{
    __dpmi_regs regs;

    /* BIOS video mode 0x13: 320x200 with 256 colors */
    regs.x.ax = 0x13;
    __dpmi_int(0x10, &regs);
}

void show_frame(unsigned char *palette)
{
    int color;

    /* wait for the end of a vertical retrace, then for the next one */
    while (inportb(0x3DA) & 8)
        ;
    while (!(inportb(0x3DA) & 8))
        ;

    /* load the VGA palette (its channels are only 6 bits, so divide by 4) */
    for (color = 0; color < PALETTE_COLORS; color++) {
        outportb(0x3C8, color);
        outportb(0x3C9, palette[color * 4 + 0] / 4);
        outportb(0x3C9, palette[color * 4 + 1] / 4);
        outportb(0x3C9, palette[color * 4 + 2] / 4);
    }

    /* copy the pixels into video memory */
    dosmemput(pixels, screen_width * screen_height, 0xA0000);
}

#elif defined(SDL)
#include <SDL.h>

SDL_Surface *surface;

void init_screen(void)
{
    SDL_Init(SDL_INIT_VIDEO);
    surface = SDL_SetVideoMode(screen_width, screen_height, 8, 0);
    pixels = surface->pixels;
}

void show_frame(unsigned char *palette)
{
    SDL_SetColors(surface, (SDL_Color *)palette, 0, PALETTE_COLORS);
    SDL_UpdateRect(surface, 0, 0, 0, 0);
}

#else
#include "curses.h"

void init_screen(void)
{
    int width, height;

    initscr();

    /* use an even number of columns and lines, at most 320x200 */
    width = COLS & ~1;
    height = LINES & ~1;
    if (width < screen_width)
        screen_width = width;
    if (height < screen_height)
        screen_height = height;
}

void show_frame(unsigned char *palette)
{
    /* curses has no palette, so draw each pixel as a brightness character */
    static const char shades[] = " .,:+=@#";
    int i;

    for (i = 0; i < screen_width * screen_height; i++) {
        move(i / screen_width, i % screen_width);
        addch(shades[pixels[i] / 32]);
    }
    refresh();
}
#endif

int main(int argc, char **argv)
{
    int x, y, i;
    double *polar;

    init_screen();

    /* precompute polar coordinates of every pixel relative to the center */
    polar = polar_coords;
    for (y = -screen_height / 2; y < screen_height / 2; y++) {
        for (x = -screen_width / 2; x < screen_width / 2; x++) {
            *polar++ = sqrt(x * x + y * y);
            *polar++ = atan2(x, y);
        }
    }

    for (;;) {
        /*
         * Move the cross-fade along.  When it reaches one end, bounce back
         * and replace the palette that is now invisible with a new one.
         */
        blend += blend_step;
        if (blend > 1) {
            blend = 2 - blend;
            blend_step = -blend_step;
            random_palette(palette_b);
        } else if (blend < 0) {
            blend = -blend;
            blend_step = -blend_step;
            random_palette(palette_a);
        }

        for (i = 0; i < PALETTE_BYTES; i++)
            palette_mixed[i] = blend * palette_a[i] + (1 - blend) * palette_b[i];

        /* draw the spiral: brightness is a sine wave around each pixel */
        polar = polar_coords;
        for (i = 0; i < screen_width * screen_height; i++) {
            double distance = *polar++;
            double angle = *polar++;

            pixels[i] = fabs(sin((angle + spin) * 1.5 + distance * ring_frequency +
                                 wobble_amount * sin(wobble_phase + distance / wobble_length)) *
                             255);
        }
        show_frame(palette_mixed);

        wobble_amount += blend_step;
        elapsed += FULL_TURN / 360;
        wobble_phase += elapsed;

        /* after one turn start spinning, after two start making rings */
        if (elapsed > FULL_TURN) {
            spin += (elapsed - FULL_TURN) / 8;

            if (elapsed > FULL_TURN * 2)
                ring_frequency = sin(elapsed) + sin(elapsed * 2) / 2;
        }
    }
}
