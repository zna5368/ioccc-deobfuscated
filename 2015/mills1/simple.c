#include <curses.h>

/*
 * Game settings.  The Makefile passes these to the compiler as one letter
 * macros (-DU=99999 and so on), so they can still be changed the same way as
 * for the original program.  The defaults are shown on the right.
 */
#define STEP_DELAY_USEC   U /* 99999: pause between steps */
#define PIPE_PERIOD       W /* 28: columns from one pipe to the next */
#define PIPE_SPACE        D /* 20: empty columns between pipes */
#define GAP_HEIGHT        H /* 8: rows from the top to the bottom of a gap */
#define GRAVITY           A /* 100: added to the velocity on every step */
#define FLAP_VELOCITY     I /* -200: velocity right after a flap */
#define GLIDE_SPRITE      B /* "<o^=" */
#define FLAP_SPRITE       F /* "<ov=" */
#define INSTRUCTIONS      G /* "Tap to Flap!" */

/* Vertical position and velocity are stored in units of 1/256 of a row */
#define SUBROW_BITS 8

#define BLANK_SPRITE      "    "
#define SPRITE_LAST_COLUMN 3   /* offset of the last column of the sprite */
#define INSTRUCTIONS_ROW   2
#define SCORE_ROW          0
#define SCORE_WIDTH        9   /* columns reserved at the right of SCORE_ROW */
#define GAP_MIN_TOP        2   /* highest row a gap may start at */
#define GAP_ROWS_RESERVED  6   /* rows a gap may not use, top and bottom */

/*
 * Counts the columns of the scrolling world.  It is negative while a pipe is
 * entering at the left edge and goes from 0 up to PIPE_SPACE - 1 in between.
 */
int pipe_timer;
int gap_top;          /* row of the upper edge of the current gap */
int bird_velocity;
int bird_column;
int bird_height;      /* in units of 1/256 of a row, 0 is the top */
/*
 * Starts at the bird's column and goes down by one every step.  Once it is
 * below zero, each further PIPE_PERIOD steps is one more pipe the bird cleared.
 */
int score_countdown;
int best_score;

/* Picks the character for one row of the column entering at the left edge */
static int next_column_cell(int row)
{
    int gap_bottom = gap_top + GAP_HEIGHT;

    if (pipe_timer >= 0 || (row > gap_top && row < gap_bottom))
        return ' ';
    if (row < gap_top || row > gap_bottom)
        return '|';
    return '=';
}

/* Inserting a character at column 0 pushes that whole row to the right */
static void scroll_world_right(void)
{
    int row;

    for (row = LINES - 1; row >= 0; row--)
        mvinsch(row, 0, next_column_cell(row));
}

/* The bird dies below the screen or if either end of it touches something */
static int bird_has_crashed(void)
{
    int row = bird_height >> SUBROW_BITS;

    if (row >= LINES)
        return 1;
    if (row < 0)
        row = 0;   /* Above the screen it is checked against the top row */
    return mvinch(row, bird_column) != ' '
        || mvinch(row, bird_column + SPRITE_LAST_COLUMN) != ' ';
}

/* A point is earned for each pipe that has scrolled past the bird */
static void update_score(void)
{
    int score;

    score_countdown--;
    score = score_countdown / -PIPE_PERIOD;
    if (score < 0)
        score = 0;
    if (best_score < score)
        best_score = score;
    mvprintw(SCORE_ROW, COLS - SCORE_WIDTH, " %u/%u ", score, best_score);
}

/* Plays one round, returning when the bird crashes */
static void play_round(void)
{
    bird_height = 0;
    bird_velocity = 0;
    pipe_timer = 0;
    timeout(0);   /* getch() must not wait for a key */
    clear();
    mvaddstr(INSTRUCTIONS_ROW, bird_column + SPRITE_LAST_COLUMN, INSTRUCTIONS);

    for (;;) {
        mvaddstr(bird_height >> SUBROW_BITS, bird_column, BLANK_SPRITE);
        scroll_world_right();

        if (getch() > 0)
            bird_velocity = FLAP_VELOCITY;
        else
            bird_velocity += GRAVITY;
        bird_height += bird_velocity;

        if (bird_has_crashed())
            return;

        mvaddstr(bird_height >> SUBROW_BITS, bird_column,
                 bird_velocity < 0 ? FLAP_SPRITE : GLIDE_SPRITE);
        update_score();
        refresh();

        pipe_timer++;
        if (pipe_timer == PIPE_SPACE) {
            pipe_timer -= PIPE_PERIOD;
            gap_top = rand() % (LINES - GAP_HEIGHT - GAP_ROWS_RESERVED)
                    + GAP_MIN_TOP;
        }
        usleep(STEP_DELAY_USEC);
    }
}

int main()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    /* The game restarts forever, unless the terminal has no middle column */
    for (;;) {
        bird_column = COLS / 2;
        score_countdown = bird_column;
        if (bird_column == 0)
            break;
        play_round();
        flash();
        flushinp();
    }
}
