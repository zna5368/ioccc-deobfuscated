#include <stdio.h>
#include <unistd.h>

/*
 * Vortex text stirrer.
 *
 * The text is placed on a ROWS x COLS torus (COLS = 2 * ROWS). One stirring
 * step shifts every column, every slope-2 diagonal, every row and every
 * slope-(-2) diagonal by an amount taken from a wave. Running the 32 steps
 * forwards encrypts, running them backwards (in reverse order with negated
 * shifts) decrypts.
 */

#define GRID_SIZE      9999
#define STEPS          32
#define HASH_MULTIPLY  33331LL
#define HASH_MODULUS   33333331LL
#define FRAME_DELAY_US (HASH_MULTIPLY * 2)

long long rows, cols;          /* size of the stirring area */
long long text_seed;           /* hash of the character histogram */
long long password_hash;       /* hash of the optional password */

char grid[GRID_SIZE][GRID_SIZE] = {0};
char saved[GRID_SIZE][GRID_SIZE];

/* how far a line moves in columns per cell travelled along it */
static const int COLUMN_STEP[4] = { 0, 2, 1, -2 };
/* strength of the wave for each kind of line */
static const int WAVE_STRENGTH[4] = { 3, 4, 6, 4 };

enum { LINE_COLUMN, LINE_DIAGONAL, LINE_ROW, LINE_ANTIDIAGONAL };

/*
 * Compute how many cells the given line is shifted during this step. The
 * shift is the sum of two periodic waves (frequencies 4 and 2) whose phase
 * depends on the text seed, the password and the current step, rounded to
 * the nearest whole cell.
 */
static long long wave_shift(int kind, int step, long long line, long long period)
{
    long long seed = text_seed + kind;
    long long sum = 0;

    for (int frequency = 4; frequency >= 2; frequency -= 2) {
        seed = (seed + password_hash) * HASH_MULTIPLY % HASH_MODULUS;
        long long offset = seed % HASH_MULTIPLY;
        long long pos = frequency * line
                      + offset * period * step / HASH_MULTIPLY / 5
                      + period * (period + offset)
                      - period * step / 10;
        long long within = pos % period;
        long long sign = (pos / period % 2) ? 1 : -1;

        /* parabola arcs with alternating sign approximate a sine wave */
        sum += within * (period - within) * sign * WAVE_STRENGTH[kind];
    }

    /* divide by period^2, rounding half away from zero */
    return (sum + period * period / (sum > 0 ? 2 : -2)) / period / period;
}

/* Perform one stirring step forwards (direction 1) or backwards (-1). */
static void stir(int step, int direction)
{
    int first = direction > 0 ? LINE_COLUMN : LINE_ANTIDIAGONAL;
    int stop = direction > 0 ? LINE_ANTIDIAGONAL + 1 : LINE_COLUMN - 1;

    for (int kind = first; kind != stop; kind += direction) {
        /* columns and diagonals move down one row per cell; rows do not */
        int crosses_rows = (kind != LINE_ROW);
        long long cells = rows * cols;
        long long line_count = crosses_rows ? cols : rows;
        long long line_length = crosses_rows ? rows : cols;
        long long col_step = COLUMN_STEP[kind] * direction;
        long long row_step = crosses_rows ? direction : 0;

        for (long long r = 0; r < rows; r++)
            for (long long c = 0; c < cols; c++)
                saved[r][c] = grid[r][c];

        for (long long line = 0; line < line_count; line++) {
            long long shift = wave_shift(kind, step, line, line_count);

            for (long long pos = 0; pos < line_length; pos++) {
                long long row, col;

                if (crosses_rows) {
                    row = direction * pos;
                    col = line + col_step * pos;
                } else {
                    row = line;
                    col = direction * pos;
                }
                /* keep the numbers positive for the modulus */
                row += 4 * cells;
                col += 4 * cells;

                grid[row % rows][col % cols] =
                    saved[(row + row_step * shift) % rows][(col + col_step * shift) % cols];
            }
        }
    }
}

/* Replace empty cells of the stirring area with spaces. */
static void fill_blanks(void)
{
    for (long long r = 0; r < rows; r++)
        for (long long c = 0; c < cols; c++)
            if (grid[r][c] == 0)
                grid[r][c] = ' ';
}

/* Remove trailing spaces from every line of the grid. */
static void trim_lines(void)
{
    for (int r = GRID_SIZE - 1; r >= 0; r--) {
        char *end = grid[r];
        while (*end)
            end++;
        while (end > grid[r] && end[-1] == ' ')
            *--end = 0;
    }
}

/*
 * Score how tidy the stirring area looks: count the cells (outside the first
 * column) that do not start a new word. Readable text has few word starts
 * compared with stirred text, where letters are scattered among spaces.
 * Masking with 95 makes a space (and a NUL) count as blank.
 */
static int tidiness(void)
{
    int score = 0, previous_blank = 1;

    for (int i = 0; i < rows * cols; i++) {
        int column = i % cols;
        int blank = (grid[i / cols][column] & 95) == 0;
        int word_start = previous_blank && !blank;

        if (column != 0 && !word_start)
            score++;
        previous_blank = blank;
    }
    return score;
}

int main(int argc, char **argv)
{
    int histogram[256] = {0};
    int line = 0, column = 0;
    int ch;

    rows = 40;
    text_seed = password_hash = 0;

    /* optional SIZE argument followed by optional PASSWORD */
    argc--;
    argv++;
    if (argc && sscanf(*argv, "%lld", &rows)) {
        argc--;
        argv++;
    }
    if (argc) {
        for (char *p = *argv; *p; p++)
            password_hash = (password_hash * HASH_MULTIPLY + *p) % HASH_MODULUS;
    }

    /* read the text into the grid */
    while ((ch = getchar()) >= 0) {
        if (ch == '\n') {
            line++;
            column = 0;
        } else {
            grid[line][column++] = ch;
        }
        histogram[ch]++;
    }
    cols = 2 * rows;
    fill_blanks();

    /* the histogram is unchanged by stirring, so the seed works both ways */
    for (int c = '!'; c <= '~'; c++)
        text_seed = (3331 * text_seed + histogram[c]) % HASH_MULTIPLY;

    /* try stirring forwards, then backwards, and compare the results */
    for (int step = 0; step < STEPS; step++)
        stir(step, 1);
    int forward_score = tidiness();
    for (int step = STEPS - 1; step >= 0; step--)
        stir(step, -1);

    for (int step = STEPS - 1; step >= 0; step--)
        stir(step, -1);
    int backward_score = tidiness();
    for (int step = 0; step < STEPS; step++)
        stir(step, 1);

    int go_backward = forward_score - backward_score < 0;

    /* animate the chosen direction, printing the final result */
    for (int frame = 0; frame <= STEPS; frame++) {
        trim_lines();

        int last = GRID_SIZE - 1;
        while (last && !grid[last][0])
            last--;

        if (frame == STEPS) {
            fprintf(stderr, "%s", "\033[H");
            for (int r = 0; r <= last; r++)
                puts(grid[r]);
        }

        fprintf(stderr, frame ? "%s" : "%s\033[2J", "\033[H");
        for (int r = 0; r < line || r < rows; r++)
            fprintf(stderr, "%s\033[K\n", grid[r]);

        fill_blanks();
        if (frame != STEPS) {
            if (go_backward)
                stir(STEPS - 1 - frame, -1);
            else
                stir(frame, 1);
        }
        usleep(FRAME_DELAY_US);
    }
}
