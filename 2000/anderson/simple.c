#include <stdio.h>

/*
 * Translate text on standard input into ASCII-art semaphore signals.
 *
 * Every signal is drawn as a small 8x4 character picture of a person
 * holding two flags.  Up to 8 pictures are printed side by side.
 */

#define LINE_SIZE 1000
#define GLYPH_ROWS 4
#define GLYPH_COLS 8
#define GLYPHS_PER_LINE 8
#define NUM_CODES 28

/* special signal codes; letters A..Z are codes 2..27 */
#define CODE_BREAK 0
#define CODE_NUMERAL 1
#define CODE_A 2
#define CODE_K 12

/* positions an arm can point to (as seen from the viewer's left arm) */
enum arm_position {
    ACROSS_DOWN, /* down, crossing in front of the body */
    DOWN,        /* hanging straight down */
    LOW,         /* down and out */
    OUT,         /* horizontally out */
    HIGH,        /* up and out */
    UP,          /* straight up */
    ACROSS_HIGH  /* up, crossing in front of the body */
};

/* arm positions {left arm, right arm} for every signal code */
static const char signal_table[NUM_CODES][2] = {
    {DOWN, DOWN},         /* break (rest) */
    {UP, HIGH},           /* numeral sign */
    {LOW, DOWN},          /* A / 1 */
    {OUT, DOWN},          /* B / 2 */
    {HIGH, DOWN},         /* C / 3 */
    {UP, DOWN},           /* D / 4 */
    {DOWN, HIGH},         /* E / 5 */
    {DOWN, OUT},          /* F / 6 */
    {DOWN, LOW},          /* G / 7 */
    {OUT, ACROSS_DOWN},   /* H / 8 */
    {UP, ACROSS_DOWN},    /* I / 9 */
    {UP, OUT},            /* J */
    {LOW, UP},            /* K / 0 */
    {LOW, HIGH},          /* L */
    {LOW, OUT},           /* M */
    {LOW, LOW},           /* N */
    {OUT, ACROSS_HIGH},   /* O */
    {OUT, UP},            /* P */
    {OUT, HIGH},          /* Q */
    {OUT, OUT},           /* R */
    {OUT, LOW},           /* S */
    {HIGH, UP},           /* T */
    {HIGH, HIGH},         /* U */
    {UP, LOW},            /* V */
    {ACROSS_HIGH, OUT},   /* W */
    {ACROSS_HIGH, LOW},   /* X */
    {HIGH, OUT},          /* Y */
    {ACROSS_DOWN, OUT},   /* Z */
};

/* where the stick of the arm is drawn for each position (left arm) */
static const char arm_row[] = {2, 2, 2, 1, 1, 1, 1};
static const char arm_col[] = {4, 2, 2, 2, 2, 2, 4};
static const char arm_char[] = {'\\', '|', '/', '_', '\\', '|', '/'};

/* where the two character flag is drawn for each position (left arm) */
static const char flag_row[] = {3, 3, 3, 2, 0, 0, 0};
static const char flag_col[] = {4, 2, 1, 0, 1, 2, 4};

/* the person without arms */
static const char body[GLYPH_ROWS][GLYPH_COLS + 1] = {
    "        ",
    "   ()   ",
    "   ^^   ",
    "   ][   ",
};

static char glyphs[NUM_CODES][GLYPH_ROWS][GLYPH_COLS];

static void draw_arm(char glyph[GLYPH_ROWS][GLYPH_COLS], int position, int right)
{
    char stick = arm_char[position];
    int col = arm_col[position];
    int fcol = flag_col[position];

    /* the right arm is the left arm mirrored horizontally */
    if (right) {
        col = GLYPH_COLS - 1 - col;
        fcol = GLYPH_COLS - 2 - fcol;
        if (stick == '\\')
            stick = '/';
        else if (stick == '/')
            stick = '\\';
    }
    glyph[(int)arm_row[position]][col] = stick;

    /* flags alternate between the "<>" and "[]" styles */
    if (position % 2 == 0) {
        glyph[(int)flag_row[position]][fcol] = '<';
        glyph[(int)flag_row[position]][fcol + 1] = '>';
    } else {
        glyph[(int)flag_row[position]][fcol] = '[';
        glyph[(int)flag_row[position]][fcol + 1] = ']';
    }
}

static void build_glyphs(void)
{
    int code, row, col;

    for (code = 0; code < NUM_CODES; code++) {
        for (row = 0; row < GLYPH_ROWS; row++)
            for (col = 0; col < GLYPH_COLS; col++)
                glyphs[code][row][col] = body[row][col];
        draw_arm(glyphs[code], signal_table[code][0], 0);
        draw_arm(glyphs[code], signal_table[code][1], 1);
    }
}

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * Convert the text in buf into signal codes, written over the same
 * buffer.  Digits produce two codes (numeral sign + letter) so the
 * output can overtake the input and overwrite characters that have not
 * been read yet, exactly as the original program does.
 */
static int encode_line(char *buf)
{
    char *in = buf;
    char *out = buf;
    int after_space = 1;
    char c;

    while (*in) {
        c = *in;
        if (c >= '0' && c <= '9')
            *out++ = CODE_NUMERAL;

        if (is_space(c)) {
            /* a run of white space is a single break */
            if (!after_space)
                *out++ = CODE_BREAK;
            after_space = 1;
        } else if (c >= 'A' && c <= 'Z') {
            after_space = 0;
            *out++ = c - 'A' + CODE_A;
        } else if (c >= 'a' && c <= 'z') {
            after_space = 0;
            *out++ = c - 'a' + CODE_A;
        } else if (c == '0') {
            after_space = 0;
            *out++ = CODE_K;
        } else if (c > '0' && c <= '9') {
            after_space = 0;
            *out++ = c - '1' + CODE_A;
        }
        in++;
    }
    /* every line ends with a break */
    if (!after_space)
        *out++ = CODE_BREAK;
    return out - buf;
}

static void print_codes(char *codes, char count)
{
    char per_line;
    char row;
    char i;

    /* count is a char just like in the original, so it may wrap around */
    do {
        per_line = count < GLYPHS_PER_LINE ? count : GLYPHS_PER_LINE;
        for (row = 0; row < GLYPH_ROWS; row++) {
            i = 0;
            do {
                fwrite(glyphs[(int)codes[(int)i]][(int)row], 1, GLYPH_COLS, stdout);
                putchar(' ');
            } while (++i < per_line);
            putchar('\n');
        }
        putchar('\n');
        count -= i;
        codes += i;
    } while (count);
}

int main()
{
    /* encoding can grow a line to twice its length */
    static char line[2 * LINE_SIZE];
    char count;

    build_glyphs();
    while (fgets(line, LINE_SIZE, stdin)) {
        count = encode_line(line);
        if (count)
            print_codes(line, count);
    }
    return 0;
}
