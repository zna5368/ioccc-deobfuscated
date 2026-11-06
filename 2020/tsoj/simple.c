#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* Player ship sprite (4 x 3) */
#define SHIP_W 4
#define SHIP_H 3
static const char ship_sprite[] = " /\\ " "/__\\" " ^^ ";

/* Asteroid sprite (3 x 2) */
#define ROCK_W 3
#define ROCK_H 2
static const char rock_sprite[] = "o0o" "0OO";

/* The asteroid hit box used for bullets is larger than the sprite */
#define ROCK_HIT_W 6
#define ROCK_HIT_H 4

#define BULLET_CHAR '@'
#define STAR_CHAR '.'
#define FRAME_MS 9

struct vec {
    float x, y;
};

/* Current time in milliseconds */
int now_ms()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

/*
 * Copy a sprite of size width x height into the screen buffer at (x, y).
 * Rows below the screen (and sprites with a negative position) are skipped.
 * Rows are not clipped at the right edge, so they spill into the next line.
 */
void draw_sprite(char *screen, int rows, int cols, float x, float y,
                 int width, int height, const char *sprite)
{
    int X = x, Y = y;

    for (int j = 0; j < height && Y + j < rows - X / cols && Y >= 0 && X >= 0;
         ++j)
        memcpy(&screen[(Y + j) * cols + X], &sprite[j * width], width);
}

/*
 * Do the boxes (x1, y1, w1, h1) and (x2, y2, w2, h2) touch?
 * Box 1 is to the right of box 2 when x1 - x2 > w2, etc.
 */
int overlaps(float x1, float y1, int w1, int h1,
             float x2, float y2, int w2, int h2)
{
    return !((x1 - x2 > w2 && x1 >= x2) || (x2 - x1 > w1 && x2 >= x1) ||
             (y1 - y2 > h2 && y1 >= y2) || (y2 - y1 > h1 && y2 >= y1));
}

int main()
{
    /* Put the terminal in non-blocking, no-echo, character mode */
    struct termios raw, saved;
    tcgetattr(0, &raw);
    saved = raw;
    raw.c_lflag &= ~ICANON & ~ECHO;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &raw);

    srand(time(0));

    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int rows = ws.ws_row, cols = ws.ws_col;
    int num_rocks = cols * rows / 100;
    int last_time = now_ms();
    int score = 0;
    int first_frame = 1;
    int star_speed = 0;

    struct vec ship = { cols / 2, rows / 2 };
    struct vec bullet = { -3 * rows };   /* parked far off screen */
    struct vec rock_pos[num_rocks], rock_vel[num_rocks];

    char key = 0, prev_key;
    char screen[rows + 1][cols];
    char *pixels = &screen[0][0];

    while (key != 'q') {
        float dt = (now_ms() - last_time) * .001;

        /*
         * Read a key. With no input the last key is kept, so the ship keeps
         * moving. 'p' (or a non-ASCII byte) pauses until another key arrives.
         */
        prev_key = key;
        do
            read(0, &key, 1);
        while (key == 'p' || key < 0);

        last_time = now_ms();
        star_speed += 15;

        /* Draw the scrolling star field with a modular-arithmetic pattern */
        for (int i = rows * cols - 1; i >= 0; i--) {
            int n = star_speed * dt;
            n = abs(((i / cols) - n) * (i % cols) + rows * cols);
            pixels[i] = n % 3 + n % 5 + n % 7 + n % 11 + n % 13 + n % 17 +
                        n % 19 > 14 ? ' ' : STAR_CHAR;
        }

        /* Space fires a bullet from the ship's nose and keeps moving */
        if (key == ' ') {
            bullet.x = ship.x + 2;
            bullet.y = ship.y;
            key = prev_key;
        }

        switch (prev_key) {
        case 'a': ship.x += dt * -20; break;
        case 'd': ship.x += dt * 20; break;
        case 'w': ship.y += dt * -15; break;
        case 's': ship.y += dt * 15; break;
        }
        ship.x = ship.x < 0 ? 0 : ship.x >= cols - SHIP_W ? cols - SHIP_W : ship.x;
        ship.y = ship.y < 0 ? 0 : ship.y >= rows - SHIP_H ? rows - SHIP_H : ship.y;

        /* Move and draw the bullet */
        bullet.y += dt * -20;
        int by = bullet.y, bx = bullet.x;
        if (by * cols + bx >= 0)
            pixels[by * cols + bx] = BULLET_CHAR;

        for (int i = num_rocks - 1; i >= 0; i--) {
            struct vec *rock = &rock_pos[i], *vel = &rock_vel[i];
            int hit = 0;

            rock->x += dt * vel->x;
            rock->y += dt * vel->y;

            if (overlaps(bullet.x, bullet.y, 1, 1, rock->x, rock->y,
                         ROCK_HIT_W, ROCK_HIT_H) && !first_frame) {
                hit = 1;
                score++;
                bullet.y = -3 * rows;
            }

            /* Respawn asteroids above the screen when hit or gone */
            if (rock->x < -3 || rock->x >= cols + 3 || rock->y >= rows + 2 ||
                hit || first_frame) {
                rock->y = -rand() % rows * (1 + first_frame);
                rock->x = rand() % cols;
                vel->x = -8 + rand() % 15;
                vel->y = 10 + rand() % 5;
            }

            /* Asteroid hits the ship: game over */
            if (overlaps(ship.x, ship.y, SHIP_W, SHIP_H, rock->x, rock->y,
                         ROCK_W, ROCK_H))
                key = 'q';

            draw_sprite(pixels, rows, cols, rock->x, rock->y,
                        ROCK_W, ROCK_H, rock_sprite);
        }

        draw_sprite(pixels, rows, cols, ship.x, ship.y,
                    SHIP_W, SHIP_H, ship_sprite);

        screen[rows][0] = 0;
        /* Print the screen, leaving the top-left corner for the score */
        printf("\033[0;4H%s\033[0;0H    \033[4D%i\n", &screen[0][4], score);

        while (now_ms() - last_time < FRAME_MS)
            ;
        first_frame = 0;
    }

    tcsetattr(0, TCSADRAIN, &saved);
}
