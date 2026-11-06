#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/*
 * Reads a JSON array of objects like
 *   [ {"name": "Foo", "value": 1.5}, ... ]
 * and writes an SVG pie chart with a legend to standard output.
 *
 * The input is scanned one character at a time by a small state machine.
 * Each string value becomes a legend entry and each number value becomes a
 * pie slice.  When the input ends the slices are drawn one per loop pass,
 * and the program stops with an assertion failure once no values are left.
 *
 * Output from a part of the loop that is not selected is still "printed":
 * every literal character becomes a space and every number prints nothing.
 * This is why the output contains long runs of spaces.
 */

/* getchar() returns EOF (-1), which the original keeps as the byte 0xff */
#define END_OF_INPUT 0xff

#define CENTER 250
#define RADIUS 200
#define LEGEND_X 472
#define LABEL_X 500
#define LEGEND_STEP 28

enum state {
	OUTSIDE_VALUE,   /* waiting for a ':' */
	AFTER_COLON,     /* reading a value: number text or the opening '"' */
	IN_STRING,       /* inside a string value: copy it into the legend */
	DRAWING_PIE      /* input is over: one pie slice per pass */
};

enum char_class {
	CLASS_SPACE = 1,  /* any byte below '!' */
	CLASS_OTHER = 2,
	CLASS_COMMA = 3,
	CLASS_QUOTE = 4
};

/* Number text of every value, each one followed by '\t' or '\n' in turn */
static char *values;
static size_t values_length;
/* Which separator comes next: 0 means '\t' and 1 means '\n' */
static int separator_toggle;
/* Index of the next color (starts at 1) */
static int color_index;
/* Running sum of all values and the current pie angle */
static double total;
static double angle;

/* Print the text, or one space per character when the part is not active */
static void emit(int active, const char *text)
{
	for (; *text; text++)
		putchar(active ? *text : ' ');
}

/* Print a number, or nothing when the part is not active */
static void emit_number(int active, double number)
{
	if (active)
		printf("%f", fabs(number));
}

/* The original keeps total and angle as "%f" text, so round the same way */
static double keep_as_text(double number)
{
	char text[512];

	snprintf(text, sizeof text, "%f", number);
	return atof(text);
}

static void append_value_char(char ch)
{
	values = realloc(values, values_length + 2);
	values[values_length++] = ch;
	values[values_length] = '\0';
}

static char separator(int toggle)
{
	return toggle ? '\n' : '\t';
}

static void emit_fill(int active)
{
	emit(active, "FILL=RGB(");
	emit_number(active, fma(cos(color_index), 48, 48));
	emit(active, "%,0%,");
	emit_number(active, fma(sin(color_index), 48, 48));
	emit(active, "%) />");
	if (active)
		color_index++;
}

static void emit_point_on_circle(int active)
{
	emit_number(active, fma(RADIUS, sin(angle), CENTER));
	emit(active, " ");
	emit_number(active, fma(RADIUS, cos(angle), CENTER));
}

/* Draw one pie slice from the value at the start of values */
static void draw_slice(int active)
{
	double value = atof(values);

	emit(active, "<PATH D='M");
	emit_number(active, CENTER);
	emit(active, " ");
	emit_number(active, CENTER);
	emit(active, "L");
	emit_point_on_circle(active);
	if (active) {
		double fraction = exp2(log2(value) - log2(total));
		angle = keep_as_text(fma(fraction, 2 * acos(-1), angle));
	}
	emit(active, "A");
	emit_number(active, RADIUS);
	emit(active, " ");
	emit_number(active, RADIUS);
	emit(active, " 0 ");
	/* large arc flag: is the slice more than half of the pie? */
	emit(active, 2 * value - total >= 0 ? "1" : "0");
	emit(active, " 0 ");
	emit_point_on_circle(active);
	emit(active, "Z'");
	emit_fill(active);
	if (active) {
		/* skip to the next value and switch separators */
		values = strchr(values, separator(separator_toggle));
		/* the original crashes here (before flushing) if none is found */
		assert(values != NULL);
		separator_toggle = !separator_toggle;
	}
}

static void emit_legend_start(int active)
{
	emit(active, "<RECT WIDTH=");
	emit_number(active, 16);
	emit(active, " HEIGHT=");
	emit_number(active, 16);
	emit(active, " X=");
	emit_number(active, LEGEND_X);
	emit(active, " Y=");
	emit_number(active, LEGEND_STEP * color_index);
	emit(active, " ");
	emit_fill(active);
	emit(active, "<TEXT X=");
	emit_number(active, LABEL_X);
	emit(active, " Y=");
	emit_number(active, LEGEND_STEP * color_index - 16);
	emit(active, ">");
}

static enum char_class classify(unsigned char ch)
{
	if (ch == '"')
		return CLASS_QUOTE;
	if (ch == ',')
		return CLASS_COMMA;
	if (ch < '!')
		return CLASS_SPACE;
	return CLASS_OTHER;
}

int main()
{
	enum state state = OUTSIDE_VALUE;
	unsigned char ch;
	int code;

	emit(1, "<SVG WIDTH=");
	emit_number(1, 750);
	emit(1, " HEIGHT=");
	emit_number(1, 750);
	emit(1, ">");

	values = strdup("\n");
	values_length = 1;
	separator_toggle = 0;
	color_index = 1;
	total = 0;
	angle = 0;

	for (;;) {
		ch = getchar();
		code = 4 * state + classify(ch);

		/* code 13..16 */
		draw_slice(code >= 13);
		fflush(NULL);
		assert(strlen(values) >= (code >= 13 ? 2 : 1));

		/* code 12: closing quote of a string value ends the label */
		if (code == 12)
			state = OUTSIDE_VALUE;
		emit(code == 12, "</TEXT>");

		/* code 9..11: copy the string straight to the output */
		if (code >= 9 && code <= 11)
			write(STDOUT_FILENO, &ch, 1);

		/* code 8: opening quote of a string value starts a legend entry */
		if (code == 8)
			state = IN_STRING;
		emit_legend_start(code == 8);

		if (code == 7) {
			/* comma ends a number value */
			char start;

			state = OUTSIDE_VALUE;
			append_value_char(separator(separator_toggle));
			separator_toggle = !separator_toggle;
			start = separator(separator_toggle);
			total = keep_as_text(total + atof(strrchr(values, start)));
		} else if (code == 6) {
			/* part of a number value */
			append_value_char(ch);
		} else if (code <= 5) {
			if (ch == ':') {
				state = AFTER_COLON;
			} else if (ch == END_OF_INPUT) {
				separator_toggle = 0;
				state = DRAWING_PIE;
				color_index = 1;
			}
		}
	}
}
