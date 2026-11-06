#include <stdio.h>

/*
 * Tic-tac-toe computed by a "printf machine".
 *
 * The original program is one huge printf() call. printf() can only do
 * three useful things here:
 *
 *   - print text, which adds to the count of bytes printed so far
 *     (padding "%Nd" adds N, "%s" of a one-byte "bit" adds 0 or 1)
 *   - store that count (mod 256) into a byte of memory ("%hhn")
 *
 * Every piece of game logic below is built only from those operations,
 * exactly in the order the original format string used them, so the
 * bytes written to stdout are identical (including all the "junk" text
 * that the counting produces before the board is drawn).
 *
 * A "bit" is two bytes of memory: "x\0" means true (x != 0) and "\0\0"
 * means false. Printing it with "%s" adds 1 or 0 to the count.
 */

char memory[538] = {1, 0, 10, 0, 10};

/* ---- memory map ------------------------------------------------------ */

char *game_running   = memory + 0;   /* loop while non-zero                */
char *turns_left     = memory + 2;   /* counts 10, 9, ... 0                */
char *move           = memory + 4;   /* square typed by the player (1..9)  */
char *p1_squares     = memory + 4;   /* P1 square i is p1_squares + 2*i    */
char *p2_squares     = memory + 22;  /* P2 square i is p2_squares + 2*i    */
char *temp           = memory + 46;
char *p1_won         = memory + 48;
char *p2_won         = memory + 50;
char *scratch        = memory + 52;
char *is_tie         = memory + 54;
char *illegal_move   = memory + 58;
char *temp2          = memory + 60;
char *screen         = memory + 66;  /* board text followed by status line */
char *scanf_format   = memory + 126; /* "" in round one, then "%hhd"       */

/* screen layout:  "c|c|c\n-----\nc|c|c\n-----\nc|c|c\nP1> " / "P1 WINS\n" */
#define BOARD_ROW_LEN  12
#define STATUS         (screen + 30)
#define STATUS_P       (STATUS + 0)
#define STATUS_DIGIT   (STATUS + 1)
#define STATUS_PROMPT  (STATUS + 2)  /* '>' or ' '          */
#define STATUS_W_OR_T  (STATUS + 3)  /* ' ', 'W' or 'T'     */
#define STATUS_I       (STATUS + 4)  /* 'I', or 0 to end it */
#define STATUS_N_OR_E  (STATUS + 5)
#define STATUS_S       (STATUS + 6)
#define STATUS_END     (STATUS + 7)

static char *p1_square(int i) { return p1_squares + 2 * i; }
static char *p2_square(int i) { return p2_squares + 2 * i; }

static char *board_cell(int i)
{
    return screen + BOARD_ROW_LEN * ((i - 1) / 3) + 2 * ((i - 1) % 3);
}

static const int win_lines[8][3] = {
    {1, 2, 3}, {4, 5, 6}, {7, 8, 9},
    {1, 4, 7}, {2, 5, 8}, {3, 6, 9},
    {1, 5, 9}, {3, 5, 7},
};

/* ---- printf machine primitives -------------------------------------- */

static int printed;                 /* the "%n" count of the current round */

/* "%Nd" of 0: print a number padded to width N, adding N to the count */
static void add(int width)
{
    printed += printf("%*d", width, 0);
}

/* pad so that the count becomes exactly target */
static void count_up_to(int target)
{
    add(target - printed);
}

/* "%.*d" of 0: prints n zeros (nothing when n is 0) */
static void add_zeros(int n)
{
    printed += printf("%.*d", n, 0);
}

/* "%s": adds the length of the string; for a bit that is 0 or 1 */
static void add_bit(char *bit)
{
    printed += printf("%s", bit);
}

/* print the same bit several times: count += times * bit */
static void add_bit_times(char *bit, int times)
{
    while (times--)
        add_bit(bit);
}

/* "%hhn": store the count (mod 256) */
static void store(char *where)
{
    *where = (char)printed;
}

/*
 * Bring the count up to a multiple of 256 so that the next calculation
 * starts at zero. The original repeats "store count; print it" 256 times:
 * every step prints one byte equal to the count until it wraps to 0.
 */
static void reset_count(void)
{
    while (printed % 256 != 0) {
        store(scratch);
        add_bit(scratch);
    }
    store(scratch);
}

/* ---- logic gates built on the counter -------------------------------- */

/* out = !(a && b && c)       (3 + 253 wraps to 0) */
static void nand3(char *a, char *b, char *c, char *out)
{
    add_bit(a);
    add_bit(b);
    add_bit(c);
    add(253);
    store(out);
}

/* out = !in                  (1 + 255 wraps to 0) */
static void not(char *in, char *out)
{
    add_bit(in);
    add(255);
    store(out);
}

/* out = a || b               (just add the bits) */
static void or(char *a, char *b, char *out)
{
    add_bit(a);
    add_bit(b);
    store(out);
}

/* ---- one round of the game ------------------------------------------ */

/* write all the fixed characters of the screen, and mark the move */
static void write_constant_text(char *square_moved)
{
    int i;

    count_up_to('\n');
    store(board_cell(3) + 1);
    store(board_cell(4) - 1);
    store(board_cell(6) + 1);
    store(board_cell(7) - 1);
    store(board_cell(9) + 1);
    store(STATUS_END);
    store(STATUS_PROMPT);
    store(square_moved);           /* square becomes a true bit */

    count_up_to(' ');
    store(STATUS_PROMPT);
    count_up_to('%');
    store(scanf_format + 0);
    count_up_to('-');
    for (i = 0; i < 5; i++)
        store(board_cell(3) + 2 + i);
    for (i = 0; i < 5; i++)
        store(board_cell(6) + 2 + i);
    count_up_to('I');
    store(STATUS_I);
    count_up_to('N');
    store(STATUS_N_OR_E);
    count_up_to('P');
    store(STATUS_P);
    count_up_to('S');
    store(STATUS_S);
    count_up_to('W');
    store(STATUS_W_OR_T);
    count_up_to('d');
    store(scanf_format + 3);
    count_up_to('h');
    store(scanf_format + 1);
    store(scanf_format + 2);
    count_up_to('|');
    for (i = 0; i < 9; i += 3) {
        store(board_cell(i + 1) + 1);
        store(board_cell(i + 2) + 1);
    }
    reset_count();
    reset_count();
}

/* won = won || (all three squares of a line belong to the player) */
static void check_lines(char *(*square)(int), char *won)
{
    int line;

    for (line = 0; line < 8; line++) {
        nand3(square(win_lines[line][0]), square(win_lines[line][1]),
              square(win_lines[line][2]), temp);
        reset_count();
        not(temp, temp);
        reset_count();
        or(temp, won, won);
        add(253);
        store(STATUS_DIGIT);       /* throwaway, rewritten later */
        reset_count();
    }
}

/* turns_left -= 1, then is_tie = no winner && no turns left */
static void count_turn_and_check_tie(int turns_before)
{
    reset_count();
    add_zeros(turns_before);
    add(255);
    store(turns_left);
    reset_count();

    not(turns_left, is_tie);       /* board full */
    reset_count();
    add_bit(p1_won);
    add_bit(p2_won);
    add(255);
    store(temp);                   /* temp = nobody won */
    reset_count();
    add_bit(temp);
    add_bit(is_tie);
    add(254);
    store(is_tie);                 /* !(nobody won && full) */
    reset_count();
    not(is_tie, is_tie);
    reset_count();
}

/* illegal_move = some square belongs to both players */
static void check_illegal_moves(void)
{
    int i;

    for (i = 1; i <= 9; i++) {
        add(254);
        store(temp);               /* throwaway */
        add_bit(p1_square(i));
        add_bit(p2_square(i));
        store(temp2);              /* temp2 = !(p1 && p2) */
        reset_count();
        not(temp2, temp2);
        store(scratch);            /* one step count reset */
        add_bit(scratch);
        or(temp2, illegal_move, illegal_move);
    }
}

/* game_running = !(p1_won || p2_won || is_tie || illegal_move) */
static void check_game_over(void)
{
    add_bit(p1_won);
    add_bit(p2_won);
    add_bit(is_tie);
    add_bit(illegal_move);
    store(temp);
    reset_count();
    not(temp, game_running);
    reset_count();
}

/* build "P1> " or "P1 WINS" / "P2 TIES" and the X/O board */
static void draw_screen(int turns_before)
{
    int i;

    add_bit_times(temp, 'I');
    store(STATUS_I);               /* 'I' when over, else ends the text */
    reset_count();

    printed += printf("%s", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    add_bit_times(game_running, '>' - ' ');
    store(STATUS_PROMPT);          /* '>' while playing, else ' ' */
    reset_count();

    for (i = 1; i <= 9; i++) {
        add_bit_times(p2_square(i), 'O' - ' ');
        add_bit_times(p1_square(i), 'X' - ' ');
        add(' ');
        store(board_cell(i));      /* ' ', 'X' or 'O' */
        reset_count();
    }

    /* temp2 = (128 * turns) mod 256: non-zero when P1 just moved */
    for (i = 0; i < 128; i++)
        add_zeros(turns_before);
    store(temp2);
    reset_count();

    add_bit(is_tie);
    add_bit(p1_won);
    add_bit(p2_won);
    store(temp);                   /* game finished normally */
    reset_count();

    add_bit_times(temp, 255);      /* subtract one if finished */
    add_bit(temp2);
    store(temp2);                  /* true when P2's turn or P2 won */
    reset_count();

    add_bit(temp2);
    add('1');
    store(STATUS_DIGIT);           /* '1' or '2' */
    reset_count();

    add_bit(p1_won);
    add_bit(p2_won);
    add_bit(illegal_move);
    store(temp);                   /* somebody wins */
    reset_count();

    add_bit_times(temp, 'W' - ' ');
    add_bit_times(is_tie, 'T' - ' ');
    add(' ');
    store(STATUS_W_OR_T);          /* ' ', 'W' or 'T' */
    reset_count();

    add_bit_times(is_tie, 'E' - 'N' + 256);
    add('N');
    store(STATUS_N_OR_E);          /* "WINS" or "TIES" */
    reset_count();

    /* clear the terminal and show the screen text */
    printed += printf("%s", "\n\033[2J\n");
    printed += printf("%s", screen);
}

static void play_round(char *square_moved, int turns_before)
{
    printed = 0;
    write_constant_text(square_moved);
    check_lines(p1_square, p1_won);
    check_lines(p2_square, p2_won);
    count_turn_and_check_tie(turns_before);
    check_illegal_moves();
    check_game_over();
    draw_screen(turns_before);
}

int main()
{
    while (*game_running) {
        /* the first round has an empty format so nothing is read */
        scanf(scanf_format, move);

        /* odd turns are P1's, even turns are P2's */
        char *squares = (*turns_left % 2) ? p1_squares : p2_squares;

        play_round(squares + 2 * *move, *turns_left);
    }
}
