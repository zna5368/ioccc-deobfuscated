#include <stdio.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#define WINDOW_SIZE 400
#define MARKER_SIZE 8
#define FULL_CIRCLE (360 * 64)		/* X11 arc angles are in 1/64 degree */

#define DAYS_PER_YEAR 365.25
#define DAYS_PER_MONTH 30.6001		/* month length in the calendar formula */
#define DAYS_PER_CENTURY 36524.25	/* Gregorian: 400 years / 4 */

#define FIRST_GREGORIAN_DAY (-152384)	/* 1582/10/15 */
#define COMET_VISIBLE_DAYS 150
#define LOG_SCALE_BASE 32		/* 31 AU lands on the window edge */
#define KEPLER_TOLERANCE 1e-6

/*
 * Keplerian orbit of one body.  Angles are in radians, the mean anomaly is the
 * value at day 0 (2000/01/01) and the mean motion is in radians per day.
 */
struct body {
	const char *label;
	double semi_major_axis;		/* astronomical units */
	double eccentricity;
	double inclination;
	double mean_anomaly_at_epoch;
	double mean_motion;
	double argument_of_perihelion;
	double ascending_node;
	int perihelion_day;		/* comets only: 0 means always shown */
};

static const struct body bodies[] = {
	{ "Me", 0.38709841668605804, 0.20563660562038422, 0.12227068468928337,
	  3.0507298707962036, 0.071424789726734161, 0.50820672512054443,
	  0.84368547797203064, 0 },
	{ "Ve", 0.72332099080085754, 0.0067639898043125868, 0.059302367269992828,
	  0.87636736035346985, 0.027962449938058853, 0.96158811450004578,
	  1.3381895422935486, 0 },
	{ "Ea", 1.0000007748603821, 0.016731629148125648, -9.4851998255762737e-06,
	  -0.042989956215023994, 0.017202119342982769, 1.8857001662254333,
	  -0.089231770485639572, 0 },
	{ "Ma", 1.5237123966217041, 0.093365106731653214, 0.032320331782102585,
	  0.33770926296710968, 0.0091460999101400375, -1.2850973606109619,
	  0.86765918135643005, 0 },
	{ "Ju", 5.2024800777435303, 0.048535898327827454, 0.022665092721581459,
	  0.35011023283004761, 0.0014502099948003888, -1.5012950897216797,
	  1.7504400014877319, 0 },
	{ "Sa", 9.5414986610412598, 0.055508248507976532, 0.043532717972993851,
	  -0.74675041437149048, 0.00058398162946105003, -0.36265543103218079,
	  1.9833918809890747, 0 },
	{ "Ur", 19.187978744506836, 0.046857399865984917, 0.013491068035364151,
	  2.4743309020996094, 0.00020475429482758045, 1.718652606010437,
	  1.2908891439437866, 0 },
	{ "Ne", 30.069526672363281, 0.0089543899521231651, 0.030893290415406227,
	  4.4949436187744141, 0.00010439250399940647, -1.4853584170341492,
	  2.3001058101654053, 0 },
	/* Halley, shown around its perihelion of 1986/02/09 */
	{ "Ha", 17.943499565124512, 0.96729999780654907, 2.831589937210083,
	  1.1482547521591187, 0.00022632399486610666, 1.952269971370697,
	  1.0149799585342407, -5074 },
	/* Hale-Bopp, shown around its perihelion of 1997/04/01 */
	{ "Hb", 187.02499389648438, 0.99510997533798218, 1.5607999563217163,
	  0.0067594998981803656, 6.7255912199470913e-06, -4.0039598941802979,
	  4.9300498962402344, -1005 },
	/* A semi-major axis of 0 pins the Sun to the middle of the window.
	 * The original table holds this entry twice, so it is drawn twice. */
	{ "Su", 0, 0, 0, 0, 0, 0, 0, 0 },
	{ "Su", 0, 0, 0, 0, 0, 0, 0, 0 },
};

#define BODY_COUNT (int)(sizeof(bodies) / sizeof(bodies[0]))

/* Convert a calendar date to the number of days since 2000/01/01. */
static int date_to_day_number(int year, int month, int day)
{
	int gregorian_correction;

	/* January and February count as the end of the previous year */
	if (month < 3) {
		year--;
		month += 12;
	}
	month++;

	if (year < 1582 ||
	    (year == 1582 && (month < 11 || (month == 11 && day < 5)))) {
		gregorian_correction = 0;	/* Julian calendar */
	} else {
		gregorian_correction = 2 - year / 100 + year / 400;
	}

	return (int)(DAYS_PER_YEAR * year) + (int)(DAYS_PER_MONTH * month) +
		day - 730550 + gregorian_correction;
}

/*
 * Convert a number of days since 2000/01/01 back to a calendar date.  The
 * julian_offset days are added to dates from before the Gregorian calendar.
 */
static void day_number_to_date(int day_number, int julian_offset,
			       int *year, int *month, int *day)
{
	double shifted_days = day_number + 36525;
	double years_since_1900, day_of_year;
	int whole_days;

	if (day_number >= FIRST_GREGORIAN_DAY) {
		double centuries = floor(day_number / DAYS_PER_CENTURY +
					 1.9983778) + 14;
		shifted_days += 1 + centuries - floor(centuries / 4);
	} else {
		shifted_days += julian_offset;
	}
	whole_days = (int)shifted_days;

	years_since_1900 = floor(whole_days / DAYS_PER_YEAR + .802601);
	day_of_year = whole_days -
		floor(DAYS_PER_YEAR * years_since_1900 + .750001) + 416;
	*month = (int)floor(day_of_year / DAYS_PER_MONTH);
	*day = (int)(day_of_year - floor(DAYS_PER_MONTH * *month));
	*year = (int)(years_since_1900 + 1900);

	if (*month > 13) {
		*month -= 13;
	} else {
		*month -= 1;
		*year -= 1;
	}
}

/* Solve Kepler's equation with Newton's method. */
static double eccentric_anomaly(double mean_anomaly, double eccentricity)
{
	double anomaly = mean_anomaly + eccentricity * sin(mean_anomaly);
	double correction;

	do {
		correction = (mean_anomaly - anomaly +
			      eccentricity * sin(anomaly)) /
			     (1 - eccentricity * cos(anomaly));
		anomaly += correction;
	} while (fabs(correction) > KEPLER_TOLERANCE);

	return anomaly;
}

/*
 * Find where a body appears on a day: radius is its distance from the Sun in
 * pixels (log scale) and angle is its direction as seen from the north of the
 * ecliptic.
 */
static void locate_body(const struct body *orbit, int day_number,
			int center_x, int center_y,
			double *radius, double *angle)
{
	double e = orbit->eccentricity;
	double mean_anomaly = orbit->mean_anomaly_at_epoch +
		orbit->mean_motion * day_number;
	double anomaly = eccentric_anomaly(mean_anomaly, e);
	double true_anomaly = 2 * atan(sqrt((1 + e) / (1 - e)) *
				       tan(anomaly / 2));
	double distance = orbit->semi_major_axis * (1 - e * e) /
		(1 + e * cos(true_anomaly));
	double orbit_angle = true_anomaly + orbit->argument_of_perihelion;
	double latitude = asin(sin(orbit_angle) * sin(orbit->inclination));

	*radius = log1p(distance) *
		(fmin(center_x, center_y) * cos(latitude) /
		 log(LOG_SCALE_BASE));
	*angle = atan2(sin(orbit_angle) * cos(orbit->inclination),
		       cos(orbit_angle)) + orbit->ascending_node;
}

static int is_visible(const struct body *orbit, int day_number)
{
	return orbit->perihelion_day == 0 ||
		fabs(day_number - orbit->perihelion_day) <= COMET_VISIBLE_DAYS;
}

static void draw_date(Display *display, Window window, GC gc,
		      int day_number, int julian_offset)
{
	char text[28];
	int year, month, day;

	day_number_to_date(day_number, julian_offset, &year, &month, &day);
	XDrawString(display, window, gc, 0, 10, text,
		    sprintf(text, "%04d/%02d/%02d", year, month, day));
}

static void draw_bodies(Display *display, Window window, GC gc,
			const XWindowAttributes *attributes, int day_number)
{
	int i;

	for (i = 0; i < BODY_COUNT; i++) {
		const struct body *orbit = &bodies[i];
		int center_x = attributes->width / 2;
		int center_y = attributes->height / 2;
		double radius, angle;

		locate_body(orbit, day_number, center_x, center_y,
			    &radius, &angle);

		if (is_visible(orbit, day_number)) {
			int x = center_x;
			int y = center_y;

			x += radius * cos(angle) - MARKER_SIZE / 2;
			y -= radius * sin(angle) + MARKER_SIZE / 2;
			XDrawArc(display, window, gc, x, y,
				 MARKER_SIZE, MARKER_SIZE, 0, FULL_CIRCLE);
			XDrawString(display, window, gc, x + 7, y + 1,
				    orbit->label, 2);
		}
	}
}

/* Number of days that a key moves the date. */
static int days_for_key(XKeyEvent *key)
{
	switch (XLookupKeysym(key, 0)) {
	case XK_Left:
		return -1;
	case XK_Right:
		return 1;
	case XK_Up:
		return 20;
	case XK_Down:
		return -20;
	default:
		return 0;
	}
}

int main()
{
	Display *display;
	Window window;
	GC gc;
	XWindowAttributes attributes;
	XEvent event;
	int screen;
	int year, month, day;
	int day_number;
	int julian_offset;

	display = XOpenDisplay(0);
	screen = DefaultScreen(display);
	window = RootWindow(display, screen);
	gc = XCreateGC(display, window, 0, 0);
	window = XCreateSimpleWindow(display, window, 0, 0,
				     WINDOW_SIZE, WINDOW_SIZE, 0,
				     BlackPixel(display, screen),
				     WhitePixel(display, screen));
	XMapWindow(display, window);
	XSelectInput(display, window, KeyPressMask | ExposureMask);

	scanf("%d/%d/%d", &year, &month, &day);
	day_number = date_to_day_number(year, month, day);

	/* The very first frame of a pre-Gregorian date shows the date moved
	 * forward by the day of the month that was typed in.  Later frames
	 * show the right date. */
	julian_offset = day;

	for (;;) {
		if (XGetWindowAttributes(display, window, &attributes)) {
			XClearWindow(display, window);
		}
		draw_date(display, window, gc, day_number, julian_offset);
		draw_bodies(display, window, gc, &attributes, day_number);

		XNextEvent(display, &event);
		if (event.type == KeyPress) {
			day_number += days_for_key(&event.xkey);
		}
		julian_offset = 0;
	}

	return 0;
}
