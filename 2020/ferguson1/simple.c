#include <curses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

/*
 * The colours WALL, WB (wall background), HEAD, HB (head background),
 * BODY and BS (body background) are passed in by the Makefile with -D so
 * that snake-colours.sh and play.sh can pick different colours.
 */

/* colour pair numbers */
#define PAIR_WALL 1
#define PAIR_HEAD 2
#define PAIR_BODY 3

/* direction the snake is travelling (0 = not moving yet) */
enum { NONE, LEFT, RIGHT, UP, DOWN };

/* settings that can be changed with environment variables */
static size_t max_size = 997;           /* MAXSIZE */
static size_t size = 5;                 /* SIZE: current length */
static size_t grow = 5;                 /* GROW: growth per bug eaten */
static size_t shed_amount = 5;          /* SHEDS: growth every SHED moves */
static unsigned long shed_interval;     /* SHED */
static unsigned long walls_passable = 1;/* WALLS */
static unsigned long cannibal;          /* CANNIBAL */
static unsigned long evade = 300;       /* EVADE: moves before the bug runs */
static int wait_time = 231;             /* WAIT: getch() timeout */

/* counters */
static size_t bugs_eaten;
static unsigned long evade_counter;
static unsigned long shed_counter;

/* field dimensions: the last row/column inside the play area */
static int max_x, max_y;
/* current terminal size, checked to see if it was made smaller */
static int term_cols, term_rows;

/* the snake: tail is index 0, head is index size - 1 */
static int *snake_x, *snake_y;
static int head_x, head_y;
static int direction = NONE;

/* the bug (food) */
static int bug_x, bug_y;

static int game_over;
static int paused;

static void show_status(size_t eaten)
{
    bugs_eaten += eaten;
    mvprintw(0, 0, "X:%d/%d Y:%d/%d S:%zu/%zu B:%zu\n",
             head_x, max_x, head_y, max_y, size, max_size, bugs_eaten);
}

/* leave curses mode (only once) and print an optional message */
static int finish(const char *message)
{
    static int curses_ended;

    if (!curses_ended)
        endwin();
    curses_ended = 1;
    if (message)
        puts(message);
    return 1;
}

static void set_colour(int pair, int on)
{
    attroff(COLOR_PAIR(pair));
    if (on)
        attron(COLOR_PAIR(pair));
}

static int random_between(int low, int high)
{
    return rand() % (high - low + 1) + low;
}

/* move the snake to the new head position, changing its length by amount */
static void move_snake(size_t amount)
{
    size_t i;

    /* erase the tail */
    mvaddch(snake_y[0], snake_x[0], ' ');

    size = size + amount < max_size ? size + amount : max_size;
    if (size >= max_size) {
        finish("YOU WIN!");
        game_over = 1;
        return;
    }

    /* shift every segment one place towards the tail */
    set_colour(PAIR_BODY, 1);
    for (i = 0; size && i < size - 1; ++i) {
        if (i != 0)
            mvaddch(snake_y[i], snake_x[i], 'o');
        snake_x[i] = snake_x[i + 1];
        snake_y[i] = snake_y[i + 1];
    }
    /* the old head becomes body */
    if (size > 1)
        mvaddch(snake_y[i], snake_x[i], 'o');

    snake_x[i] = head_x;
    snake_y[i] = head_y;
    while (++i < max_size)
        snake_x[i] = snake_y[i] = 0;

    /* the head shows which way the snake is going */
    set_colour(PAIR_HEAD, 1);
    mvaddch(head_y, head_x, game_over ? ';' : "+<>^v"[direction]);
}

/* put the bug somewhere new, eating it first if eaten is set */
static void place_bug(size_t eaten)
{
    size_t i;
    int on_snake;

    if (eaten) {
        move_snake(grow);
        show_status(1);
    }
    if (game_over)
        return;

    evade_counter = 0;
    if (!eaten)
        mvaddch(bug_y, bug_x, ' ');

    /* pick a random spot that is not on the snake */
    do {
        bug_x = random_between(1, max_x - 1);
        bug_y = random_between(2, max_y - 1);
        on_snake = 0;
        for (i = 0; i < size; ++i)
            if (bug_x == snake_x[i] && bug_y == snake_y[i])
                on_snake = 1;
    } while (on_snake);
}

static void read_setting(const char *name, unsigned long *value)
{
    char *text = getenv(name);

    if (text)
        *value = strtoul(text, NULL, 0);
}

static void read_size_setting(const char *name, size_t *value)
{
    char *text = getenv(name);

    if (text)
        *value = strtoul(text, NULL, 0);
}

int main(void)
{
    char *text;
    size_t i;
    size_t field_area;
    int key_handled;

    srand((unsigned)time(NULL));
    if (!initscr())
        return puts("curses error");

    getmaxyx(stdscr, max_y, max_x);
    --max_y;
    --max_x;
    if (max_x < 9 || max_y < 9)
        return finish("screen too small");

    read_size_setting("MAXSIZE", &max_size);
    field_area = (size_t)((max_y - 2) * (max_x - 2));
    if (field_area < max_size)
        max_size = field_area;
    read_size_setting("GROW", &grow);
    read_size_setting("SIZE", &size);
    text = getenv("WAIT");
    if (text)
        wait_time = (int)strtol(text, NULL, 0);
    read_setting("EVADE", &evade);
    read_setting("WALLS", &walls_passable);
    read_setting("CANNIBAL", &cannibal);
    read_setting("SHED", &shed_interval);
    read_size_setting("SHEDS", &shed_amount);

    snake_x = calloc(max_size + 1, sizeof *snake_x);
    snake_y = calloc(max_size + 1, sizeof *snake_y);
    if (!snake_x || !snake_y)
        return finish("memory error");

    head_x = max_x / 2;
    head_y = max_y / 2;

    attron(A_BOLD);
    curs_set(0);
    nonl();
    intrflush(stdscr, 0);
    keypad(stdscr, 1);
    noecho();
    cbreak();
    start_color();
    init_pair(PAIR_WALL, WALL, WB);
    init_pair(PAIR_HEAD, HEAD, HB);
    init_pair(PAIR_BODY, BODY, BS);

    move_snake(0);
    place_bug(0);

    while (!game_over) {
        key_handled = 1;
        getmaxyx(stdscr, term_rows, term_cols);

        /* draw the walls, the bug and the status line */
        set_colour(PAIR_WALL, 1);
        mvhline(1, 0, '#', max_x);
        mvvline(1, 0, '#', max_y);
        mvhline(max_y, 0, '#', max_x);
        mvvline(1, max_x, '#', max_y);
        set_colour(PAIR_WALL, 0);
        mvaddch(bug_y, bug_x, '*');
        show_status(0);
        refresh();

        timeout(wait_time);
        switch (getch()) {
        case KEY_LEFT:
            direction = LEFT;
            break;
        case KEY_RIGHT:
            direction = RIGHT;
            break;
        case KEY_UP:
            direction = UP;
            break;
        case KEY_DOWN:
            direction = DOWN;
            break;
        case 'q':
        case 'Q':
            game_over = 1;
            /* fall through */
        case ' ':
            paused = !paused;
            /* fall through */
        default:
            key_handled = 0;
        }
        /* an arrow key unpauses the game */
        if (key_handled)
            paused = 0;

        if (game_over || paused)
            continue;
        if (direction == NONE)
            continue;

        switch (direction) {
        case LEFT:
            --head_x;
            break;
        case RIGHT:
            ++head_x;
            break;
        case UP:
            --head_y;
            break;
        case DOWN:
            ++head_y;
            break;
        }

        /* hit a wall, or the terminal was made smaller */
        if (head_x < 1 || head_x >= max_x || head_y < 2 || head_y >= max_y ||
            max_x >= term_cols || max_y >= term_rows) {
            if (walls_passable && !(max_y >= term_rows || max_x >= term_cols)) {
                /* wrap around to the other side */
                if (head_x < 1 || head_x >= max_x)
                    head_x = head_x < 1 ? max_x - 1 : 1;
                if (head_y < 2 || head_y >= max_y)
                    head_y = head_y < 2 ? max_y - 1 : 2;
            } else {
                finish("OUCH!");
                break;
            }
        }

        /* ran into itself */
        if (!cannibal) {
            for (i = 0; i < size; ++i)
                if (head_x == snake_x[i] && head_y == snake_y[i])
                    break;
            if (i < size) {
                finish("OW!");
                break;
            }
        }

        if (head_x == bug_x && head_y == bug_y) {
            place_bug(1);
        } else {
            if (evade && !(++evade_counter % evade))
                place_bug(0);
            if (shed_interval) {
                if (!(++shed_counter % shed_interval)) {
                    move_snake(shed_amount);
                    shed_counter = 0;
                } else {
                    move_snake(0);
                }
            } else {
                move_snake(0);
            }
        }
    }

    finish(NULL);
    printf("X:%d/%d Y:%d/%d S:%zu/%zu B:%zu\n",
           head_x, max_x, head_y, max_y, size, max_size, bugs_eaten);
    free(snake_x);
    free(snake_y);
    return 0;
}
