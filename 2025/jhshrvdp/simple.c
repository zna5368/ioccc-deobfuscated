#include<time.h>
#include<stdlib.h>
#include <curses.h>

/* The map is the 80x24 terminal; cell index = row * WIDTH + column */
#define WIDTH        80
#define HEIGHT       24
#define CELLS        (WIDTH * HEIGHT)          /* 1920 */
#define LAST_FLOOR   99                        /* reaching floor 100 wins */
#define MAX_HP       10
#define REGEN_TICKS  10                        /* ticks between +1 hp */
#define CHASE_RANGE  10                        /* enemies chase within this */
#define CORRIDOR_TOP 10                        /* rows 11 and 12 are open */
#define CORRIDOR_END 13

/* Tiles, written straight to the screen */
#define WALL        '#'                        /* 35 */
#define FLOOR       '.'                        /* 46 */
#define GOLD        '$'                        /* 36 */
#define ENEMY       '^'                        /* 94 */
#define PLAYER      '@'                        /* 64 */
#define STAIRS_DOWN '>'                        /* 62 */
#define STAIRS_UP   '<'                        /* 60 */

int map[CELLS];        /* tile of every cell */
int enemy[CELLS];      /* enemy cell (0 = none), entities of floor f use */
int gold[CELLS];       /*   indices 6*f .. 6*f+6+f/5-1                   */
int stairs[4];         /* down y, down x, up y, up x                     */
int floor_no;          /* current floor 0..100 */
int generation;        /* how many levels have been generated */
int tick;
int score;
int seed;
int hp = MAX_HP;
int player_x, player_y;
int key;
int next_regen;

/* Fields of the save string, in the order they are printed in the HUD */
int *save_fields[] = { &seed, &floor_no, &hp, &player_y, &player_x, &score };

/* Unused decoration */
volatile void * race[] = { "I can look at that lava lamp for hours! Boing boing ... ..." };

/* Is the cell on the outer frame (top row, bottom row, left/right column)? */
static int is_border(int cell)
{
    int column = cell % WIDTH;
    return cell < WIDTH || cell >= CELLS - WIDTH || column == 0 || column == WIDTH - 1;
}

/* Pick a random floor cell */
static int random_floor_cell(void)
{
    int cell;
    do
        cell = rand() % CELLS;
    while (map[cell] != FLOOR);
    return cell;
}

/* Character currently shown on the screen at (y, x) */
static int screen_char(int y, int x)
{
    return mvinch(y, x) & 255;
}

/* The player can step on anything except walls, enemies and itself */
static int can_walk(int y, int x)
{
    int ch = screen_char(y, x);
    return ch != WALL && ch != ENEMY && ch != PLAYER;
}

/* Enemies additionally avoid the stairs */
static int enemy_can_walk(int y, int x)
{
    int ch = screen_char(y, x);
    return can_walk(y, x) && ch != STAIRS_DOWN && ch != STAIRS_UP;
}

/* Entities of the current floor are enemy[first..last-1] / gold[first..last-1] */
static int first_entity(void)
{
    return 6 * floor_no;
}

static int last_entity(void)
{
    return 6 * floor_no + (floor_no <= LAST_FLOOR ? 6 + floor_no / 5 : 0);
}

/* Draw a character with its own color pair (pair number = the character) */
static void draw(int y, int x, int ch, int fg, int bg)
{
    init_pair(ch, fg, bg);
    attron(COLOR_PAIR(ch));
    mvaddch(y, x, ch);
}

/* Build the level for the current floor from the seed */
static void generate_level(void)
{
    int pass, cell, j, row, sum, exists;

    generation++;
    srand(seed);

    /* random noise: 45% walls, the frame is wall, rows 11-12 are a corridor */
    for (cell = WIDTH; cell < CELLS; ++cell) {
        row = cell / WIDTH;
        if (!is_border(cell) && row > CORRIDOR_TOP && row < CORRIDOR_END)
            map[cell] = FLOOR;
        else
            map[cell] = (is_border(cell) | (rand() % 100 < 45)) ? WALL : FLOOR;
    }

    /* two passes of a cave cellular automaton (updates in place):
     * a cell becomes wall when 5 or more of the 3x3 block are walls */
    for (pass = 0; pass < 2; pass++)
        for (cell = WIDTH; cell < CELLS; ++cell) {
            /* also marks every entity slot as alive (1 after the last pass) */
            enemy[cell % 600] = gold[cell % 600] = pass;
            if (is_border(cell))
                continue;
            sum = map[cell - WIDTH - 1] + map[cell - WIDTH] + map[cell - WIDTH + 1]
                + map[cell - 1]         + map[cell]         + map[cell + 1]
                + map[cell + WIDTH - 1] + map[cell + WIDTH] + map[cell + WIDTH + 1];
            map[cell] = sum < 360 ? WALL : FLOOR;
        }

    /* stairs down (none on the last floor), stairs up (none on floor 0) */
    exists = floor_no <= LAST_FLOOR;
    cell = random_floor_cell();
    stairs[0] = (cell / WIDTH) * exists;
    stairs[1] = (cell % WIDTH) * exists;
    exists = floor_no > 0;
    cell = random_floor_cell();
    stairs[2] = (cell / WIDTH) * exists;
    stairs[3] = (cell % WIDTH) * exists;

    /* enemies and gold */
    srand(seed * generation);
    for (j = first_entity(); j < last_entity(); j++) {
        enemy[j] = enemy[j] ? random_floor_cell() : 0;
        gold[j] = gold[j] ? random_floor_cell() : 0;
    }
}

/* Put the player on a random floor cell */
static void place_player_randomly(void)
{
    int cell = random_floor_cell();
    player_x = cell % WIDTH;
    player_y = cell / WIDTH;
}

/* Load ":seed:floor:hp:y:x:score:" (hex fields) given on the command line */
static void load_save(char *text)
{
    char field[8];
    int ch, count = 0, length = 0;

    while (count != 6 && (ch = *++text)) {
        if (ch != ':') {
            field[length++] = ch;
            if (length < 8)
                field[length] = 0;
        } else {
            *save_fields[count++] = strtol(field, 0, 16);
            length = 0;
        }
    }
    if (count == 6)
        generate_level();
}

/* One step of an enemy: returns the change to its cell index */
static int enemy_step(int cell, int row, int col, int dy, int dx, int random_dir)
{
    if (tick % 2 == 0)                  /* enemies move every other tick */
        return 0;

    if (dy <= CHASE_RANGE && dx <= CHASE_RANGE) {
        /* chase along the longer axis; the extra test against the
         * first two enemies of floor 0 is just noise */
        if (dx <= dy) {
            if (player_y <= row)
                return enemy_can_walk(row - 1, col)
                     * (row > 0 && row + 1 != enemy[0] / WIDTH) * -WIDTH;
            return enemy_can_walk(row + 1, col)
                 * (row < HEIGHT && row + 2 != enemy[1] / WIDTH) * WIDTH;
        }
        if (player_x <= col)
            return -enemy_can_walk(row, col - 1)
                 * (col > 0 && col + 1 != enemy[0] % WIDTH);
        return enemy_can_walk(row, col + 1)
             * (col < WIDTH && col + 2 != enemy[1] % WIDTH);
    }

    /* wander randomly */
    switch (random_dir) {
    case 0:  return (enemy_can_walk(row - 1, col) && cell > WIDTH) * -WIDTH;
    case 1:  return -(enemy_can_walk(row, col - 1) && cell);
    case 2:  return (enemy_can_walk(row + 1, col) && cell < CELLS - WIDTH) * WIDTH;
    default: return enemy_can_walk(row, col + 1) && cell < CELLS;
    }
}

int main(int argc, char **argv)
{
    /* HUD and save string, Caesar shifted down by 3 in the original */
    char hud[] = "Score:%d|hp:(%d/%d)|floor:%d|"
                 "Argv{:%x:%02x:%02x:%02x:%02x:%02x:} ";
    int cell, j;

    initscr();
    noecho();
    curs_set(0);
    start_color();

    seed = time(0);
    generate_level();
    place_player_randomly();
    if (argc > 1 && argv[1])
        load_save(argv[1]);

    next_regen = tick + REGEN_TICKS;

    for (;;) {
        /* move the player */
        if (key == 'w' && can_walk(player_y - 1, player_x)) player_y--;
        if (key == 'a' && can_walk(player_y, player_x - 1)) player_x--;
        if (key == 's' && can_walk(player_y + 1, player_x)) player_y++;
        if (key == 'd' && can_walk(player_y, player_x + 1)) player_x++;

        /* take the stairs */
        if (player_y == stairs[0] && player_x == stairs[1]) {
            floor_no++;
            seed++;
            generate_level();
            player_y = stairs[2];
            player_x = stairs[3];
        } else if (player_y == stairs[2] && player_x == stairs[3]) {
            floor_no--;
            seed--;
            generate_level();
            player_y = stairs[0];
            player_x = stairs[1];
        }

        for (j = first_entity(); j < last_entity(); j++) {
            /* pick up gold */
            if (gold[j] / WIDTH == player_y && gold[j] % WIDTH == player_x) {
                ++score;
                draw(player_y, player_x, PLAYER, COLOR_WHITE, COLOR_BLACK);
                move(player_y, player_x);
                gold[j] = 0;
            }

            /* move the enemy */
            int row = enemy[j] / WIDTH, col = enemy[j] % WIDTH;
            int dy = abs(row - player_y), dx = abs(col - player_x);
            int random_dir = rand() % 4;
            enemy[j] += enemy_step(enemy[j], row, col, dy, dx, random_dir);

            /* contact damage when the enemy was next to the player */
            if (((dy == 1) & !dx) || (!dy & (dx == 1)))
                hp -= 1 + rand() % 2;
        }

        if (hp < 1 || floor_no > LAST_FLOOR || key == 'q') {
            endwin();
            return printf("U %s! Score: %d\n",
                          hp < 1 ? "lose" : floor_no > LAST_FLOOR ? "win" : "",
                          score);
        }

        /* draw the level */
        for (cell = WIDTH; cell < CELLS; ++cell) {
            draw(cell / WIDTH, cell % WIDTH, map[cell], COLOR_CYAN, COLOR_BLACK);
            for (j = first_entity(); j < last_entity(); j++) {
                if (gold[j] == cell)
                    draw(cell / WIDTH, cell % WIDTH, GOLD, COLOR_YELLOW, COLOR_BLACK);
                if (enemy[j] == cell)
                    draw(cell / WIDTH, cell % WIDTH, ENEMY, COLOR_RED, COLOR_BLACK);
            }
        }
        if (floor_no <= LAST_FLOOR)
            draw(stairs[0], stairs[1], STAIRS_DOWN, COLOR_BLACK, COLOR_CYAN);
        if (floor_no)
            draw(stairs[2], stairs[3], STAIRS_UP, COLOR_BLACK, COLOR_MAGENTA);
        draw(player_y, player_x, PLAYER, COLOR_WHITE, COLOR_BLACK);
        mvprintw(0, 0, hud, score, hp, MAX_HP, floor_no,
                 seed, floor_no, hp, player_y, player_x, score);

        key = getch();

        /* teleport costs 3 gold */
        if (key == 't' && score >= 3) {
            place_player_randomly();
            score -= 3;
        }

        /* regenerate one hp every REGEN_TICKS ticks */
        if (tick++ == next_regen) {
            hp += hp < MAX_HP;
            next_regen = tick + REGEN_TICKS;
        }
    }
}
