#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* the pointer velocity in pixels per update */
float velocity_x = 0, velocity_y = 0;
/* where the pointer was after the last update */
float last_x, last_y;
/* the velocity is divided by this every update */
float greasiness = 1.03;

int first_update = 1;
int max_x, max_y, screen;
Display *display;

/* round half away from zero */
static int round_away(float value)
{
	int sign = (value > 0) - (value < 0);
	return (int)(value + sign * .5);
}

static int update_pointer(void)
{
	int hit_x = 0, hit_y = 0, unused, x, y;
	Window window;

	XQueryPointer(display, XRootWindow(display, screen), &window, &window,
		      &x, &y, &unused, &unused, (unsigned int *)&unused);

	if (x >= max_x || x <= 0)
		hit_x = 1;
	if (y >= max_y || y <= 0)
		hit_y = 1;

	if (first_update) {
		last_x = x;
		last_y = y;
		first_update = 0;
	} else {
		/* if the user moved the mouse, take its movement as the new speed */
		if (!hit_x && x - last_x - round_away(velocity_x) != 0)
			velocity_x = x - last_x;
		if (!hit_y && y - last_y - round_away(velocity_y) != 0)
			velocity_y = y - last_y;

		/* friction */
		velocity_x /= greasiness;
		velocity_y /= greasiness;
	}

	/* bounce off the edges of the screen */
	if (x >= max_x || x <= 0)
		velocity_x = -velocity_x;
	if (y >= max_y || y <= 0)
		velocity_y = -velocity_y;

	last_x = x;
	last_y = y;

	/* move the pointer relative to where it is now */
	XWarpPointer(display, None, None, 0, 0, max_x, max_y,
		     round_away(velocity_x), round_away(velocity_y));
	return 0;
}

int main(int argc, char **argv)
{
	clock_t last_tick = 0;

	if (argc == 2)
		greasiness = atof(argv[1]);

	display = XOpenDisplay(0);
	if (display == 0)
		exit(1);

	screen = DefaultScreen(display);
	max_x = DisplayWidth(display, screen) - 1;
	max_y = DisplayHeight(display, screen) - 1;

	/* update the pointer every hundredth of a second of CPU time */
	for (;;) {
		if ((clock() - last_tick) * 100 > CLOCKS_PER_SEC) {
			update_pointer();
			last_tick = clock();
		}
	}
}
