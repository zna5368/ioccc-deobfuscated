#include <time.h>
#include <ncurses.h>
#include <stdlib.h>

/*
 * Bits of a cell value in cells[].
 * A covered cell is COVERED (plus MINE if it hides a mine), a probed cell has
 * neither COVERED nor FLAGGED, and a flagged cell has FLAGGED.
 */
#define MINE    0x0001  /* the cell contains a mine */
#define COVERED 0x0010  /* the cell has not been probed yet */
#define FLAGGED 0x0100  /* the cell is known to be a mine */
#define MARK    0x1000  /* temporary mark used by the solver */

/*
 * scan_block() adds up the values of the (up to) 3x3 cells around a cell.
 * Every bit above sits in its own 4 bit field, so the sum holds four counters
 * at once.
 */
#define MINES_IN(sum)   ((sum) % 16)         /* mines in the block */
#define COVERED_IN(sum) ((sum) / 16 % 16)    /* covered cells in the block */
#define FLAGS_IN(sum)   ((sum) / 256 % 16)   /* flagged cells in the block */
#define MARKED_IN(sum)  ((sum) / 4096)       /* marked cells in the block */

enum { READY, PLAYING, LOST };

static int *cells;         /* state of each cell (holds priorities before the first click) */
static int *scan_order;    /* scan_order[priority] = cell index */
static int width, height, cell_count, mine_count;
static int state;          /* READY before the first click, then PLAYING or LOST */
static int scan_pos;       /* where the solver continues in scan_order */
static int timer;          /* -start time while running, elapsed seconds when stopped */
static char cell_text[] = " x";

/* color pairs: index + 1 is the pair number used with COLOR_PAIR() */
static const short pair_colors[12][2] = {
    { COLOR_WHITE,   COLOR_RED   },  /* 1: mine '*' after losing */
    { COLOR_RED,     COLOR_WHITE },  /* 2: flag '!' */
    { COLOR_BLACK,   COLOR_WHITE },  /* 3: covered cell */
    { COLOR_WHITE,   COLOR_BLACK },  /* 4: empty probed cell, size message */
    { COLOR_BLUE,    COLOR_BLACK },  /* 5: '1' */
    { COLOR_GREEN,   COLOR_BLACK },  /* 6: '2' */
    { COLOR_RED,     COLOR_BLACK },  /* 7: '3', mine counter and timer */
    { COLOR_MAGENTA, COLOR_BLACK },  /* 8: '4' */
    { COLOR_YELLOW,  COLOR_BLACK },  /* 9: '5', smiley face */
    { COLOR_CYAN,    COLOR_BLACK },  /* 10: '6' */
    { COLOR_WHITE,   COLOR_BLACK },  /* 11: '7' */
    { COLOR_WHITE,   COLOR_BLACK },  /* 12: '8' */
};

/*
 * Toggle the MARK bit of every covered neighbor of center and return the sum
 * of all the cell values in the 3x3 block (center included).
 * Calling it twice on the same cell removes its marks again; calling it on two
 * cells leaves marks on the covered cells next to exactly one of them.
 */
static int scan_block(int center)
{
    int sum = 0, row, col, cell;
    int center_row = center / width, center_col = center % width;

    for (row = center_row > 0 ? center_row - 1 : 0;
         row < height && row < center_row + 2; row++) {
        for (col = center_col > 0 ? center_col - 1 : 0;
             col < width && col < center_col + 2; col++) {
            cell = row * width + col;
            if (cell != center)
                cells[cell] ^= (cells[cell] & COVERED) << 8;   /* COVERED -> MARK */
            sum += cells[cell];
        }
    }
    return sum;
}

/*
 * Compare probed cell a with probed cell b (b_sum == 0 compares a with an
 * empty block, which gives the simple single cell rules).
 * Marked cells next to a are the covered cells next to a but not next to b.
 * If they are all safe they are probed, if they are all mines they are
 * flagged. Returns 1 if anything was changed.
 */
static int deduce(int a, int a_sum, int b_sum)
{
    /* mines still to find around b minus mines still to find around a */
    int diff = (MINES_IN(b_sum) + FLAGS_IN(a_sum)) % 16
             - (MINES_IN(a_sum) + FLAGS_IN(b_sum)) % 16;
    /* covered cells next to b but not next to a */
    int only_b = MARKED_IN(b_sum);
    int new_bits, changed = 0, row, col, cell;
    int a_row = a / width, a_col = a % width;

    if (diff == only_b)
        new_bits = 0;          /* b's own cells hold all the extra mines: probe */
    else if (diff == COVERED_IN(b_sum) - COVERED_IN(a_sum) - only_b)
        new_bits = FLAGGED;    /* a's own cells must all be mines: flag */
    else
        return 0;

    for (row = a_row > 0 ? a_row - 1 : 0; row < height && row < a_row + 2; row++) {
        for (col = a_col > 0 ? a_col - 1 : 0; col < width && col < a_col + 2; col++) {
            cell = row * width + col;
            if ((cells[cell] & (MARK | FLAGGED | COVERED)) == (MARK | COVERED)) {
                cells[cell] = (cells[cell] & ~COVERED) | new_bits;
                changed = 1;
            }
        }
    }
    return changed;
}

/* Try the probed cells in priority order and apply the first deduction found. */
static void solve_step(void)
{
    int stop = (scan_pos + cell_count - 1) % cell_count;
    int cell, other, cell_sum, row, col, cell_row, cell_col;

    for (; state == PLAYING && stop != scan_pos; scan_pos = (scan_pos + 1) % cell_count) {
        cell = scan_order[scan_pos];
        if (cells[cell] & (COVERED | FLAGGED))
            continue;

        /* single cell rules */
        cell_sum = scan_block(cell);
        if (deduce(cell, cell_sum, 0))
            return;

        /* two cell rules with every probed cell in the 5x5 block */
        cell_row = cell / width;
        cell_col = cell % width;
        for (row = cell_row - 2 < 0 ? 0 : cell_row - 2;
             row < cell_row + 3 && row < height; row++) {
            for (col = cell_col - 2 < 0 ? 0 : cell_col - 2;
                 col < width && col < cell_col + 3; col++) {
                other = row * width + col;
                if (!(cells[other] & (COVERED | FLAGGED))) {
                    if (deduce(cell, cell_sum, scan_block(other)))
                        return;
                    scan_block(other);   /* remove the marks of other */
                }
            }
        }
        scan_block(cell);                /* remove the marks of cell */
    }
}

static void draw_board(void)
{
    int cell, kind, now, shown_time;
    int covered_shown = 0, mines_left = mine_count, covered_count = 0, last_covered = 0;

    if (LINES - 1 < height || COLS / 2 < width) {
        clear();
        attrset(COLOR_PAIR(4));
        mvprintw(LINES / 2, COLS / 2 - 16, "Make the terminal bigger!");
    } else {
        for (cell = 0; cell < cell_count; cell++) {
            if (state == READY) {
                kind = 3;
            } else if (cells[cell] & FLAGGED) {
                mines_left--;
                kind = 2;
            } else if (state == LOST && cells[cell] & MINE) {
                kind = 1;
            } else if (cells[cell] & COVERED) {
                last_covered = cell;
                covered_count++;
                kind = 3;
            } else {
                kind = 4 + MINES_IN(scan_block(cell));  /* marks are cleared later */
            }
            /* 1 '*' mine, 2 '!' flag, 3 '.' covered, 4 '.' empty, 5.. '1'-'8' */
            cell_text[1] = "_*!..12345678"[kind];
            covered_shown += kind == 3;
            attrset(COLOR_PAIR(kind));
            mvprintw(cell / width + 1, cell % width * 2, "%s", cell_text);
        }

        now = timer + time(0);
        if (!covered_shown && timer < 0 && state == PLAYING)
            timer = now;                 /* every cell is done: stop the clock */
        shown_time = timer < 0 ? now : timer;

        attrset(COLOR_PAIR(7));
        mvprintw(0, 0, "%03d%*s%03d", mines_left > 999 ? 999 : mines_left,
                 width * 2 - 6, "", shown_time > 999 ? 999 : shown_time);

        attrset(COLOR_PAIR(9));
        if (state == LOST)
            mvprintw(0, width - 1, "X-(");
        else if (state == PLAYING && !covered_shown)
            mvprintw(0, width - 1, "8-)");
        else
            mvprintw(0, width - 1, ":-)");

        /* as many covered cells as mines left: they are all mines */
        if (mines_left == covered_count && mines_left)
            cells[last_covered] |= FLAGGED;
    }
    refresh();
}

/* first click: turn priorities into mines, keeping the clicked cell safe */
static void place_mines(int clicked)
{
    int i;

    for (i = 0; i < cell_count; i++) {
        scan_order[cells[i]] = i;
        cells[i] = COVERED | (cells[i] < mine_count);
    }
    mine_count -= cells[clicked] & MINE;
    cells[clicked] = COVERED;
}

int main(int argc, char **argv)
{
    MEVENT event;
    FILE *file;
    int i, j, swap, key, cell, pair;

    srand(time(0));
    initscr();
    start_color();
    for (pair = 0; pair < 12; pair++)
        init_pair(pair + 1, pair_colors[pair][0], pair_colors[pair][1]);
    noecho();
    cbreak();
    timeout(9);
    curs_set(0);
    keypad(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED | BUTTON1_RELEASED, 0);

    for (;;) {
        /* set up a new game */
        if (argc < 2) {
            file = 0;
            width = COLS / 2;
            height = LINES - 1;
            mine_count = width * height / 5;
        } else {
            file = fopen(argv[argc - 1], "r");
            if (fscanf(file, "%d %d %d", &width, &height, &mine_count) > 3) {
                /* never true: only uses the return value */
            }
        }
        cell_count = width * height;
        cells = realloc(cells, cell_count * sizeof(int));
        scan_order = realloc(scan_order, cell_count * sizeof(int));

        /* give each cell a priority: random shuffle or read from the map */
        for (i = 0; i < cell_count; i++) {
            if (!file) {
                cells[i] = i;
                if (i) {
                    j = rand() % i;
                    swap = cells[j];
                    cells[j] = cells[i];
                    cells[i] = swap;
                }
            } else {
                if (fscanf(file, "%d", &cells[i])) {
                }
            }
        }
        if (file)
            fclose(file);
        timer = state = scan_pos = 0;

        clear();
        for (;;) {
            draw_board();
            key = getch();
            if (key == 'r' || (key == KEY_RESIZE && state == READY))
                break;
            if (key == 'q') {
                endwin();
                return 0;
            }
            if (key == KEY_MOUSE && getmouse(&event) == OK &&
                event.x / 2 < width && event.y <= height) {
                if (event.y == 0 && width - 2 < event.x && event.x < width + 2)
                    break;               /* clicked the smiley face */
                cell = event.x / 2 + (event.y - 1) * width;
                if (cell >= 0) {
                    if (state == READY) {
                        place_mines(cell);
                        state = PLAYING;
                        timer = -time(0);
                    }
                    if (state == PLAYING) {
                        if ((cells[cell] & (FLAGGED | MINE)) == MINE) {
                            timer += time(0);
                            state = LOST;
                            cells[cell] &= FLAGGED | COVERED | MINE;
                        } else {
                            cells[cell] &= FLAGGED | MINE;   /* probe it */
                        }
                    }
                }
            }
            if (state == PLAYING) {
                for (cell = 0; cell < cell_count; cell++)
                    cells[cell] &= FLAGGED | COVERED | MINE;   /* clear marks */
                solve_step();
            }
        }
    }
}
