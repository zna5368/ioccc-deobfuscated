#include <stdio.h>
#include <curses.h>

#define SIZE 6          /* the board is SIZE x SIZE cells */
#define TARGET 5        /* every cell must be brought back to this value */
#define SCRAMBLE_MOVES 20

/* screen position of a board cell inside the window */
#define SCREEN_ROW(row) (2 * (row) + 1)
#define SCREEN_COL(col) (4 * (col) + 3)

/* size of the board window (one extra cell row/column for the border) */
#define WIN_ROWS SCREEN_ROW(SIZE)
#define WIN_COLS SCREEN_COL(SIZE)

/* random integer in the range [low, high] */
#define RANDOM(low, high) (lrand48() % ((high) - (low) + 1) + (low))

/* board[column][row] holds a digit from 1 to 9 */
char board[SIZE][SIZE];
WINDOW *win = NULL;

char *help_text = "Use l-h-k-j for move.\n"
                  "Use + for increment. Use - for decrement.\n"
                  "Use Q to quit.";

static void quit(int status)
{
    endwin();
    exit(status);
}

void init_game()
{
    long time();

    srand48(time((int *)0));
    memset(board, TARGET, sizeof board);

    initscr();
    cbreak();
    noecho();
    nonl();

    if (COLS < 80 || LINES < 24)
        quit(1);

    win = newwin(WIN_ROWS, WIN_COLS, (LINES - WIN_ROWS) / 2, (COLS - WIN_COLS) / 2);
    if (!win)
        quit(1);

    mvaddstr(LINES - 5, 0, help_text);
    refresh();
}

void draw_board()
{
    int col, row;

    for (row = 0; row < SIZE; ++row)
        for (col = 0; col < SIZE; ++col)
            mvwaddch(win, SCREEN_ROW(row), SCREEN_COL(col), board[col][row] + '0');
}

/*
 * Change the cell at (col, row) by delta (+1 or -1) and every
 * neighbouring cell (up to eight of them) by -delta.  Nothing happens
 * if any cell would leave the range 1..9.
 */
void change_cell(int col, int row, int delta)
{
    int x, y;
    int first_x = col > 0 ? col - 1 : 0, last_x = col < SIZE - 1 ? col + 1 : SIZE - 1;
    int first_y = row > 0 ? row - 1 : 0, last_y = row < SIZE - 1 ? row + 1 : SIZE - 1;

    for (x = first_x; x <= last_x; ++x)
        for (y = first_y; y <= last_y; ++y) {
            int is_center = (x == col && y == row);
            int change = is_center ? delta : -delta;

            if (board[x][y] + change < 1 || board[x][y] + change > 9)
                return;
        }

    for (x = first_x; x <= last_x; ++x)
        for (y = first_y; y <= last_y; ++y)
            board[x][y] += (x == col && y == row) ? delta : -delta;
}

/*
 * The win check: it only looks at the first 35 cells (in memory order),
 * so the bottom-right cell is never actually tested.
 */
int is_solved()
{
    char *cell = (char *)board;
    int i;

    for (i = 0; i < SIZE * SIZE - 1; ++i)
        if (cell[i] != TARGET)
            return 0;
    return 1;
}

int main()
{
    char *strchr();
    int row = 0, col = 0;
    int key, i;

    init_game();

    /*
     * Scramble the board with random valid moves.  The random numbers are
     * drawn in the same order gcc evaluated the original's call arguments
     * (right to left), so the same seed gives the same starting board.
     */
    for (i = 0; i < SCRAMBLE_MOVES; ++i) {
        int increment = RANDOM(1, 2) == 1;
        int r = RANDOM(0, SIZE - 1);
        int c = RANDOM(0, SIZE - 1);

        change_cell(c, r, increment ? 1 : -1);
    }

    box(win, '|', '-');
    draw_board();
    wmove(win, SCREEN_ROW(row), SCREEN_COL(col));
    wrefresh(win);

    for (;;) {
        key = wgetch(win);

        if (strchr("+-Qlkjh", (char)key)) {
            switch (key) {
            case 'Q':
                quit(0);
                break;
            case 'h': /* left */
                if (--col < 0)
                    col = SIZE - 1;
                break;
            case 'l': /* right */
                if (++col == SIZE)
                    col = 0;
                break;
            case 'k': /* up */
                if (--row < 0)
                    row = SIZE - 1;
                break;
            case 'j': /* down */
                if (++row == SIZE)
                    row = 0;
                break;
            default: /* '+', '-' (or a NUL byte, which acts like '-') */
                change_cell(col, row, key == '+' ? 1 : -1);
                draw_board();
                break;
            }
        }

        if (is_solved()) {
            draw_board();
            wrefresh(win);
            quit(0);
        }
        wmove(win, SCREEN_ROW(row), SCREEN_COL(col));
        wrefresh(win);
    }
}
