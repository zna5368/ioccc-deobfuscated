#include <X11/Xlib.h>
#include <math.h>
#include <time.h>

/*
 * Plot the four Galilean moons of Jupiter as seen from Earth, using the
 * low accuracy method from Jean Meeus, "Astronomical Algorithms" (1st ed),
 * chapter "Positions of the satellites of Jupiter".
 *
 * All angles are in radians.  The constants are the exact double values that
 * the original program decoded from its string table, so the plotted pixels
 * are identical.  The equivalent value in degrees is noted where useful.
 */

#define CENTER_X 399           /* pixel x of Jupiter's centre */
#define CENTER_Y 39            /* pixel y of Jupiter's centre */
#define SCALE 14               /* pixels per Jupiter radius */
#define LABEL_Y 20             /* baseline of the moon labels */
#define FULL_CIRCLE (360 * 64) /* X11 arcs are measured in 1/64 degree */

#define J2000 2451545                  /* Julian day of 2000-01-01 12:00 UT */
#define UNIX_EPOCH_SINCE_J2000 10957.5 /* days from 1970-01-01 to J2000 */

double atof();

Display *display;
Window window;
GC normal_gc, xor_gc;
Font font;

static const char *moon_label[4] = {"I", "II", "III", "IV"};
static const int moon_label_length[4] = {1, 2, 3, 2};

/* reduce an angle into the range (-2pi, 2pi) */
static double wrap(double angle) { return fmod(angle, 2 * M_PI); }

static void draw_moons(double d) {
  double sin_V, M, N, J, A, B, K, R, r, delta, psi;
  double lambda, DS, DE, t, u[4], radius[4], G, H, twice_12, twice_23;
  int i;

  /* long period term in Jupiter's motion: V = 172.74 + 0.00111588 d */
  sin_V = sin(3.0148817596768493 + 1.9479442030734262e-05 * d);

  /* mean anomaly of Earth: M = 357.529 + 0.9856003 d */
  M = wrap(6.2400584164234481 + 0.017201937539988776 * d);

  /* mean anomaly of Jupiter: N = 20.020 + 0.0830853 d + 0.329 sin V */
  N = (0.34941491407635938 + 0.0014500898693499131 * d) +
      0.0057421201501695052 * sin_V;

  /* difference of mean longitudes: J = 66.115 + 0.9025179 d - 0.329 sin V */
  J = wrap((1.1539244148330314 + 0.01575192480097698 * d) -
           0.0057421201501695052 * sin_V);

  /* equations of the centre of Earth (A) and Jupiter (B) */
  A = 0.00034906417925939515 * sin(M * 2) + 0.033423023390166007 * sin(M);
  B = 0.0029321726982405316 * sin(N * 2) + 0.096953038462852081 * sin(N);

  K = J + (A - B);

  /* radius vectors of Earth (R) and Jupiter (r) in AU */
  R = (1.0001399547308254 - 0.016710011814087489 * cos(M)) -
      0.00014000069244146651 * cos(M * 2);
  r = (5.208720027312733 - 0.2520800177902276 * cos(N)) -
      0.00611003196519877 * cos(N * 2);

  /* distance Earth-Jupiter */
  delta = sqrt((r * r + R * R) - r * (R * 2) * cos(K));

  /* phase angle of Jupiter as seen from Earth */
  psi = asin(sin(K) * (R / delta));

  /* heliocentric longitude of Jupiter: lambda = 34.35 + 0.083091 d + ... */
  lambda = 0.59952063872846595 +
           (0.0057421201501695052 * sin_V + (0.0014502199193517917 * d + B));

  /* planetocentric declination of the Sun (DS) and of Earth (DE) */
  DS = 0.054454289064088224 * sin(0.74700096685578643 + lambda);
  DE = DS -
       0.038746288357112579 * (cos(0.38397245424015414 + lambda) * sin(psi));
  DE = DE - 0.022689303291511198 *
                (sin(lambda - 1.7540558657503316) * ((r - delta) / delta));

  /* time corrected for the light travel time from Jupiter (c = 173 AU/day) */
  t = d - delta / 172.99999943195431;

  /* angles of the moons measured from the inferior conjunction */
  u[0] = wrap((2.8589661829601116 + 3.5501020321685361 * t) + (psi - B));
  u[1] = wrap((6.2554482914302758 + 1.7678725726035434 * t) + (psi - B));
  u[2] = wrap((0.099708906786158735 + 0.8767576794032419 * t) + (psi - B));
  u[3] = wrap((3.9237636474956945 + 0.37503601760093108 * t) + (psi - B));

  /* perturbation arguments */
  G = wrap(5.7801813482954856 + 0.87808359212787512 * t);
  H = wrap(1.5254178224377974 + 0.37645407359619965 * t);
  twice_12 = (u[0] - u[1]) * 2;
  twice_23 = (u[1] - u[2]) * 2;

  /* distances of the moons from Jupiter in Jupiter radii */
  radius[0] = 5.9072997841378845 - 0.024400005957583071 * cos(twice_12);
  radius[1] = 9.3990998327938833 - 0.082200006813512028 * cos(twice_23);
  radius[2] = 14.992399770664688 - 0.021600016281522638 * cos(G);
  radius[3] = 26.369900292537547 - 0.19350001899584246 * cos(H);

  /* corrections to the angles */
  u[0] = 0.0082554190502690528 * sin(twice_12) + u[0];
  u[1] = 0.018587748020044703 * sin(twice_23) + u[1];
  u[2] = 0.0028798165963688227 * sin(G) + u[2];
  u[3] = 0.01467819650456903 * sin(H) + u[3];

  for (i = 0; i < 4; i++) {
    double x = radius[i] * sin(u[i]);
    double y = sin(DE) * (radius[i] * cos(u[i]));
    /* a moon on the far side of Jupiter is drawn with XOR so it vanishes
       behind the planet's disc */
    int behind = cos(u[i]) < 0;
    XTextItem label;

    XDrawArc(display, window, behind ? xor_gc : normal_gc,
             (int)(CENTER_X + SCALE * x), (int)(CENTER_Y + SCALE * y), 2, 2, 0,
             FULL_CIRCLE);

    label.chars = (char *)moon_label[i];
    label.nchars = moon_label_length[i];
    label.delta = 0;
    label.font = font;
    XDrawText(display, window, normal_gc, (int)(CENTER_X + SCALE * x), LABEL_Y,
              &label, 1);
  }
}

int main(int argc, char **argv) {
  int screen;
  XGCValues values;
  double days_skipped = 0, day;

  screen = DefaultScreen(display = XOpenDisplay(0));
  XMapWindow(display, window = XCreateSimpleWindow(
                          display, RootWindow(display, screen), 0, 0, 800, 80,
                          0, 0, BlackPixel(display, screen)));

  values.foreground = WhitePixel(display, screen);
  values.function = GXxor;
  xor_gc = XCreateGC(display, window, GCFunction | GCForeground, &values);
  normal_gc = XCreateGC(display, window, GCForeground, &values);
  font = XLoadFont(display, "10x20");

  /* days since J2000, either given or taken from the clock, advanced by the
     time step in argv[1] every frame */
  while ((day = (argc > 2 ? atof(argv[2]) - J2000
                          /* 3.75 * 23040 = 86400 seconds per day */
                          : time(0) / 3.75 / 23040 - UNIX_EPOCH_SINCE_J2000) +
                (days_skipped += argc > 1 ? atof(argv[1]) : 0)) != 0) {
    XClearWindow(display, window);
    /* Jupiter */
    XFillArc(display, window, normal_gc, CENTER_X - 14, CENTER_Y - 14, 28, 28,
             0, FULL_CIRCLE);
    draw_moons(day);
    XSync(display, 0);
    usleep(10000);
  }
}
