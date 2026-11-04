#include <stdio.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#define WIDTH 400       /* window width */
#define HEIGHT 200      /* window height */
#define CENTER 200      /* x of the screen centre */
#define TRACK_LEN 10660 /* track steps in one lap */
#define VIEW 200        /* how many track steps ahead are drawn */
#define START_TIME 53300 /* 8'53"00: the initial lap time and best time */
#define CAR_ROW 163     /* bottom row of the player's car */
#define CAR_SCALE 190   /* size of the player's car (out of 200) */
#define MAX_SPEED 163   /* top speed in mph */
#define HORIZON 135     /* bottom row of the mountains */

/* colours as 0xRRGGBB (only the low three bytes are used) */
#define BLACK 0x000000
#define SKY 0x6464EC
#define SUN 0xCDCDA4
#define MOUNTAIN 0x8787EB
#define GRASS 0x00D034
#define WHITE 0xC8C8D1
#define RED 0xC80000
#define FLAKE 0xFFFFFF

/*
 * One big int array holds the track and the keyboard state:
 *   track_x[i]  sideways position of the road at step i
 *   track_y[i]  height of the road at step i
 *   key_down[k] 1 while the key with keysym k is held down
 * Past the end of the lap track_x runs into the start of track_y.
 */
#define track_x (world)
#define track_y (world + TRACK_LEN)
#define key_down (world + 65793)

Window window;
GC gc;
int opp_scale,   /* size of the nearest visible opponent car (0 = none) */
    lap_time,    /* time since the lap started, in 1/100 s */
    steer_vel,   /* how fast the car is turning */
    pen_x,       /* "pen" for draw_spans(): centre offset of the row, */
    pen_inner,   /* inner and outer distance of the two spans, */
    pen_outer,   /* */
    pen_row,     /* and the screen row */
    tick,        /* counts event polls, also holds the white colour */
    grip,        /* 4 in summer, 1 in winter */
    night,       /* 'd' option */
    winter,      /* 'a' option */
    fog,         /* distance used to fade colours */
    opp_x,       /* sideways screen position of the opponent car */
    pos,         /* player position on the track */
    speed,       /* player speed in mph */
    lap_start,   /* time when the lap started */
    car_row,     /* bottom row of the car being drawn */
    offset,      /* player position across the road */
    opp_phase,   /* moves the opponent cars along the track */
    best,        /* best lap time */
    opp_lane;    /* lane of the opponent cars */
int world[1 << 18];
int timer[4]; /* struct timeval for gettimeofday() and one for select() */
Display *display;
Pixmap pixmap;
char text[99];

static int min(int a, int b) { return a < b ? a : b; }

/*
 * Fade one colour byte with the distance fog: during the day colours
 * fade towards white (twice as fast in winter), at night towards black.
 */
static int shade(int color, int shift) {
  return (fog / grip * !night +
          (color >> shift & 255) * (200 + fog / (winter | (night - 2))) / 200)
         << shift;
}

/*
 * Draw two horizontal spans on pen_row, from pen_inner to pen_outer pixels
 * left and right of CENTER + pen_x (pen_inner = 0 gives one solid span).
 * Returns the time since lap_start in 1/100 s. (On a 64-bit system the
 * struct timeval fields are 8 bytes, so this really counts whole seconds.)
 */
static int draw_spans(int color) {
  XSetForeground(display, gc,
                 shade(color, 0) | shade(color, 8) | shade(color, 16) | 0);
  XDrawLine(display, pixmap, gc, CENTER + pen_x - pen_inner, pen_row,
            CENTER + pen_x - pen_outer, pen_row);
  XDrawLine(display, pixmap, gc, CENTER + pen_x + pen_inner, pen_row,
            CENTER + pen_x + pen_outer, pen_row);
  gettimeofday((struct timeval *)timer, 0);
  return 100 * timer[0] + timer[1] / TRACK_LEN - lap_start;
}

/* one row of a car: black tyres and body first, then the paint (windows,
   stripes and the outline are left black) */
static void draw_car_row(int scale, int row, int paint, int color) {
  int half = min(min(75, 100 - row) / 3, 95 - 2 * row);
  if (paint)
    half = min(half, 2 * row) - 1;
  pen_outer = half * scale / 200;
  if (row < 9)
    pen_inner = scale * 14 / 200; /* two tyres */
  else if (paint && (row - 31) / 5 * (row % 12) == 0)
    pen_inner = pen_outer - 1; /* rear window and stripes */
  else
    pen_inner = 0;
  pen_row = car_row - row * scale / 200;
  fog = 200 - pen_row;
  lap_time = draw_spans(paint ? color : BLACK);
}

static void draw_car(int scale, int x, int first_row, int tint) {
  /* the products wrap around, the wrapped values are the car colours */
  int row, color = (int)((unsigned)tick * tick) +
                   (int)((unsigned)tick * tint) / 65793;
  pen_x = x;
  for (row = first_row; row <= 41; row++)
    draw_car_row(scale, row, 0, color);
  for (row = 9; row <= 40; row++)
    draw_car_row(scale, row, 1, color);
}

int main(int argc, char **argv) {
  XEvent event;
  int shape = 4, segment = 0, height = 0, heading = 0, curve = 0;
  int steer, sun, mountain, mountain_x, slice, depth, stripe, road = 0;
  int slice_row = 0, seed, flake;

  /* the original main() is also the drawing routine: any other argument
     count makes it draw (without a display) and return */
  if (argc != 1 && argc != 2)
    return draw_spans(argc);

  /* options: ['a']['d'][track number] */
  if (argc == 2) {
    winter = argv[1][0] == 'a';
    night = argv[1][winter] == 'd';
    shape -= argv[1][winter + night] % 3;
  }

  /* build the track: hills in 200 step segments and a curvy first half
     that is mirrored so that the second half leads back to the start */
  for (tick = 1; tick < TRACK_LEN; tick++) {
    segment = tick / 200 % 6;
    if (segment == 2 || segment == 4)
      height += segment - 3;
    track_y[tick] = height / 4;
    if (2 * tick <= TRACK_LEN) {
      curve -= (curve != -34 && tick / 149 % shape) -
               (curve != 34 && tick / 353 % 3);
      heading += curve;
      track_x[tick] = heading / 55;
    } else
      track_x[tick] = track_x[TRACK_LEN - tick];
  }
  grip = 4 - 3 * winter;
  /* the original shares one variable for both; in the first frame the
     last segment type (5) is used as car_row, so hardly any sky is drawn */
  car_row = segment;

  display = XOpenDisplay(0);
  gc = XCreateGC(display,
                 window = XCreateSimpleWindow(display, RootWindow(display, 0),
                                              0, 0, WIDTH, HEIGHT, 0, 0, 0),
                 0, 0);
  XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
  XMapWindow(display, window);
  pixmap =
      XCreatePixmap(display, window, WIDTH, HEIGHT, DefaultDepth(display, 0));
  opp_phase = 200;
  /* starting five laps ahead makes the first frame start a new lap */
  pos = lap_time = best = START_TIME;

  for (;;) {
    timer[3] = 65793; /* select() timeout in microseconds (32-bit layout) */

    /* every key press or release flips the state of that key */
    for (++tick; XPending(display); ++tick) {
      XNextEvent(display, &event);
      key_down[XLookupKeysym(&event.xkey, 0)] ^=
          event.type == KeyPress || event.type == KeyRelease;
    }

    /* driving */
    steer = key_down[XK_Right] - key_down[XK_Left];
    steer_vel = steer ? steer_vel + grip * steer * (steer_vel * steer < 8)
                      : steer_vel * .9;
    speed = min(key_down[XK_space] * 3 + speed - !!speed, MAX_SPEED);
    pos += speed / 9;
    if (pos > TRACK_LEN) { /* lap completed */
      best = min(lap_time, best);
      pos %= TRACK_LEN;
      lap_start = 0;
      lap_start = draw_spans(BLACK); /* returns the current time */
    }
    offset += steer_vel * (speed + 100) / WIDTH -
              speed / 60 * (track_x[pos + 4] - track_x[pos]);
    if (offset * offset > TRACK_LEN) { /* off the road */
      offset *= .97;
      speed *= .8;
    } else if (opp_scale > CAR_ROW && opp_x * opp_x < 1800) /* crash */
      speed = 60;

    /* black dashboard area at the bottom, then the sky above car_row */
    pen_x = 0;
    pen_row = pen_outer = WIDTH;
    while (pen_row-- >= car_row)
      draw_spans(BLACK);
    for (fog = HORIZON; pen_row >= 0; pen_row--)
      draw_spans(SKY);

    /* the sun (just one dot at night) */
    mountain_x = pen_x = track_x[pos] / -2 - offset;
    sun = 17 * night;
    do {
      pen_row = sun++ + 20;
      pen_outer = sun + sun - sun * sun / 9;
      draw_spans(SUN);
    } while (sun < 17);

    /* mountains: each one is a pair of slopes going down to HORIZON,
       the first one continues from where the sun ended */
    for (mountain = 0; mountain != 5;) {
      if (pen_row++ == HORIZON) {
        pen_inner = pen_outer = 200;
        mountain_x -= 100;
        pen_row = 30 + 9 * mountain++;
      }
      pen_outer -= 2;
      pen_inner += 3;
      pen_x = mountain_x / .7;
      draw_spans(MOUNTAIN - mountain * sun);
    }

    /* the landscape, one track step at a time from far away to the car;
       a step that lies lower on the screen fills the rows down to it */
    depth = opp_scale = pen_inner = 0;
    car_row = slice = pos + VIEW; /* off screen unless an opponent is seen */
    opp_phase += 9;
    for (;;) {
      slice_row = depth / 200 + 70 + track_y[slice] - track_y[pos];
      if (!depth)
        pen_row = slice_row;
      stripe = 2 * slice / 41 % 2;
      pen_x = track_x[--slice] - track_x[pos] - offset;
      depth += 200 - (fog = slice - pos);
      if ((opp_phase - slice) % 410 == 0) { /* an opponent car is here */
        car_row = pen_row;
        opp_x = pen_x + opp_lane * depth / WIDTH;
        opp_scale = depth / 100;
      }
      if (slice <= pos)
        break;
      for (; pen_row < slice_row; pen_row++) {
        pen_outer = 2 * WIDTH; /* grass or snow */
        draw_spans(winter ? tick + stripe * 0x080808 : GRASS - stripe * 10000);
        pen_outer = 9 + depth / 200; /* kerb */
        draw_spans(!stripe + winter ? (tick = WHITE) : RED);
        pen_outer -= depth / 9 / 200; /* asphalt, white at the finish line */
        draw_spans(slice % TRACK_LEN > 6 ? (road = tick * (3 + winter) / 5)
                                         : tick);
        pen_outer = depth / 13 / WIDTH; /* dashed centre line */
        draw_spans(stripe ? road : tick);
      }
    }

    /* the nearest opponent car, then the player's car */
    draw_car(opp_scale, opp_x, 1, slice_row);
    car_row = CAR_ROW;
    draw_car(CAR_SCALE, 0, 0, 0);

    /* without a visible opponent choose a lane for the next one */
    if (!opp_scale)
      opp_lane = 1 - lap_time % 3;

    fog = pen_outer = pen_inner;
    draw_spans(RED); /* sets the text colour */
    XDrawString(display, pixmap, gc, 30, CAR_SCALE, text,
                sprintf(text,
                        "Time: %1d'%02d\"%02d          "
                        "High: %1d'%02d\"%02d          "
                        "Speed: %03dmph",
                        lap_time / 100 / 60, lap_time / 100 % 60,
                        lap_time % 100, best / 100 / 60, best / 100 % 60,
                        best % 100, speed));

    /* snow flakes at pseudo random places (the products wrap around) */
    seed = lap_time;
    for (flake = 1; flake < 2 * winter * WIDTH; flake++) {
      seed = (int)((unsigned)seed * 3);
      pen_x = seed % WIDTH;
      pen_row = (int)((unsigned)seed * seed) % CAR_ROW;
      draw_spans(FLAKE);
    }

    XCopyArea(display, pixmap, window, gc, 0, 0, WIDTH, HEIGHT, 0, 0);
    XSync(display, 0);
    XFlush(display);
    select(0, 0, 0, 0, (struct timeval *)timer + 1);
  }
}
