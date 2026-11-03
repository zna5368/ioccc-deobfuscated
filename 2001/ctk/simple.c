#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

/*
 * DRIVER - drive down a twisty highway without running off the road.
 *
 * The screen scrolls downwards: every timer tick a new line of road is
 * inserted at the top of the screen and the car is redrawn on row 22.
 */

#define ROWS 22          /* number of road rows that are remembered */
#define CAR_ROW 22       /* screen row the car is drawn on */
#define SCREEN_WIDTH 80
#define TREE_WIDTH 76

enum { TURN_LEFT = 0, STRAIGHT = 1, TURN_RIGHT = 2 };

/* road_center[0] / road_half_width[0] describe the newest (top) row,
 * index ROWS-1 describes the row the car is on */
char road_center[] = "((((((((((((((((((((((";                       /* 22 x 40 */
char road_half_width[] = "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"; /* 22 x 8 */

/* character used for the road edges for each road direction */
char edge_char[] = {'\\', '|', '/'};

/* keyboard LED to light for each gear (Linux console ESC [ n q):
 * 1st = num lock, 2nd = caps lock, 3rd = scroll lock, 4th = none */
char gear_led[] = {2, 3, 1, 0};

/* tree drawn one line per tick from the top down; since the screen scrolls
 * down, the first line ends up at the bottom. The empty string means
 * "no tree being drawn" */
char *tree[] = {"  |", "  |", "%\\|/%", " %%%", ""};

char direction = STRAIGHT;
char car_col = 40;      /* car column on the current tick */
char old_car_col = 40;  /* car column on the previous tick */
char steering = 0;      /* -1 left, 0 straight, +1 right */
char speed = -1;        /* current speed, 0..24, slowly follows target_speed */
char target_speed;      /* gear * 8 */
char tree_col;
char **tree_line = &tree[4];
char linux_console = 0; /* sound and LEDs only work on the Linux console */
unsigned int score = 0;

/* shift every element down by one, keeping element 0 unchanged */
static void scroll_rows(char *row)
{
    int i;

    for (i = ROWS - 1; i > 0; i--)
        row[i] = row[i - 1];
}

static void set_gear(int gear)
{
    target_speed = gear << 3;
    printf("\e[%uq", gear_led[gear]);
}

void tick(int sig)
{
    int n;

    car_col += steering;

    /* cursor home, insert a line (scrolling the screen down), draw both
     * road edges and the tree on row 1, draw the car on row 22 and erase
     * the old car that was just scrolled down to row 23 */
    printf("\x1b[H\x1b[L"
           "\x1b[;%uH%c"
           "\x1b[;%uH%c"
           "\x1b[;%uH%s"
           "\x1b[22;%uH@"
           "\x1b[23;%uH \n",
           road_center[0] - road_half_width[0], edge_char[direction],
           road_center[0] + road_half_width[0], edge_char[direction],
           tree_col, *tree_line,
           car_col,
           old_car_col);

    /* crashed: the car left the road */
    if (abs(car_col - road_center[ROWS - 1]) >= road_half_width[ROWS - 1])
        exit(0);

    /* accelerate or brake by one step towards the speed of the gear */
    if (speed != target_speed) {
        struct itimerval timer = {{0, 0}, {0, 0}};

        if (speed < target_speed)
            speed++;
        else
            speed--;

        timer.it_interval.tv_usec = timer.it_value.tv_usec =
            72000 / ((speed >> 3) + 1);
        setitimer(ITIMER_REAL, &timer, 0);

        /* engine pitch: set the console bell frequency */
        if (linux_console)
            printf("\e[10;%u]", speed + 24);
    }

    /* engine sound */
    if (linux_console)
        putchar('\a');

    /* faster and narrower road gives more points */
    score += (9 - road_half_width[ROWS - 1]) * ((speed >> 3) + 1);
    old_car_col = car_col;

    scroll_rows(road_center);
    scroll_rows(road_half_width);

    /* occasionally make the road narrower (but never narrower than 1) */
    n = rand();
    if ((n & 255) == 0) {
        if (--road_half_width[0] == 0)
            ++road_half_width[0];
    }

    /* continue drawing the current tree, or occasionally start a new one
     * far enough away from the road */
    if (**tree_line) {
        tree_line++;
    } else if ((n & 0x1F00) == 0) {
        while (abs((tree_col = rand() % TREE_WIDTH) - road_center[0] + 2)
                   - road_half_width[0] < 6)
            ;
        ++tree_col;
        tree_line = tree;
    }

    /* occasionally change the direction of the road */
    n = rand() & 31;
    if (n < 3)
        direction = n;

    /* bend the road, going straight when hitting the screen edge */
    if (direction == TURN_LEFT) {
        if (--road_center[0] <= road_half_width[0]) {
            ++road_center[0];
            ++direction;
        }
    } else if (direction == TURN_RIGHT) {
        if (++road_center[0] + road_half_width[0] > SCREEN_WIDTH - 1) {
            --road_center[0];
            --direction;
        }
    }

    signal(sig, tick);
}

void finish()
{
    signal(SIGALRM, SIG_IGN);
    /* turn off LEDs, reset the terminal, show the score */
    printf("\e[0q\ecScore: %u\n", score);
    system("stty echo -cbreak");
}

int main(int argc, char **argv)
{
    atexit(finish);

    /* the argument "q" silences the sound */
    if (argc < 2 || argv[1][0] != 'q') {
        char *term = getenv("TERM");

        linux_console = term[0] == 'l' && term[1] == 'i' &&
                        term[2] == 'n' && term[3] == 'u';
    }

    srand(getpid());
    system("stty -echo cbreak");
    set_gear(0);
    tick(SIGALRM);

    for (;;) {
        switch (getchar()) {
        case 'q':
            return 0;
        case '[':
        case 'b':
        case ',':
            steering = -1;
            break;
        case ' ':
        case 'n':
        case '.':
            steering = 0;
            break;
        case ']':
        case 'm':
        case '/':
            steering = 1;
            break;
        case '1':
            set_gear(0);
            break;
        case '2':
            set_gear(1);
            break;
        case '3':
            set_gear(2);
            break;
        case '4':
            set_gear(3);
            break;
        }
    }
    finish();
}
