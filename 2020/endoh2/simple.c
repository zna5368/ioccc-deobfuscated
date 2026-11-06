/* A long time ago in a galaxy far, far away.... */

#include <complex.h>
#include <stdio.h>
#include <time.h>

/* Terminal frame: 24 rows of 79 characters plus a newline */
#define SCREEN_WIDTH      80
#define SCREEN_HEIGHT     24
#define SCREEN_SIZE       (SCREEN_WIDTH * SCREEN_HEIGHT)

/* ANSI escape sequences */
#define CLEAR_SCREEN      "\x1b[2J"
#define CURSOR_HOME       "\x1b[H"

#define MAX_LETTERS       3333
#define LETTER_SPACING    16   /* horizontal distance between letters */
#define LINE_SPACING      25   /* distance between lines of text */
#define FOCAL_LENGTH      48   /* perspective projection scale */
#define START_DISTANCE    40   /* how far away the first line starts */
#define FRAMES_PER_SECOND 10

/* A 2D point: real part is x (left/right), imaginary part is y (toward the
 * viewer).  Using C99 complex numbers lets points be added and scaled. */
typedef complex double point;

/*
 * Font: every uppercase letter is drawn as a chain of quadratic bezier curves
 * on a 13 x 21 grid.  A point with is_control set is the bezier control point
 * of the next curve; a curve without a control point is a straight line.
 */
struct font_point {
    int x, y, is_control;
};

struct glyph {
    int count;
    struct font_point points[11];
};

static const struct glyph font[26] = {
    /* A */ {5, {{1, 16, 0}, {6, 2, 0}, {11, 16, 0}, {9, 11, 0}, {3, 11, 0}}},
    /* B */ {10, {{2, 2, 0}, {10, 1, 1}, {10, 5, 0}, {10, 8, 1}, {2, 8, 0}, {10, 8, 1}, {10, 12, 0}, {10, 16, 1}, {2, 15, 0}, {2, 2, 0}}},
    /* C */ {5, {{10, 3, 0}, {2, 0, 1}, {2, 8, 0}, {2, 16, 1}, {10, 13, 0}}},
    /* D */ {6, {{2, 2, 0}, {10, 1, 1}, {10, 8, 0}, {10, 16, 1}, {2, 15, 0}, {2, 2, 0}}},
    /* E */ {7, {{10, 2, 0}, {2, 2, 0}, {2, 8, 0}, {8, 8, 0}, {2, 8, 0}, {2, 15, 0}, {10, 15, 0}}},
    /* F */ {6, {{10, 2, 0}, {2, 2, 0}, {2, 8, 0}, {8, 8, 0}, {2, 8, 0}, {2, 15, 0}}},
    /* G */ {8, {{10, 3, 0}, {2, 0, 1}, {2, 8, 0}, {2, 16, 1}, {6, 16, 0}, {11, 16, 1}, {10, 10, 0}, {6, 10, 0}}},
    /* H */ {6, {{2, 2, 0}, {2, 15, 0}, {2, 8, 0}, {10, 8, 0}, {10, 2, 0}, {10, 15, 0}}},
    /* I */ {6, {{3, 2, 0}, {9, 2, 0}, {6, 2, 0}, {6, 15, 0}, {3, 15, 0}, {9, 15, 0}}},
    /* J */ {6, {{6, 2, 0}, {11, 2, 0}, {9, 2, 0}, {9, 13, 0}, {6, 19, 1}, {3, 13, 0}}},
    /* K */ {6, {{2, 2, 0}, {2, 15, 0}, {2, 10, 0}, {10, 2, 0}, {4, 8, 0}, {10, 15, 0}}},
    /* L */ {3, {{2, 2, 0}, {2, 15, 0}, {10, 15, 0}}},
    /* M */ {5, {{2, 15, 0}, {2, 2, 0}, {6, 10, 0}, {10, 2, 0}, {10, 15, 0}}},
    /* N */ {4, {{2, 15, 0}, {2, 2, 0}, {10, 15, 0}, {10, 2, 0}}},
    /* O */ {9, {{6, 2, 0}, {2, 2, 1}, {2, 9, 0}, {2, 15, 1}, {6, 15, 0}, {10, 15, 1}, {10, 9, 0}, {10, 2, 1}, {6, 2, 0}}},
    /* P */ {6, {{2, 15, 0}, {2, 2, 0}, {10, 1, 1}, {10, 5, 0}, {10, 9, 1}, {2, 8, 0}}},
    /* Q */ {11, {{10, 19, 0}, {6, 19, 1}, {6, 15, 0}, {10, 15, 1}, {10, 9, 0}, {10, 2, 1}, {6, 2, 0}, {2, 2, 1}, {2, 9, 0}, {2, 15, 1}, {6, 15, 0}}},
    /* R */ {8, {{2, 15, 0}, {2, 2, 0}, {10, 1, 1}, {10, 5, 0}, {10, 9, 1}, {2, 8, 0}, {6, 8, 0}, {10, 15, 0}}},
    /* S */ {9, {{10, 3, 0}, {6, 1, 1}, {3, 2, 0}, {0, 5, 1}, {6, 8, 0}, {12, 11, 1}, {9, 15, 0}, {6, 16, 1}, {2, 14, 0}}},
    /* T */ {4, {{1, 2, 0}, {11, 2, 0}, {6, 2, 0}, {6, 16, 0}}},
    /* U */ {5, {{2, 2, 0}, {1, 16, 1}, {6, 16, 0}, {11, 16, 1}, {10, 2, 0}}},
    /* V */ {3, {{2, 2, 0}, {6, 15, 0}, {10, 2, 0}}},
    /* W */ {5, {{1, 2, 0}, {3, 15, 0}, {6, 4, 0}, {9, 15, 0}, {11, 2, 0}}},
    /* X */ {5, {{2, 2, 0}, {10, 16, 0}, {6, 9, 0}, {10, 2, 0}, {2, 16, 0}}},
    /* Y */ {5, {{2, 1, 0}, {6, 9, 0}, {6, 16, 0}, {6, 9, 0}, {10, 1, 0}}},
    /* Z */ {4, {{2, 2, 0}, {10, 2, 0}, {2, 15, 0}, {10, 15, 0}}},
};

/* Default text, laid out as 4 lines of 7 columns */
static const char default_message[] =
    "MAY THE"
    " IOCCC "
    "BE WITH"
    "  YOU";
#define DEFAULT_COLUMNS 7

/* A letter placed on the flat "page" that scrolls away from the viewer */
struct letter {
    int x, y, glyph;
};

static struct letter letters[MAX_LETTERS];
static int letter_count;
static int line_count;

/* The output buffer: clear screen, cursor home, then the frame itself */
static char output[sizeof(CLEAR_SCREEN CURSOR_HOME) + SCREEN_SIZE] =
    CLEAR_SCREEN CURSOR_HOME;
static char *const screen = output + sizeof(CLEAR_SCREEN CURSOR_HOME) - 1;

/* A quadratic bezier curve of one glyph, placed at a position on the page */
struct curve {
    point start, control, end, offset;
};

/*
 * Evaluate the curve at parameter t (0..1), then project the page point onto
 * the screen with perspective: the farther away, the smaller and higher.
 * Result: real part is the screen column (centered on 0), imaginary part is
 * the screen row in half-character units.
 */
static point project(const struct curve *c, double t)
{
    double s = 1 - t;
    point world = s * s * c->start + 2 * s * t * c->control + t * t * c->end
                  + c->offset;
    double distance = -cimag(world);
    double x = creal(world);

    if (distance < 1)
        distance = 1;
    return FOCAL_LENGTH * (x + FOCAL_LENGTH * I) / distance;
}

/*
 * Draw one dot.  Every terminal cell is two dots tall:
 * '^' = upper dot, '_' = lower dot, 'I' = both.
 */
static void plot(point p)
{
    int column = creal(p);
    int half_row = cimag(p);
    char *cell;

    if (column <= -41 || column >= 39 || half_row <= 9 || half_row >= 48)
        return;

    cell = &screen[half_row / 2 * SCREEN_WIDTH + column + 40];
    if (*cell == 'I')
        return;
    if (half_row % 2)
        *cell = (*cell == '^') ? 'I' : '_';
    else
        *cell = (*cell == '_') ? 'I' : '^';
}

/*
 * Draw the curve between parameters t0 and t1: if both ends project to less
 * than one unit apart, plot the midpoint; otherwise split in half and recurse.
 */
static void draw_curve(const struct curve *c, double t0, double t1)
{
    double middle = (t0 + t1) / 2;
    point delta = project(c, t0) - project(c, t1);
    double dx = creal(delta), dy = cimag(delta);

    if (dx * dx + dy * dy < 1) {
        plot(project(c, middle));
    } else {
        draw_curve(c, t0, middle);
        draw_curve(c, middle, t1);
    }
}

static point font_point_position(const struct font_point *fp)
{
    return fp->x + fp->y * I;
}

/* Draw one letter's glyph as a chain of bezier curves */
static void draw_letter(const struct letter *letter, int frame)
{
    const struct glyph *g = &font[letter->glyph];
    struct curve c;
    int i;

    c.offset = (letter->y - frame - START_DISTANCE) * I + letter->x;
    c.start = c.control = font_point_position(&g->points[0]);

    for (i = 1; i < g->count; i++) {
        if (g->points[i].is_control) {
            c.control = font_point_position(&g->points[i]);
        } else {
            c.end = font_point_position(&g->points[i]);
            draw_curve(&c, 0, 1);
            c.start = c.control = c.end;
        }
    }
}

static void load_default_message(void)
{
    int pos;

    letter_count = 0;
    for (pos = 0; default_message[pos] != '\0'; pos++) {
        if (default_message[pos] != ' ') {
            letters[letter_count].x = pos % DEFAULT_COLUMNS * LETTER_SPACING
                                      - DEFAULT_COLUMNS * LETTER_SPACING / 2;
            letters[letter_count].y = pos / DEFAULT_COLUMNS * LINE_SPACING;
            letters[letter_count].glyph = default_message[pos] - 'A';
            letter_count++;
        }
    }
    line_count = (pos + DEFAULT_COLUMNS - 1) / DEFAULT_COLUMNS;
}

/*
 * Read the text from a file, decoding it with ROT13.  Each line is centered.
 * Only uppercase letters are drawn; other characters leave a gap.  A line is
 * only counted when it ends with a line break.
 */
static void load_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    int ch, column = 0, stored = 0;

    letter_count = line_count = 0;
    while ((ch = fgetc(file)) > 0) {
        if (ch > '\r') {
            if (ch >= 'A' && ch <= 'Z') {
                letters[stored].x = column * LETTER_SPACING;
                letters[stored].y = line_count * LINE_SPACING;
                letters[stored].glyph = (ch - 'A' + 13) % 26;   /* ROT13 */
                stored++;
            }
            column++;
        } else {
            /* end of line: shift the letters of this line to center it */
            line_count++;
            for (; letter_count < stored; letter_count++)
                letters[letter_count].x -= column * LETTER_SPACING / 2;
            column = 0;
        }
    }
    fclose(file);
}

static void clear_screen_buffer(void)
{
    int n;

    for (n = 0; n < SCREEN_SIZE; n++)
        screen[n] = (n % SCREEN_WIDTH == SCREEN_WIDTH - 1) ? '\n' : ' ';
    screen[SCREEN_SIZE] = '\0';
}

int main(int argc, char **argv)
{
    clock_t start_time = clock();
    const char *frame_text = output;   /* first frame also clears the screen */
    int frame, total_frames, i;

    load_default_message();
    if (argc > 1)
        load_file(argv[1]);

    total_frames = 200 + line_count * LINE_SPACING;
    for (frame = 0; frame < total_frames; frame++) {
        clear_screen_buffer();
        for (i = 0; i < letter_count; i++)
            draw_letter(&letters[i], frame);

        puts(frame_text);
        frame_text = output + sizeof(CLEAR_SCREEN) - 1;

        /* busy-wait until it is time for the next frame */
        while ((clock() - start_time) * FRAMES_PER_SECOND
               < frame * CLOCKS_PER_SEC)
            ;
    }
    return 0;
}
