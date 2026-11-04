#include <stdio.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

static Display *display;
static Window window;
static GC gc;
static int screen_height, screen_width, horizontal_total;
static double pixel_clock, carrier_freq;

/*
 * Fill the screen with a pattern whose electromagnetic emissions form an
 * AM signal: a carrier at carrier_freq switched on and off at audio_freq.
 *
 * The electron beam visits one pixel per pixel clock tick and spends
 * horizontal_total ticks on each scan line (visible pixels plus blanking),
 * so `tick` counts time in pixel clock ticks since the top of the frame.
 */
static void draw_tone(int audio_freq)
{
    int tick = 0;
    int col, row;
    double audio_cycles_per_tick, carrier_cycles_per_tick;

    /* half cycles per pixel clock tick: floor(tick * rate) is even or odd */
    audio_cycles_per_tick = audio_freq / pixel_clock * 2.0;
    carrier_cycles_per_tick = carrier_freq / pixel_clock * 2.0;

    XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
    XFillRectangle(display, window, gc, 0, 0,
                   (unsigned int)screen_width, (unsigned int)screen_height);

    if (audio_freq == 0) {
        /* silence: (re)draw every pixel in black */
        for (col = 0; col < screen_width; col++)
            for (row = 0; row < screen_height; row++)
                XDrawPoint(display, window, gc, col, row);
        return;
    }

    XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
    for (row = 0; row < screen_height; row++) {
        col = 0;
        /* audio square wave is "on" for this scan line */
        if ((int)(tick * audio_cycles_per_tick) % 2 == 0) {
            for (; col < screen_width; col++) {
                /* carrier square wave is high at this pixel */
                if ((int)(tick * carrier_cycles_per_tick) % 2 == 1)
                    XDrawPoint(display, window, gc, col, row);
                tick++;
            }
        }
        /* skip the rest of the scan line, including horizontal blanking */
        tick += horizontal_total - col;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage:%spixelclockcarrier_freqhorizontal_total\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    pixel_clock = atof(argv[1]);
    carrier_freq = atof(argv[2]);
    horizontal_total = atoi(argv[3]);

    display = XOpenDisplay("");
    if (!display)
        exit(EXIT_FAILURE);

    /* full screen window that the window manager leaves alone */
    XSetWindowAttributes attributes;
    attributes.event_mask = ExposureMask | ButtonPressMask;
    attributes.override_redirect = True;
    screen_width = WidthOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
    screen_height = HeightOfScreen(ScreenOfDisplay(display, DefaultScreen(display)));
    window = XCreateWindow(display, RootWindow(display, DefaultScreen(display)),
                           0, 0, (unsigned int)screen_width,
                           (unsigned int)screen_height, 0, CopyFromParent,
                           InputOutput, (Visual *)CopyFromParent,
                           CWOverrideRedirect | CWEventMask, &attributes);
    XMapWindow(display, window);
    XRaiseWindow(display, window);
    gc = XCreateGC(display, window, 0, 0);

    /* audio frequency in Hz for each touch tone key; other keys are silent */
    static int key_tone[255];
    key_tone['1'] = 948;
    key_tone['2'] = 1011;
    key_tone['3'] = 1082;
    key_tone['4'] = 989;
    key_tone['5'] = 1053;
    key_tone['6'] = 1123;
    key_tone['7'] = 1030;
    key_tone['8'] = 1094;
    key_tone['9'] = 1164;
    key_tone['*'] = 1075;
    key_tone['0'] = 1138;
    key_tone['#'] = 1209;

    /* play each key for half a second */
    struct timespec note_length = {0, 500000000};
    int key;
    while ((key = getchar()) != EOF) {
        draw_tone(key_tone[key]);
        nanosleep(&note_length, NULL);
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
    exit(EXIT_SUCCESS);
}
