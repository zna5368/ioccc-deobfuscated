#include <math.h>
#include <X11/Xlib.h>

/*
 * Two player tic-tac-toe for X Windows.  Each player gets a board on their
 * own X display (both boards may share one display).  Player 1 (window "I")
 * plays crosses and moves first, player 0 (window "II") plays circles.
 */

#define SQUARE 50          /* width and height of one board square        */
#define CENTER 25          /* middle pixel of a square                    */
#define BOARD_SIZE 152     /* three squares plus their 1 pixel borders    */
#define ROW_BYTES 7        /* a 50 pixel bitmap row padded to whole bytes */
#define LINE_WIDTH 3       /* half thickness of the lines in a cross      */

static Display *displays[2];
static Window squares[2][3][3];
static Pixmap blank_pixmap[2], cross_pixmap[2], circle_pixmap[2];

/* scratch 1 bit per pixel image used to build the pixmaps */
static char bitmap[SQUARE * SQUARE];

/* number of times each square was taken (a char, so it wraps after 256) */
static char board[3][3];

/* next command line argument naming an X display */
static char **display_arg;

static void set_pixel(int x, int y)
{
    bitmap[x / 8 + y * ROW_BYTES] |= 1 << x % 8;
}

static void clear_bitmap(void)
{
    int i;

    for (i = 0; i < SQUARE * SQUARE; i++)
        bitmap[i] = 0;
}

/* turn the bitmap into a black and white pixmap and start a fresh bitmap */
static Pixmap make_pixmap(Display *display, Window window, int screen)
{
    Pixmap pixmap = XCreatePixmapFromBitmapData(
        display, window, bitmap, SQUARE, SQUARE, BlackPixel(display, screen),
        WhitePixel(display, screen), DefaultDepth(display, screen));

    clear_bitmap();
    return pixmap;
}

/* two thick diagonal lines from corner to corner */
static void draw_cross(void)
{
    int row, offset, x;

    for (row = 0; row < SQUARE; row++) {
        for (offset = -LINE_WIDTH; offset <= LINE_WIDTH; offset++) {
            x = row + offset;
            if (x >= 0 && x <= SQUARE)
                set_pixel(x, row);
            x = SQUARE - row + offset;
            if (x >= 0 && x <= SQUARE)
                set_pixel(x, row);
        }
    }
}

/* a thick ring: circles with radius 25 down to 20.5 using 8-way symmetry */
static void draw_circle(void)
{
    int dx, dy;
    float radius;

    for (dx = 0; dx < SQUARE; dx++) {
        for (radius = CENTER; radius > 20 && radius > dx; radius -= .5) {
            dy = sqrt(radius * radius - dx * dx);
            set_pixel(CENTER + dx, CENTER + dy);
            set_pixel(CENTER - dx, CENTER + dy);
            set_pixel(CENTER + dy, CENTER + dx);
            set_pixel(CENTER + dy, CENTER - dx);
            set_pixel(CENTER + dx, CENTER - dy);
            set_pixel(CENTER - dx, CENTER - dy);
            set_pixel(CENTER - dy, CENTER - dx);
            set_pixel(CENTER - dy, CENTER + dx);
        }
    }
}

/* open a display and create the board window of one player */
static void open_board(int player)
{
    Display *display;
    Window board_window;
    XSetWindowAttributes attributes;
    int screen, col, row;

    /* use the next argument, or keep the missing one for the default display */
    if (*display_arg)
        display_arg++;
    display = displays[player] = XOpenDisplay(*display_arg);
    screen = XDefaultScreen(display);

    board_window =
        XCreateWindow(display, RootWindow(display, screen), 0, 0, BOARD_SIZE,
                      BOARD_SIZE, 2, CopyFromParent, InputOutput,
                      CopyFromParent, 0, 0);
    XStoreName(display, board_window, player ? "I" : "II");

    blank_pixmap[player] = make_pixmap(display, board_window, screen);
    draw_cross();
    cross_pixmap[player] = make_pixmap(display, board_window, screen);
    draw_circle();
    circle_pixmap[player] = make_pixmap(display, board_window, screen);

    XMapRaised(display, board_window);

    attributes.event_mask = ButtonPressMask;
    for (col = 2; col >= 0; col--) {
        for (row = 2; row >= 0; row--) {
            squares[player][col][row] = XCreateWindow(
                display, board_window, SQUARE * col, SQUARE * row, SQUARE,
                SQUARE, 1, CopyFromParent, InputOutput, CopyFromParent,
                CWEventMask, &attributes);
            XMapRaised(display, squares[player][col][row]);
            XSetWindowBackgroundPixmap(display, squares[player][col][row],
                                       blank_pixmap[player]);
        }
    }
}

/* show the mark of a player in a square on the board of the given display */
static void draw_mark(int board_owner, int col, int row, int mark_player)
{
    Display *display = displays[board_owner];
    Window square = squares[board_owner][col][row];

    XSetWindowBackgroundPixmap(display, square,
                               mark_player ? cross_pixmap[board_owner]
                                           : circle_pixmap[board_owner]);
    XClearWindow(display, square);
}

static void play(void)
{
    int player = 1;
    int col, row;
    XEvent event;

    for (;;) {
        XFlush(displays[!player]);
        XNextEvent(displays[player], &event);
        for (col = 2; col >= 0; col--) {
            for (row = 2; row >= 0; row--) {
                if (event.xany.window == squares[player][col][row] &&
                    !board[col][row]++) {
                    draw_mark(player, col, row, player);
                    player = !player;
                    draw_mark(player, col, row, !player);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    display_arg = argv;
    open_board(0);
    open_board(1);
    play();
    return 0;
}
