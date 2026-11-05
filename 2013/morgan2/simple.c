#include <stdlib.h>
#include <curses.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

/* The board and the stack of pending dominoes. */
#define BOARD_SIZE 8
#define CELL_COUNT (BOARD_SIZE * BOARD_SIZE)
#define STACK_SIZE 8

/*
 * Screen layout, measured in text cells. The X11 display uses the same
 * coordinates and scales them up (see draw_square and draw_frame_x11).
 */
#define CELL_WIDTH 6
#define CELL_HEIGHT 3
#define BOARD_WIDTH (BOARD_SIZE * CELL_WIDTH)
#define BOARD_HEIGHT (BOARD_SIZE * CELL_HEIGHT)
#define STACK_LEFT 50
#define SELECTED_LEFT 58
#define STATUS_LINE 4

/* A domino is drawn two cells inside the left edge of its box. */
#define DOMINO_INSET 2
/* The cursor is drawn one cell inside the left edge of its box. */
#define CURSOR_INSET 1

/* A domino is 3 columns of 2 squares. */
#define DOMINO_SQUARES 6
#define CURSOR_SQUARES 4
#define CURSOR_SPACING 4

/*
 * Bit (2 * column + row) of (id - 1) is the color of that square, so every
 * combination of colors appears exactly once in the ids 1 to 64.
 * An id of 0 means there is no domino.
 */
#define NO_DOMINO 0
#define COLUMN_MASK 3        /* bits of the leftmost column */
#define ROW_MASK 21          /* bits of the top row */
#define COLUMN_SHIFT 4       /* moves the rightmost column onto the leftmost */
#define ROW_SHIFT 1          /* moves the bottom row onto the top row */

/* Result of comparing two touching sides. Any mismatch outweighs all matches. */
#define SIDE_MATCH 1
#define SIDE_MISMATCH 8
#define MAX_MATCHING_SIDES 4

/* Palette indexes, used for both the X11 colors and the curses color pairs. */
#define PALETTE_BLANK 0
#define PALETTE_RED 1
#define PALETTE_GREEN 2
#define PALETTE_CURSOR 3
#define PALETTE_SIZE 4

/* X11 window geometry in pixels. */
#define WINDOW_WIDTH 460
#define WINDOW_HEIGHT 300
#define PIXELS_PER_COLUMN 6
#define PIXELS_PER_LINE 12
#define SQUARE_OFFSET 1
#define BOX_SIZE 36
#define BOX_LEFT 3
#define BOX_TOP 6
#define STATUS_X 348
#define STATUS_Y 58

#define KEY_QUIT 'q'
#define KEY_NEWLINE 10

static const char *const x11_color_names[PALETTE_SIZE] = {
    "#fff", "#f00", "#0f0", "#000"
};

/* Bonus for placing a domino, indexed by the pending dominoes minus one. */
static const int stack_bonus[STACK_SIZE] = { 8, 4, 2, 2, 0, 0, 0, 0 };

static int use_x11;

/* X11 state. */
static Display *display;
static Window window;
static GC gc;
static XColor colors[PALETTE_SIZE];

/* Game state. */
static int draw_pile[CELL_COUNT];
static int next_draw = 1;
static int board[BOARD_SIZE][BOARD_SIZE];
static int stack[STACK_SIZE];
static int pending_count;
static int selected = NO_DOMINO;
static int placing;
static int board_row;
static int board_column;
static int stack_row;
static int total_score;
static int placed_count = 1;

static char status_text[64];
static int status_length;

/* Fills a rectangle, then restores the drawing color used by the outlines. */
static void fill_rectangle(int x, int y, int width, int height, int palette)
{
    XSetForeground(display, gc, colors[palette].pixel);
    XFillRectangle(display, window, gc, x, y, width, height);
    XSetForeground(display, gc, colors[PALETTE_CURSOR].pixel);
}

static void init_x11(void)
{
    int screen;
    int index;
    XColor exact_color;

    display = XOpenDisplay(0);
    screen = DefaultScreen(display);
    for (index = 0; index < PALETTE_SIZE; index++) {
        XAllocNamedColor(display, DefaultColormap(display, screen),
                         x11_color_names[index], &colors[index], &exact_color);
    }
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0,
                                 WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                                 colors[PALETTE_CURSOR].pixel,
                                 colors[PALETTE_BLANK].pixel);
    XMapWindow(display, window);
    XSelectInput(display, window, KeyPressMask | ExposureMask);
    gc = XCreateGC(display, window, 0, 0);
}

static void init_curses(void)
{
    initscr();
    start_color();
    keypad(stdscr, 1);
    curs_set(0);
    init_pair(PALETTE_RED, COLOR_RED, COLOR_RED);
    init_pair(PALETTE_GREEN, COLOR_GREEN, COLOR_GREEN);
    init_pair(PALETTE_CURSOR, COLOR_WHITE, COLOR_WHITE);
}

/* Draws one outlined box per cell. */
static void draw_frame_x11(int left, int width, int height)
{
    int line;
    int column;

    for (line = 0; line < height; line += CELL_HEIGHT) {
        for (column = left; column < left + width; column += CELL_WIDTH) {
            XDrawRectangle(display, window, gc,
                           BOX_LEFT + PIXELS_PER_COLUMN * column,
                           BOX_TOP + PIXELS_PER_LINE * line,
                           BOX_SIZE, BOX_SIZE);
        }
    }
}

/* Draws a table with line characters, which also wipes the cell interiors. */
static void draw_frame_curses(int left, int width, int height)
{
    int line;
    int column;

    for (line = 0; line <= height; line++) {
        int is_outer_line = line == 0 || line == height;

        move(line, left);
        if (line == 0) {
            addch(ACS_ULCORNER);
        } else if (line == height) {
            addch(ACS_LLCORNER);
        } else {
            addch(ACS_VLINE);
        }

        for (column = 1; column < width; column++) {
            if (column % CELL_WIDTH == 0) {
                addch(is_outer_line ? ACS_HLINE : ACS_VLINE);
            } else if (line % CELL_HEIGHT == 0) {
                addch(ACS_HLINE);
            } else {
                addch(' ');
            }
        }

        if (line == 0) {
            addch(ACS_URCORNER);
        } else if (line == height) {
            addch(ACS_LRCORNER);
        } else {
            addch(ACS_VLINE);
        }
    }
}

static void draw_frame(int left, int width, int height)
{
    if (use_x11) {
        draw_frame_x11(left, width, height);
    } else {
        draw_frame_curses(left, width, height);
    }
}

/* Paints one square at a position given in text cells. */
static void draw_square(int column, int line, int palette)
{
    if (use_x11) {
        fill_rectangle(SQUARE_OFFSET + PIXELS_PER_COLUMN * column,
                       SQUARE_OFFSET + PIXELS_PER_LINE * line,
                       PIXELS_PER_COLUMN, PIXELS_PER_LINE, palette);
    } else {
        mvaddch(line, column, ' ' | COLOR_PAIR(palette));
    }
}

/* Draws a domino, or blanks the position when the id is NO_DOMINO. */
static void draw_domino(int left, int box_row, int id)
{
    int square;

    for (square = 0; square < DOMINO_SQUARES; square++) {
        int palette = PALETTE_BLANK;

        if (id != NO_DOMINO) {
            palette = (((id - 1) >> square) & 1) ? PALETTE_GREEN : PALETTE_RED;
        }
        draw_square(left + square / 2,
                    1 + CELL_HEIGHT * box_row + square % 2, palette);
    }
}

/* The cursor is four squares, one inside each corner of the box. */
static void draw_cursor(int left, int box_row)
{
    int square;

    for (square = 0; square < CURSOR_SQUARES; square++) {
        draw_square(left + CURSOR_SPACING * (square / 2),
                    1 + CELL_HEIGHT * box_row + square % 2, PALETTE_CURSOR);
    }
}

static void draw_game(void)
{
    int row;
    int column;

    if (use_x11) {
        fill_rectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, PALETTE_BLANK);
    }

    draw_frame(0, BOARD_WIDTH, BOARD_HEIGHT);
    draw_frame(STACK_LEFT, CELL_WIDTH, BOARD_HEIGHT);
    draw_frame(SELECTED_LEFT, CELL_WIDTH, CELL_HEIGHT);

    for (row = 0; row < BOARD_SIZE; row++) {
        for (column = 0; column < BOARD_SIZE; column++) {
            draw_domino(DOMINO_INSET + CELL_WIDTH * column, row,
                        board[row][column]);
        }
        draw_domino(STACK_LEFT + DOMINO_INSET, row, stack[row]);
    }

    draw_domino(SELECTED_LEFT + DOMINO_INSET, 0, selected);

    if (placing) {
        draw_cursor(CURSOR_INSET + CELL_WIDTH * board_column, board_row);
    } else {
        draw_cursor(STACK_LEFT + CURSOR_INSET, stack_row);
    }

    status_length = sprintf(status_text, "score: %d %d %d", total_score,
                            placed_count, (placed_count * total_score) >> 6);
}

/* Shows the status text and waits. Non-key events return 0 to force a redraw. */
static int read_key_x11(void)
{
    XEvent event;

    XDrawString(display, window, gc, STATUS_X, STATUS_Y,
                status_text, status_length);
    XNextEvent(display, &event);
    if (event.type != KeyPress) {
        return 0;
    }
    return XLookupKeysym(&event.xkey, 0);
}

static int read_key_curses(void)
{
    mvaddstr(STATUS_LINE, SELECTED_LEFT, status_text);
    refresh();
    return getch();
}

/* Shuffles the ids 1 to 64 by dropping each one into a random free slot. */
static void shuffle_draw_pile(void)
{
    int id = 0;

    while (id < CELL_COUNT) {
        int slot = rand() & (CELL_COUNT - 1);

        if (draw_pile[slot] == 0) {
            draw_pile[slot] = ++id;
        }
    }
}

/* Places the first domino on a random cell and starts the cursor there. */
static void place_first_domino(void)
{
    int cell = rand() & (CELL_COUNT - 1);

    board_row = cell / BOARD_SIZE;
    board_column = cell % BOARD_SIZE;
    board[board_row][board_column] = draw_pile[0];
}

/*
 * Compares the touching sides of two dominoes, the first being the one to the
 * left or above. Returns 0 when one of the two positions is empty.
 */
static int compare_sides(int first, int second, int shift, int mask)
{
    if (first == NO_DOMINO || second == NO_DOMINO) {
        return 0;
    }
    if ((((first - 1) >> shift) ^ (second - 1)) & mask) {
        return SIDE_MISMATCH;
    }
    return SIDE_MATCH;
}

/* Scores the selected domino against the neighbors of the board cursor. */
static int score_neighbors(void)
{
    int row = board_row;
    int column = board_column;
    int score = 0;

    if (column > 0) {
        score += compare_sides(board[row][column - 1], selected,
                               COLUMN_SHIFT, COLUMN_MASK);
    }
    if (column < BOARD_SIZE - 1) {
        score += compare_sides(selected, board[row][column + 1],
                               COLUMN_SHIFT, COLUMN_MASK);
    }
    if (row > 0) {
        score += compare_sides(board[row - 1][column], selected,
                               ROW_SHIFT, ROW_MASK);
    }
    if (row < BOARD_SIZE - 1) {
        score += compare_sides(selected, board[row + 1][column],
                               ROW_SHIFT, ROW_MASK);
    }
    return score;
}

/* Enter while on the board: place the selected domino or leave the board. */
static void confirm_on_board(void)
{
    int matches;

    if (board[board_row][board_column] != NO_DOMINO) {
        placing = 0;
        return;
    }

    matches = score_neighbors();
    if (matches < SIDE_MATCH || matches > MAX_MATCHING_SIDES) {
        return;
    }

    board[board_row][board_column] = selected;
    stack[stack_row] = NO_DOMINO;
    selected = NO_DOMINO;
    placing = 0;
    pending_count--;
    total_score += matches * stack_bonus[pending_count];
    placed_count++;
}

/* Enter while on the stack: select the domino here or reveal a new one. */
static void confirm_on_stack(void)
{
    if (stack[stack_row] != NO_DOMINO) {
        selected = stack[stack_row];
        placing = 1;
    } else if (next_draw < CELL_COUNT) {
        stack[stack_row] = draw_pile[next_draw++];
        pending_count++;
    }
}

static void move_up(void)
{
    if (placing) {
        if (board_row > 0) {
            board_row--;
        }
    } else if (stack_row > 0) {
        stack_row--;
    }
}

static void move_down(void)
{
    if (placing) {
        if (board_row < BOARD_SIZE - 1) {
            board_row++;
        }
    } else if (stack_row < STACK_SIZE - 1) {
        stack_row++;
    }
}

/* Left and right only work on the board, the stack is a single column. */
static void move_left(void)
{
    if (placing && board_column > 0) {
        board_column--;
    }
}

static void move_right(void)
{
    if (placing && board_column < BOARD_SIZE - 1) {
        board_column++;
    }
}

static void handle_key(int key)
{
    if (key == (use_x11 ? XK_Up : KEY_UP)) {
        move_up();
    } else if (key == (use_x11 ? XK_Down : KEY_DOWN)) {
        move_down();
    } else if (key == (use_x11 ? XK_Left : KEY_LEFT)) {
        move_left();
    } else if (key == (use_x11 ? XK_Right : KEY_RIGHT)) {
        move_right();
    } else if (key == KEY_NEWLINE || key == (use_x11 ? XK_Return : KEY_ENTER)) {
        if (placing) {
            confirm_on_board();
        } else {
            confirm_on_stack();
        }
    }
}

int main(int argc, char **argv)
{
    int key;

    (void)argc;

    use_x11 = argv[1] != NULL && argv[1][0] == 'X';
    if (use_x11) {
        init_x11();
    } else {
        init_curses();
    }

    srand(time(0));
    shuffle_draw_pile();
    place_first_domino();

    do {
        draw_game();
        key = use_x11 ? read_key_x11() : read_key_curses();
        handle_key(key);
    } while (key != KEY_QUIT);

    return use_x11 || endwin();
}
