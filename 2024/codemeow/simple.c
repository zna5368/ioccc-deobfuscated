/*
 * 2024/codemeow - deobfuscated
 *
 * Note: <sys/ioctl.h> is pulled in by the Makefile (-include sys/ioctl.h),
 * just like the original program.
 */

extern long write();
extern int nanosleep();
extern int gettimeofday();

/* Kinds of tree segments */
enum { TRUNK, SHOOT_LEFT, SHOOT_RIGHT, DYING, LEAVES };

#define POT_CELLS   18      /* the pot is 3 rows of 6 cells, 4 chars each */
#define POT_WIDTH   24
#define TREE_LIFE   35

/* ANSI colour indices */
enum { GREEN, BRIGHT_GREEN, YELLOW, BRIGHT_YELLOW, WHITE };
static const char *colour_codes[] = { "32", "92", "33", "93", "37" };

/* Colour of every 4-character cell of the pot */
static const char pot_colours[POT_CELLS] = {
    WHITE, GREEN, YELLOW, YELLOW, GREEN, WHITE,
    WHITE, WHITE, WHITE,  WHITE,  WHITE, WHITE,
    WHITE, WHITE, WHITE,  WHITE,  WHITE, WHITE,
};

/* The pot drawing, 3 rows of 24 characters */
static const char *pot_rows[3] = {
    "   \\----./----\\.----/   ",
    "    \\           // /    ",
    "     \\____________/     ",
};

/* The 3 characters drawn for each kind of segment */
static const char *segment_chars[] = {
    " /\\",   /* TRUNK       */
    "\\~\\",  /* SHOOT_LEFT  */
    "/~/",    /* SHOOT_RIGHT */
    "&&&",    /* DYING       */
    "&#&",    /* LEAVES      */
};

/* Weighted random step tables (picked by an index 0..7) */
static const signed char wide_step[8]   = { -2, -2, -1,  0, 1, 1, 2, 2 };
static const signed char narrow_step[8] = { -2, -2, -1, -1, 0, 0, 0, 1 };

int screen_cols, screen_rows, pot_col, pot_row;
int rand_state;
int counter = 0;    /* first indexes pot cells, then alternates shoot sides */

void put(const char *s, long len)
{
    write(1, s, len);
}

/* Tiny pseudo random generator returning 0..7 (or 249..255 while negative) */
unsigned char next_rand(void)
{
    rand_state ^= 8;
    return ++rand_state % 8;
}

/*
 * Pick a random step from a table.  While rand_state is still negative the
 * index is >= 8; the original read past the end of its data into zeroed
 * memory, so such an index gives 0.
 */
int random_step(const signed char *table)
{
    unsigned char idx = next_rand();
    return idx < 8 ? table[idx] : 0;
}

void seed_random(void)
{
    long now[2];                /* struct timeval */
    gettimeofday(now, 0l);
    rand_state = (char)now[0];  /* low byte of the seconds */
}

void get_screen_size(void)
{
    struct winsize ws;
    ioctl(1, TIOCGWINSZ, &ws);
    screen_rows = ws.ws_row;
    screen_cols = ws.ws_col;
    pot_col = screen_cols / 2 - 12;
    pot_row = screen_rows - 4;
}

void csi(void)          { put("\033[", 2); }
void bold(void)         { csi(); put("1m", 2); }
void reset_attrs(void)  { csi(); put("0m", 2); }

void clear_screen(void)
{
    csi(); put("?1049h", 6);    /* alternate screen buffer */
    csi(); put("2J", 2);        /* clear */
    csi(); put("H", 1);         /* home */
}

/* Write a number as exactly three decimal digits */
void put3(int n)
{
    char digits[3] = { n / 100 % 10 + '0', n / 10 % 10 + '0', n % 10 + '0' };
    put(digits, 3);
}

void move_to(int row, int col)
{
    csi(); put3(row); put(";", 1); put3(col); put("H", 1);
}

void set_colour(int colour)
{
    csi(); put(colour_codes[colour], 2); put("m", 1);
}

void draw_pot(void)
{
    for (int cell = 0; cell < POT_CELLS; cell++) {
        move_to(pot_row + cell / 6, pot_col + (cell % 6) * 4);
        set_colour(pot_colours[counter++]);
        put(pot_rows[cell / 6] + (cell % 6) * 4, 4);
    }
}

void pause_frame(void)
{
    long delay[2] = { 0, 1L << 24 };    /* struct timespec: ~16.8 ms */
    nanosleep(delay, 0l);
}

/* Wood is (bright) yellow, leaves are bright green; bold half the time */
void choose_colour(int kind)
{
    if (kind < DYING)
        kind = SHOOT_RIGHT;
    int colours[] = { BRIGHT_YELLOW, GREEN, BRIGHT_GREEN,
                      YELLOW, GREEN, BRIGHT_GREEN };
    if (next_rand() < 2) {
        bold();
        set_colour(colours[kind - 2]);
    } else {
        set_colour(colours[kind + 1]);
    }
}

/* Decide how far the next segment moves (dx right, dy down) */
void choose_step(int kind, int life, int age, int *dx, int *dy)
{
    switch (kind) {
    case TRUNK:
        if (age <= 2 || life < 4) {
            *dx = random_step(wide_step);
            *dy = 0;
        } else if (age < 15) {
            *dx = random_step(wide_step);
            *dy = -(age % 2);
        } else {
            *dx = random_step(wide_step);
            *dy = random_step(wide_step) / 2;
        }
        break;
    case DYING:
        *dx = random_step(wide_step) * 1.5;
        *dy = random_step(narrow_step);
        break;
    case LEAVES:
        *dx = random_step(wide_step) / 2;
        *dy = random_step(wide_step) / 2;
        break;
    default:    /* SHOOT_LEFT moves right-ish, SHOOT_RIGHT left-ish */
        *dx = random_step(narrow_step) * 1.5 * (1 - (kind - 1) * 2);
        *dy = random_step(narrow_step) / 2;
        break;
    }
}

void grow(int x, int y, int kind, int life)
{
    int dx, dy, age, shoot_cooldown = 5;

    while (life-- > 0) {
        age = TREE_LIFE - life;
        choose_step(kind, life, age, &dx, &dy);

        /* do not grow down into the pot */
        if (dy > 0 && y > pot_row - 2)
            dy--;

        if (life < 3 || (kind < DYING && life < 10)) {
            grow(x, y, LEAVES, life);
        } else if (kind == TRUNK && life % 3 == 0) {
            if (shoot_cooldown <= 0) {
                shoot_cooldown = 3;
                grow(x, y, counter++ % 2 + SHOOT_LEFT, life + 5);
            }
        }
        shoot_cooldown--;

        x += dx;
        y += dy;
        if (x < 0) x = 0;
        if (y < 0) y = 0;

        move_to(y, x);
        choose_colour(kind);
        put(segment_chars[kind], 3);
        reset_attrs();
        pause_frame();
    }
}

int main()
{
    get_screen_size();
    seed_random();
    clear_screen();
    draw_pot();
    grow(pot_col + 10, pot_row - 1, TRUNK, TREE_LIFE);
    move_to(screen_rows, 0);
}
