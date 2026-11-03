#include <curses.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_SIZE 99     /* board array dimension */
#define MIN_SIZE 6      /* smallest allowed board width/height */
#define DEFAULT_WIDTH 15
#define DEFAULT_HEIGHT 12
#define SCREEN_OFFSET 3 /* screen row/column of board cell (0,0) */
#define EMPTY ' '
#define MARK 9          /* added to a letter that is part of a spelled POOT */
#define MARK_SHOWN '*'  /* how marked letters are drawn */

static const char *WORD = "POOT";

/* board[column][row]; holds EMPTY, a letter, or a letter + MARK */
static char board[MAX_SIZE][MAX_SIZE];
static int width, height;
static int score;

/* Draw a character at a board coordinate (may be outside the board). */
static void put(int row, int col, int ch)
{
    move(row + SCREEN_OFFSET, col + SCREEN_OFFSET);
    addch(ch);
}

/* Letters are picked from "POOT", so O is twice as likely as P or T. */
static char random_letter(void)
{
    return WORD[random() % 4];
}

/* A letter that has been marked has a value above 'T', the largest letter. */
static int is_marked(char cell)
{
    return cell > 'T';
}

/*
 * The current letter sits somewhere on a loop around the board.  Position
 * pos runs clockwise from 0 to 2*(width+height)-1:
 *   top side    0 .. width-1                 (left to right)
 *   right side  width .. width+height-1      (top to bottom)
 *   bottom side .. 2*width+height-1          (right to left)
 *   left side   .. 2*(width+height)-1        (bottom to top)
 * Work out the board cell the letter enters at, and the direction pointing
 * from that side into the board.
 */
static void entry_point(int pos, int *col, int *row, int *in_dx, int *in_dy)
{
    int perimeter = 2 * (width + height);

    if (pos < width) {                         /* top */
        *col = pos;
        *row = 0;
        *in_dx = 0;
        *in_dy = 1;
    } else if (pos < width + height) {         /* right */
        *col = width - 1;
        *row = pos - width;
        *in_dx = -1;
        *in_dy = 0;
    } else if (pos < 2 * width + height) {     /* bottom */
        *col = 2 * width + height - 1 - pos;
        *row = height - 1;
        *in_dx = 0;
        *in_dy = -1;
    } else {                                   /* left */
        *col = 0;
        *row = perimeter - 1 - pos;
        *in_dx = 1;
        *in_dy = 0;
    }
}

/* Draw ch where the current letter waits: two cells outside its entry cell. */
static void draw_cursor(int pos, int ch)
{
    int col, row, in_dx, in_dy;

    entry_point(pos, &col, &row, &in_dx, &in_dy);
    put(row - 2 * in_dy, col - 2 * in_dx, ch);
}

/* Redraw the board and score, then wipe the echoed keypress on row 0. */
static void redraw(void)
{
    int col, row;

    for (col = 0; col < width; col++)
        for (row = 0; row < height; row++)
            put(row, col, is_marked(board[col][row]) ? MARK_SHOWN
                                                     : board[col][row]);
    move(height + SCREEN_OFFSET + 2, 2);
    clrtoeol();
    printw("score: %d", score);
    move(0, 0);
    clrtoeol();
    refresh();
}

/*
 * Sideways drift added to the inward direction:
 * P goes clockwise, T goes counterclockwise, O goes straight.
 */
static int drift(char letter)
{
    switch (letter) {
    case 'P':
        return 1;
    case 'T':
        return -1;
    default:
        return 0;
    }
}

/* Bounce off a side wall; a letter moving straight sideways falls instead. */
static void bounce_side(int *dx, int *dy)
{
    if (*dy == 0) {
        *dx = 0;
        *dy = 1;
    } else {
        *dx = -*dx;
    }
}

/* Does WORD read from (col,row) in direction (dx,dy)? Marked letters count. */
static int spells_word(int col, int row, int dx, int dy)
{
    int i, c, r;

    for (i = 0; i < 4; i++) {
        c = col + i * dx;
        r = row + i * dy;
        if (c < 0 || c >= width || r < 0 || r >= height)
            return 0;
        if (board[c][r] != WORD[i] && board[c][r] - MARK != WORD[i])
            return 0;
    }
    return 1;
}

/* Mark every spelled POOT. Returns the bonus: 1 doubled per marked letter. */
static int mark_words(void)
{
    int col, row, dx, dy, i;
    int bonus = 1;

    for (col = 0; col < width; col++)
        for (row = 0; row < height; row++)
            for (dx = -1; dx <= 1; dx++)
                for (dy = -1; dy <= 1; dy++)
                    if (spells_word(col, row, dx, dy))
                        for (i = 0; i < 4; i++) {
                            board[col + i * dx][row + i * dy] += MARK;
                            bonus *= 2;
                        }
    return bonus;
}

/* Remove marked letters, letting everything above them fall down. */
static void collapse(void)
{
    int col, row, found;

    for (col = 0; col < width; col++)
        do {
            found = 0;
            for (row = height - 1; row >= 0; row--) {
                if (is_marked(board[col][row]))
                    found = 1;
                if (found)
                    board[col][row] = row > 0 ? board[col][row - 1] : EMPTY;
            }
        } while (found);
}

/*
 * Drop the letter at pos into the board.  Returns 0 if the entry cell is
 * already occupied.
 */
static int drop(int pos, char letter)
{
    int col, row, dx, dy, in_dx, in_dy, spin;

    entry_point(pos, &col, &row, &in_dx, &in_dy);
    if (board[col][row] != EMPTY)
        return 0;

    spin = drift(letter);
    dx = in_dx + spin * in_dy;
    dy = in_dy - spin * in_dx;

    draw_cursor(pos, EMPTY);
    for (;;) {
        put(row, col, EMPTY);
        if (col == 0 && dx < 0)
            bounce_side(&dx, &dy);
        if (row == 0 && dy < 0)
            dy = -dy;
        if (col == width - 1 && dx > 0)
            bounce_side(&dx, &dy);
        /* stop on the floor or when the next cell is taken */
        if ((row == height - 1 && dy > 0) || board[col + dx][row + dy] != EMPTY)
            break;
        col += dx;
        row += dy;
        put(row, col, letter);
        move(0, 0);
        refresh();
        usleep(5000);
    }
    board[col][row] = letter;
    return 1;
}

int main(int argc, char *argv[])
{
    char letter;
    int pos, col, row, bonus;

    width = height = pos = score = 0;
    if (argc == 3) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }
    if (width < MIN_SIZE || width > MAX_SIZE - 1 ||
        height > MAX_SIZE - 1 || height < MIN_SIZE) {
        width = DEFAULT_WIDTH;
        height = DEFAULT_HEIGHT;
    }

    initscr();
    for (col = 0; col < width; col++)
        for (row = 0; row < height; row++)
            board[col][row] = EMPTY;
    for (col = 0; col < width; col++) {
        put(-1, col, '-');
        put(height, col, '-');
    }
    for (row = 0; row < height; row++) {
        put(row, -1, '|');
        put(row, width, '|');
    }
    put(-1, -1, '+');
    put(-1, width, '+');
    put(height, -1, '+');
    put(height, width, '+');

    letter = random_letter();
    draw_cursor(pos, letter);
    redraw();

    for (;;) {
        switch (getch()) {
        case 'a': /* counterclockwise */
            draw_cursor(pos, EMPTY);
            if (--pos < 0)
                pos = 2 * (width + height) - 1;
            break;
        case 's': /* clockwise */
            draw_cursor(pos, EMPTY);
            if (++pos > 2 * (width + height) - 1)
                pos = 0;
            break;
        case 'd': /* drop */
            if (!drop(pos, letter))
                break;
            ++score;
            letter = random_letter();
            pos = 0;
            /* clear spelled words until no chain reaction remains */
            while ((bonus = mark_words()) > 1) {
                redraw();
                sleep(1);
                score += bonus;
                collapse();
                redraw();
            }
            break;
        }
        draw_cursor(pos, letter);
        redraw();
    }
}
