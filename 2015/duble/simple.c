#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * The page is a grid of sub-pixels. Each terminal cell is a Braille character,
 * which is a bitmap 2 dots wide and 4 dots high. A pen position is one number,
 * y * dot_row_size + x, so a pen that runs off the right edge carries on at the
 * left of the next dot row.
 */
#define BRAILLE_BASE      0xA080 /* last two UTF-8 bytes of blank U+2800 */
#define UTF8_BRAILLE_LEAD "\xe2" /* first UTF-8 byte of every Braille char */

#define STROKE_DELAY_US   41088  /* pause after each pen-down step */
#define DRAFT_SPEEDUP     4
#define DRAFT_WOBBLE      35869824 /* lrand48() below this: about 1.7% */

#define PEN_START_ROW     7      /* dot row where a glyph starts */
#define PEN_START_INDENT  2      /* dots between line_end and a new glyph */
#define LINE_START        4      /* line_end value of a new line */
#define SPACE_WIDTH       4      /* width of the ' ' character in dots */
#define ROOM_FOR_GLYPH    12     /* dots needed to draw one more glyph */
#define ROOM_FOR_WORD     58     /* dots needed to start one more word */

#define PROMPT            "> "
#define START_LINE        "\n\n\n\033[3A" /* make room, return to the top */
#define NEXT_LINE         "\033[3B"       /* move below the current line */

/* Move the cursor to a cell, print one blue Braille character there, and go
 * back to the starting row */
#define PLOT_FORMAT "\033[1A\033[%dB\033[%dC\033[34m" UTF8_BRAILLE_LEAD \
                    "%c%c\r\n\033[0m\033[%dA"

/* Typing this character shows the message below and ends the program */
#define SECRET_CHARACTER  '#'
#define SECRET_MESSAGE    "segmentation fault"

/* Compass directions, clockwise from up */
enum { DIRECTION_UP = 0, DIRECTION_DOWN = 4, DIRECTION_COUNT = 8 };

static const int step_x[DIRECTION_COUNT] = { 0,  1, 1, 1, 0, -1, -1, -1};
static const int step_y[DIRECTION_COUNT] = {-1, -1, 0, 1, 1,  1,  0, -1};

/* Bit that lights each dot of a cell, indexed by (dot row * 2 + dot column).
 * The bottom two dots were added to Braille later, so they are the high bits of
 * the code point. In BRAILLE_BASE they land in the lead byte, at bits 8 and 9 */
static const int dot_bit[8] = {0, 3, 1, 4, 2, 5, 8, 9};

/*
 * The drawing commands. Every command ends by plotting the pen. The RIGHT_n,
 * FORWARD_n and LEFT_n commands take n steps, and before each step they turn
 * 45 degrees right, not at all, or 45 degrees left.
 */
enum Command {
    END,        /* no more commands in this glyph */
    GO_BACK,    /* turn around, without moving */
    ABOVE,      /* jump to the top dot row, pointing down, pen down */
    MIDDLE,     /* the same, but to the top of the second row of cells */
    TOGGLE_PEN, /* lift the pen, or put it down */
    CALL,       /* draw the glyph named by the next entry */
    BOUNCE,     /* flip the direction top to bottom, then take one step */
    RIGHT_1,
    FORWARD_1,
    LEFT_1,
    RIGHT_2,
    FORWARD_2,
    LEFT_2,
    RIGHT_5,
    FORWARD_5,
    LEFT_5
};

/* Glyphs that other glyphs draw with CALL */
enum GlyphId {
    GLYPH_L_NO_END,
    GLYPH_HALF_W,
    GLYPH_U_BRIDGE,
    GLYPH_R_TRACE,
    GLYPH_C,
    GLYPH_COLON,
    GLYPH_N,
    GLYPH_O,
    GLYPH_R,
    GLYPH_U
};

typedef struct {
    char key;                  /* the lower case character it draws */
    unsigned char strokes[16]; /* commands, finished by END */
} Glyph;

/* Spaces never reach the glyph lookup, so ' ' is a key that cannot be typed */
#define HELPER_GLYPH ' '

static const Glyph glyphs[] = {
    [GLYPH_L_NO_END] = {HELPER_GLYPH, {FORWARD_2, TOGGLE_PEN, RIGHT_1,
            FORWARD_1, LEFT_1, FORWARD_1, LEFT_1, BOUNCE, LEFT_1, FORWARD_2,
            FORWARD_2}},
    [GLYPH_HALF_W] = {HELPER_GLYPH, {MIDDLE, FORWARD_2, LEFT_1, BOUNCE,
            LEFT_1, FORWARD_1}},
    [GLYPH_U_BRIDGE] = {HELPER_GLYPH, {FORWARD_2, LEFT_2, LEFT_2, FORWARD_1,
            GO_BACK}},
    [GLYPH_R_TRACE] = {HELPER_GLYPH, {FORWARD_2, FORWARD_1, GO_BACK,
            FORWARD_2, RIGHT_2}},
    [GLYPH_C] = {'c', {FORWARD_2, RIGHT_2, GO_BACK, TOGGLE_PEN, FORWARD_1,
            LEFT_2, LEFT_2}},
    [GLYPH_COLON] = {':', {MIDDLE, TOGGLE_PEN, FORWARD_2, FORWARD_1,
            TOGGLE_PEN}},
    [GLYPH_N] = {'n', {CALL, GLYPH_R, RIGHT_2, FORWARD_1}},
    [GLYPH_O] = {'o', {CALL, GLYPH_C, LEFT_2, FORWARD_1}},
    [GLYPH_R] = {'r', {MIDDLE, CALL, GLYPH_R_TRACE}},
    [GLYPH_U] = {'u', {MIDDLE, CALL, GLYPH_U_BRIDGE, FORWARD_2, FORWARD_1}},
    {'a', {CALL, GLYPH_C, FORWARD_1, TOGGLE_PEN, LEFT_2, LEFT_1, GO_BACK,
            TOGGLE_PEN, RIGHT_1, FORWARD_1, LEFT_1}},
    {'b', {ABOVE, FORWARD_2, FORWARD_2, CALL, GLYPH_R_TRACE, RIGHT_2, RIGHT_2}},
    {'d', {CALL, GLYPH_C, FORWARD_1, ABOVE, FORWARD_5, FORWARD_1, LEFT_1}},
    {'e', {FORWARD_1, RIGHT_1, TOGGLE_PEN, RIGHT_1, TOGGLE_PEN, RIGHT_1,
            GO_BACK, RIGHT_1, TOGGLE_PEN, LEFT_5, LEFT_1}},
    {'f', {CALL, GLYPH_L_NO_END, FORWARD_2, FORWARD_2, RIGHT_1}},
    {'g', {CALL, GLYPH_O, GO_BACK, FORWARD_5, FORWARD_1, RIGHT_5}},
    {'h', {CALL, GLYPH_L_NO_END, FORWARD_1, GO_BACK, FORWARD_2, RIGHT_1,
            BOUNCE, RIGHT_1, FORWARD_1}},
    {'i', {MIDDLE, FORWARD_2, LEFT_1, GO_BACK, TOGGLE_PEN, FORWARD_1, RIGHT_1,
            FORWARD_2, FORWARD_1, TOGGLE_PEN}},
    {'j', {MIDDLE, FORWARD_5, FORWARD_1, RIGHT_1, GO_BACK, TOGGLE_PEN,
            FORWARD_1, LEFT_1, FORWARD_5, FORWARD_2, TOGGLE_PEN}},
    {'k', {CALL, GLYPH_L_NO_END, FORWARD_1, GO_BACK, RIGHT_1, FORWARD_2,
            TOGGLE_PEN, GO_BACK, LEFT_1, FORWARD_1, TOGGLE_PEN, FORWARD_1}},
    {'l', {CALL, GLYPH_L_NO_END, LEFT_1}},
    {'m', {CALL, GLYPH_N, GO_BACK, FORWARD_2, RIGHT_2, RIGHT_2, FORWARD_1}},
    {'p', {MIDDLE, FORWARD_5, FORWARD_2, GO_BACK, FORWARD_5, FORWARD_1,
            RIGHT_5, RIGHT_1}},
    {'q', {CALL, GLYPH_O, GO_BACK, FORWARD_5, FORWARD_2}},
    {'s', {FORWARD_1, TOGGLE_PEN, RIGHT_1, FORWARD_1, BOUNCE, RIGHT_2, RIGHT_1}},
    {'t', {ABOVE, FORWARD_5, FORWARD_1, LEFT_1, TOGGLE_PEN, LEFT_2, LEFT_2,
            FORWARD_2, GO_BACK, TOGGLE_PEN, LEFT_1, FORWARD_1}},
    {'v', {MIDDLE, FORWARD_1, LEFT_1, FORWARD_1, BOUNCE, FORWARD_1, LEFT_1}},
    {'w', {CALL, GLYPH_HALF_W, CALL, GLYPH_HALF_W}},
    {'x', {MIDDLE, LEFT_1, FORWARD_2, TOGGLE_PEN, LEFT_5, TOGGLE_PEN, LEFT_1,
            FORWARD_2}},
    {'y', {CALL, GLYPH_U, FORWARD_2, FORWARD_1, RIGHT_1}},
    {'z', {TOGGLE_PEN, FORWARD_2, RIGHT_1, BOUNCE, RIGHT_1, FORWARD_1, LEFT_1,
            FORWARD_1, RIGHT_2, BOUNCE, RIGHT_2, FORWARD_1}},
    {'!', {ABOVE, FORWARD_5, TOGGLE_PEN, FORWARD_2, TOGGLE_PEN}},
    {',', {TOGGLE_PEN, GO_BACK, RIGHT_1}},
    {'.', {TOGGLE_PEN}},
    {';', {CALL, GLYPH_COLON, RIGHT_1}},
    {'?', {FORWARD_5, FORWARD_1, TOGGLE_PEN, RIGHT_2, RIGHT_2, FORWARD_1,
            RIGHT_1, LEFT_1, TOGGLE_PEN, FORWARD_2, TOGGLE_PEN}},
};

#define GLYPH_COUNT (sizeof(glyphs) / sizeof(glyphs[0]))

static int columns;       /* width of the terminal in cells */
static int dot_row_size;  /* sub-pixels in one dot row */
static int cell_row_size; /* sub-pixels in one row of cells */
static int *canvas;       /* the Braille pattern of each cell on this line */
static int canvas_bytes;

static int line_end;      /* rightmost dot drawn so far, 0 for no line */

static int pen_position;  /* y * dot_row_size + x */
static int pen_direction;
static int pen_down;
static int draft_mode;

static void print_text(const char *text)
{
    write(STDOUT_FILENO, text, strlen(text));
}

/*
 * The prompt goes to standard input, not standard output. A terminal accepts
 * this, but a pipe or a file opened for reading refuses it. So the prompt only
 * shows up in interactive use, with no need to call isatty().
 */
static void print_prompt(void)
{
    write(STDIN_FILENO, PROMPT, strlen(PROMPT));
}

/* Light the dot under the pen and print the cell that contains it */
static void plot_pen(void)
{
    char output[97];
    int cell_row = pen_position / cell_row_size;
    int x = pen_position % dot_row_size;
    int dot_row = pen_position % cell_row_size / dot_row_size;
    int *cell = canvas + cell_row * columns + x / 2;

    if (!pen_down)
        return;

    *cell |= BRAILLE_BASE | 1 << dot_bit[dot_row * 2 + pen_position % 2];

    sprintf(output, PLOT_FORMAT, cell_row + 1, x / 2,
            *cell >> 8, *cell & 255, cell_row + 1);
    print_text(output);

    if (x > line_end)
        line_end = x;
}

/* Every command ends here. Draft mode makes the pen slip now and then */
static void finish_command(int moves_pen)
{
    if (draft_mode && lrand48() < DRAFT_WOBBLE)
        pen_position++;

    if (moves_pen)
        pen_position += step_y[pen_direction] * dot_row_size
                      + step_x[pen_direction];

    plot_pen();

    if (pen_down)
        usleep(STROKE_DELAY_US / (draft_mode ? DRAFT_SPEEDUP : 1));
}

static void walk(int steps, int turn)
{
    while (steps-- > 0) {
        pen_direction = (pen_direction + turn + DIRECTION_COUNT)
                      % DIRECTION_COUNT;
        finish_command(1);
    }
}

static void jump_to_row(int position)
{
    pen_direction = DIRECTION_DOWN;
    pen_position = pen_position % dot_row_size + position;
    pen_down = 1;
    finish_command(0);
}

static void draw_glyph(const Glyph *glyph)
{
    const unsigned char *command;

    for (command = glyph->strokes; *command != END; command++) {
        switch (*command) {
        case GO_BACK:
            pen_direction = (pen_direction + DIRECTION_COUNT / 2)
                          % DIRECTION_COUNT;
            finish_command(0);
            break;
        case ABOVE:
            jump_to_row(0);
            break;
        case MIDDLE:
            jump_to_row(cell_row_size);
            break;
        case TOGGLE_PEN:
            pen_down = !pen_down;
            finish_command(0);
            break;
        case CALL:
            draw_glyph(&glyphs[*++command]);
            finish_command(0);
            break;
        case BOUNCE:
            pen_direction = (12 - pen_direction) % DIRECTION_COUNT;
            finish_command(1);
            break;
        case RIGHT_1:
            walk(1, 1);
            break;
        case FORWARD_1:
            walk(1, 0);
            break;
        case LEFT_1:
            walk(1, -1);
            break;
        case RIGHT_2:
            walk(2, 1);
            break;
        case FORWARD_2:
            walk(2, 0);
            break;
        case LEFT_2:
            walk(2, -1);
            break;
        case RIGHT_5:
            walk(5, 1);
            break;
        case FORWARD_5:
            walk(5, 0);
            break;
        case LEFT_5:
            walk(5, -1);
            break;
        }
    }
}

static const Glyph *find_glyph(char key)
{
    size_t i;

    for (i = 0; i < GLYPH_COUNT; i++)
        if (glyphs[i].key == key)
            return &glyphs[i];

    return NULL;
}

static void write_character(char character)
{
    const Glyph *glyph;
    int is_newline = character == '\n';
    int is_space = character == ' ';

    if (is_newline || (is_space && line_end + ROOM_FOR_WORD > dot_row_size)
            || line_end + ROOM_FOR_GLYPH > dot_row_size) {
        print_text(NEXT_LINE);
        print_prompt();
        line_end = 0;
    } else if (is_space) {
        line_end += SPACE_WIDTH;
    }

    if (is_newline || is_space)
        return;

    if (line_end == 0) {
        memset(canvas, 0, canvas_bytes);
        line_end = LINE_START;
        print_text(START_LINE);
    }

    pen_position = PEN_START_ROW * dot_row_size + line_end + PEN_START_INDENT;
    pen_direction = DIRECTION_UP;
    pen_down = 0;

    if (character >= 'A' && character <= 'Z')
        character += 'a' - 'A';

    glyph = find_glyph(character);
    if (glyph != NULL)
        draw_glyph(glyph);
}

static void write_secret_message(void)
{
    const char *character;

    for (character = SECRET_MESSAGE; *character != '\0'; character++)
        write_character(*character);

    write_character('\n');
}

int main(void)
{
    struct winsize window = {0};
    char character;

    signal(SIGPIPE, SIG_IGN);

    draft_mode = getenv("DRAFT") != NULL;

    print_prompt();

    /* This fails when the output is not a terminal. Everything stays 0 and the
     * first glyph drawn divides by zero, which is why such a run dumps core */
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);
    columns = window.ws_col;
    dot_row_size = columns * 2;
    cell_row_size = dot_row_size * 4;
    canvas_bytes = 3 * cell_row_size;
    canvas = calloc(canvas_bytes, 1);

    while (read(STDIN_FILENO, &character, 1) > 0) {
        if (character == SECRET_CHARACTER) {
            write_secret_message();
            break;
        }
        write_character(character);
    }

    return 0;
}
