#include <stdio.h>

/*
 * A self-reproducing (quine) analog clock.
 *
 * The program prints its own source code laid out on an 80 column screen,
 * leaving a large oval comment in the middle.  Inside that oval it draws a
 * mirrored clock face showing the time the program was compiled (__TIME__).
 */

#define SCREEN_WIDTH   80
#define SCREEN_CELLS   1839         /* 23 rows of 80 columns, minus the last newline */
#define SCREEN_SIZE    (9 << 9)

#define CENTER_COLUMN  38           /* column used for the oval test */
#define CENTER_ROW     11           /* row used for the oval test */
#define LAST_COLUMN    41           /* column 79 (38 + 41) holds the newline */
#define OVAL_SIZE      750          /* col^2 + (2*row)^2 < 750 is inside the oval */

#define FACE_RADIUS    20           /* where the hour numbers are written */
#define HOUR_HAND      10           /* length of the hour hand */
#define MINUTE_HAND    17           /* length of the minute hand */

#define CLOCK_COLUMN   39.5         /* center of the clock face */
#define CLOCK_HALF_ROW 22.5         /* center in half rows (two dots per character) */
#define SECONDS_PER_RADIAN 6875.5   /* 43200 seconds (12 hours) / (2 * pi) */
#define SERIES_TERMS   39

/*
 * The source text of the original program, broken into the three parts the
 * quine needs.  Spaces in it are ignored when it is laid out on the screen.
 *
 * header_parts: the string literals at the end of the original program, which
 *               together spell out its first two lines.
 * body:         the code that comes after "Q(" in the original program.
 */
static const char *header_parts[] = {
    "#define/**/Q(x",
    ",y)char*q=y#x\",",
    "\"#y\")\",*p,s[x;}",
    "/*IOCCC'20*/#inclu",
    "de<stdio.h>/*-Qlock-",
    "*/int(y),x,i,k,r;Q(",
};
#define HEADER_PARTS (sizeof header_parts / sizeof header_parts[0])

static const char *body =
    "9<<9];float(o)[03];"
    "void(P)(){*o=r<0?r:-r;o[1]=39.5;"
    "o[2]=22.5;for(k=0;++k<39;*o*=i"
    "/6875.5/(k%2?k:-k))y=o[1+k%2"
    "]+=*o;k=o[2];p=s+y+k/2*80;"
    "}int(main)(){for(p=s;+i<"
    "1839;*q>32?k=i++/80-11,y"
    "=(750>r*r+k*k*4)*4+y/2"
    ",*p++=r<41?y?\"0X+0X+!\""
    "[y-1]-1:+*q++:10:*q++)"
    "r=i%80-38;;;for(x=13,r"
    "=20;i=3600*--x,i;*p++="
    "\"OISEA2dC8e\"[x%10],*p+=x"
    "/10*41)P();r=10;;sscanf("
    "__TIME__,\"%d\"\":%d:%d\",&k,&"
    "x,&i);for(i+=(k*60+x)*60;18+"
    "r;*p=k%2?*p%2?+59:44:*p>39?59:"
    "39,i=!r--?i%3600*12:i)P();puts(s"
    ")";

/* characters used for the border of the oval, chosen by oval_state - 1 */
static const char border_chars[] = "/W*/W* ";

/* digits 0-9 as they look in a mirror */
static const char mirror_digits[] = "OISEA2dC8e";

static char program_text[4096];
static char screen[SCREEN_SIZE];
static char *cursor;

/* append a string to program_text */
static char *append(char *out, const char *str)
{
    while (*str)
        *out++ = *str++;
    return out;
}

/* append a string as a quoted C string literal (escaping " and \) */
static char *append_quoted(char *out, const char *str)
{
    *out++ = '"';
    for (; *str; str++) {
        if (*str == '"' || *str == '\\')
            *out++ = '\\';
        *out++ = *str;
    }
    *out++ = '"';
    return out;
}

/*
 * Build the full text of the original program:
 *   header + body + "," + header written as string literals + ")"
 */
static void build_program_text(void)
{
    char *out = program_text;
    size_t n;

    for (n = 0; n < HEADER_PARTS; n++)
        out = append(out, header_parts[n]);
    out = append(out, body);
    out = append(out, ",");
    for (n = 0; n < HEADER_PARTS; n++)
        out = append_quoted(out, header_parts[n]);
    out = append(out, ")");
    *out = '\0';
}

/*
 * Pour the program text into the screen buffer, skipping whitespace.  Cells
 * inside the oval are filled with spaces, and the edge of the oval gets
 * slash-star characters so the oval becomes a C comment.
 */
static void layout_text(void)
{
    const char *src = program_text;
    int position = 0;
    int oval_state = 0;  /* history bits: is this cell, and the ones before it, inside? */

    cursor = screen;
    while (position < SCREEN_CELLS) {
        int column = position % SCREEN_WIDTH - CENTER_COLUMN;

        if (*src > ' ') {
            int row = position / SCREEN_WIDTH - CENTER_ROW;
            int inside = OVAL_SIZE > column * column + row * row * 4;

            position++;
            oval_state = inside * 4 + oval_state / 2;

            if (column >= LAST_COLUMN)
                *cursor++ = '\n';
            else if (oval_state)
                *cursor++ = border_chars[oval_state - 1];
            else
                *cursor++ = *src++;
        } else {
            src++;   /* whitespace in the text is dropped */
        }
    }
}

/*
 * Move the cursor to the point `distance` away from the clock center at the
 * given angle (in seconds, 43200 seconds is a full turn).  The sine and cosine
 * are computed with their Taylor series.  The result is mirrored: the angle
 * goes counter clockwise.  Returns the vertical position in half rows.
 */
static int plot(int distance, int angle)
{
    float term = -distance;
    float column = CLOCK_COLUMN;
    float half_row = CLOCK_HALF_ROW;
    int k;

    for (k = 1; k < SERIES_TERMS; k++) {
        if (k % 2)
            half_row += term;   /* cosine terms: 1, -x^2/2!, x^4/4!, ... */
        else
            column += term;     /* sine terms:   x, -x^3/3!, x^5/5!, ... */
        term *= angle / SECONDS_PER_RADIAN / (k % 2 ? k : -k);
    }

    cursor = screen + (int)column + (int)half_row / 2 * SCREEN_WIDTH;
    return (int)half_row;
}

/*
 * Draw a dot of a clock hand.  Each character holds two dots: ' is the top
 * dot, , is the bottom dot and ; is both.
 */
static void draw_dot(int half_row)
{
    if (half_row % 2)
        *cursor = *cursor % 2 ? ';' : ',';     /* bottom: ' or ; becomes ; */
    else
        *cursor = *cursor > '\'' ? ';' : '\''; /* top: , or ; becomes ; */
}

int main(void)
{
    int hour, minute, second, seconds;
    int distance;

    build_program_text();
    layout_text();

    /* the numbers on the clock face, written mirrored */
    for (hour = 12; hour > 0; hour--) {
        plot(FACE_RADIUS, 3600 * hour);
        *cursor++ = mirror_digits[hour % 10];
        *cursor += hour / 10 * ('I' - ' ');    /* 10, 11, 12: add a mirrored 1 */
    }

    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
    seconds = (hour * 60 + minute) * 60 + second;

    /* hour hand: one turn every 12 hours */
    for (distance = HOUR_HAND; distance >= 0; distance--)
        draw_dot(plot(distance, seconds));

    /* minute hand: one turn every hour */
    for (distance = 1; distance <= MINUTE_HAND; distance++)
        draw_dot(plot(distance, seconds % 3600 * 12));

    puts(screen);
    return 0;
}
