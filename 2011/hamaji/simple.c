#include <stdint.h>
#include <stdio.h>

/*
 * Nonogram solver.
 *
 * Every line (60 rows followed by 60 columns) is turned into a small state
 * machine: state 0 is the leading space, then one state per box of each clue
 * block, each block followed by one space state.  For every cell we keep a
 * 64-bit set of the states that the line could be in at that cell.
 *
 * The sets are stored in NEGATIVE logic: a 1 bit means "this state is NOT
 * possible here".  That way the zero-initialised global buffer starts out as
 * "everything is possible".
 */

#define MAX_SIZE 60                       /* max width / height */
#define NUM_LINES (2 * MAX_SIZE)          /* rows 0..59, columns 60..119 */
#define NUM_CELLS (NUM_LINES * MAX_SIZE)  /* one entry per (line, position) */
#define GUESS_OFFSET (2 * NUM_CELLS)      /* scratch buffers used for guessing */

/* values stored in board[][] */
enum { UNKNOWN, BOX, SPACE };

/* results of a solving pass */
enum { UNSOLVED, CHANGED, CONTRADICTION, SOLVED };

char clue_text[NUM_LINES][MAX_SIZE]; /* clues printed as "3.1.2 " */
int clues[NUM_LINES][MAX_SIZE];      /* zero-terminated clue lists */
int clue_len[NUM_LINES];             /* strlen of clue_text */
int width, height;
int board[MAX_SIZE + 1][MAX_SIZE + 1];
int status;

int64_t box_states[NUM_LINES];   /* states of a line that are boxes */
int64_t space_states[NUM_LINES]; /* states of a line that are spaces */

/*
 * Two buffers of cell state sets are used in turn so that a pass can be
 * compared with the previous one.  Two more buffers (GUESS_OFFSET) are used
 * while trying out guesses.
 */
int64_t buffers[4 * NUM_CELLS];
int64_t *cells = buffers;
int buffer_step = NUM_CELLS;

/* Copy src into cells, noting contradictions and changes in status. */
void copy_cells(int64_t *src)
{
    int n;

    for (n = 0; n < NUM_CELLS; n++) {
        int64_t possible = src[n];

        if (~possible == 0 && clues[n / MAX_SIZE][0])
            status = CONTRADICTION; /* no state left for this cell */
        else if (cells[n] != possible)
            status = CHANGED;
        cells[n] = possible;
    }
}

/* Repeat line solving until nothing changes any more. */
void solve_lines(void)
{
    int line, pos, n, num_states, cell, cross;
    int64_t reach, spaces, *keep, *previous;

    for (status = CHANGED; status == CHANGED;) {
        status = SOLVED;

        for (line = 0; line < NUM_LINES; line++) {
            /* build the state machine of this line */
            num_states = 1;
            spaces = 1;
            for (n = 0; clues[line][n]; n++) {
                int block = clues[line][n];

                box_states[line] |= ((1LL << block) - 1) << num_states;
                num_states += block;
                spaces |= 1LL << num_states;
                space_states[line] = spaces;
                num_states++;
            }

            /* backward pass: line must end in the last block or trailing space */
            reach = ~(3LL << (num_states - 2));
            for (pos = MAX_SIZE - 1; pos >= 0; pos--) {
                reach = cells[MAX_SIZE * line + pos] |= reach;
                /* a state can be reached from the next state, or a space can repeat */
                reach = (reach | ~spaces) & (reach >> 1);
            }

            /* forward pass: line must start with a space or the first block */
            reach = ~3LL;
            for (pos = 0; pos < MAX_SIZE; pos++) {
                reach = cells[MAX_SIZE * line + pos] |= reach;
                reach = (reach | ~spaces) & (reach << 1 | 1);
            }
        }

        /* decide cells and pass the result on to the crossing line */
        for (cell = 0; cell < NUM_CELLS; cell++) {
            int64_t possible = ~cells[cell];
            int *mark;

            line = cell / MAX_SIZE;
            pos = cell % MAX_SIZE;
            mark = line < MAX_SIZE ? &board[line][pos] : &board[pos][line - MAX_SIZE];

            if (possible & box_states[line]) {
                if (possible & space_states[line]) {
                    *mark = status = UNSOLVED;
                    continue;
                }
                *mark = BOX;
                keep = box_states;
            } else {
                *mark = SPACE;
                keep = space_states;
            }
            cross = line < MAX_SIZE ? pos + MAX_SIZE : pos;
            cells[line % MAX_SIZE + MAX_SIZE * cross] |= ~keep[cross];
        }

        /* switch to the other buffer and check what changed */
        previous = cells;
        buffer_step = -buffer_step;
        cells -= buffer_step;
        copy_cells(previous);
    }
}

int main()
{
    int *count = &height;
    int section = 0, n = 0, expected, cell, state, row, col, len, x;
    int row_clue_width = 1, col_clue_height = 1;

    /* read row clues, a "-" line, then column clues */
    for (;;) {
        if (scanf("%d", clues[*count + section * MAX_SIZE] + n) < 1) {
            count = &width;
            n--;
            if (++section >= 2)
                break;
            expected = 1;
        } else {
            expected = '\n';
        }
        if (getchar() != expected) {
            n++;
        } else {
            n = 0;
            ++*count;
        }
    }

    solve_lines();

    /* try every still possible state of each unknown cell */
    if (status == UNSOLVED)
        for (cell = 0; cell < width * MAX_SIZE; cell++)
            if (cell % MAX_SIZE < width && !board[cell / MAX_SIZE][cell % MAX_SIZE]) {
                for (state = 64; state--;)
                    if (!(1 & cells[cell] >> state)) {
                        int64_t *saved = cells;

                        cells += GUESS_OFFSET;
                        copy_cells(saved);
                        cells[cell] = ~(1LL << state);
                        solve_lines();
                        cells -= GUESS_OFFSET;
                        if (status == CONTRADICTION)
                            cells[cell] |= 1LL << state;
                    }
                solve_lines();
            }

    /* turn the clues into text like "3.1.2 " */
    for (row = 0; row < NUM_LINES; row++) {
        len = 0;
        for (n = 0; (x = clues[row][n]); n++)
            len += sprintf(clue_text[row] + len, "%d.", x);
        clue_len[row] = len;
        if (row < MAX_SIZE) {
            if (row_clue_width < len)
                row_clue_width = len;
        } else if (col_clue_height < len) {
            col_clue_height = len;
        }
        clue_text[row][len - 1] = ' ';
    }

    if (status == CONTRADICTION)
        puts("invalid");
    else if (status == UNSOLVED)
        puts("failed");

    for (row = -col_clue_height; row < height; row++) {
        for (col = -row_clue_width; col < width; col++) {
            if (row < 0) {
                /* column clues written top to bottom */
                if (col < 0 || (len = row + clue_len[col + MAX_SIZE]) < 0)
                    putchar(' ');
                else
                    putchar(clue_text[col + MAX_SIZE][len]);
            } else if (col < 0) {
                /* row clues written left to right */
                if ((len = col + clue_len[row]) < 0)
                    putchar(' ');
                else
                    putchar(clue_text[row][len]);
            } else {
                putchar("?X "[board[row][col]]);
            }
        }
        puts("");
    }
    return 0;
}
