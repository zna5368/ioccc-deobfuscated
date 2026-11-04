/*
 * A shaded donut spinning over an infinite scrolling checkerboard, with a
 * scrolling "IOCCC 2006" banner, drawn with ASCII characters on an 80x23
 * VT100 terminal.
 */

#define WIDTH 80
#define HEIGHT 23
#define SCREEN_CELLS (WIDTH * HEIGHT) /* 1840 */
#define BUFFER_CELLS 1920             /* a spare row of slack */

char screen[BUFFER_CELLS];

/*
 * The banner. A '~' is transparent: the checkerboard shows through it.
 */
#define BANNER_ROWS 6
const char *banner[BANNER_ROWS] = {
    ".___________~~_________ _________ _________~~~_______________~~_______~~~~________",
    "|   \\_____  \\~\\_   ___ \\\\_   ___ \\\\_   ___ \\~~\\_____  \\   _  \\~\\   _  \\~~/  _____/",
    "|   |/   |   \\/    \\  \\//    \\  \\//    \\  \\/~~~/  ____/  /_\\  \\/  /_\\  \\/   __  \\",
    "|   /    |    \\     \\___\\     \\___\\     \\____~/       \\  \\_/   \\  \\_/   \\  |__\\  \\",
    "|___\\_______  /\\______  /\\______  /\\______  /~\\_______ \\_____  /\\_____  /\\_____  /",
    "~~~~~~~~~~~~\\/~~~~~~~~\\/~~~~~~~~\\/~~~~~~~~\\/~~~~~~~~~~\\/~~~~~\\/~~~~~~~\\/~~~~~~~\\/",
};

/* Draw the banner on the top rows with its left edge at column left. */
void draw_banner(int left)
{
    int row, col, x;

    for (row = 0; row < BANNER_ROWS; row++) {
        for (col = 0; banner[row][col]; col++) {
            x = left + col;
            if (x >= 0 && x < WIDTH && banner[row][col] != '~')
                screen[row * WIDTH + x] = banner[row][col];
        }
    }
}

int main(void)
{
    float depth[BUFFER_CELLS];   /* z-buffer of 1/z for the donut */
    float angle_a = 0, angle_b = 0;
    float phi, theta;
    int cell;

    puts("\x1b[2J"); /* clear the screen */

    for (;;) {
        float sin_a = sin(angle_a), sin_b = sin(angle_b);
        float cos_a = cos(angle_a), cos_b = cos(angle_b);

        /* Checkerboard floor: each screen row is a line on the floor whose
         * distance is inversely proportional to how far below the horizon
         * it is. */
        for (cell = 0; cell < SCREEN_CELLS; cell++) {
            int row = -cell / WIDTH - 10; /* always negative: below horizon */
            int floor_x = 41 + (cell % WIDTH - 40) * 1.3 / row + sin_b;
            int floor_y = angle_a - 100.0 / row;

            screen[cell] = ".#"[(floor_x + floor_y) & 1];
            depth[cell] = 0;
        }

        draw_banner(WIDTH - (int)(9 * angle_b) % 250);

        /* Donut: a circle of radius 1 centred 2 units from the axis
         * (theta goes round the tube) swept around the axis (phi). */
        for (theta = 0; theta < 6.28; theta += 0.07) {
            for (phi = 0; phi < 6.28; phi += 0.02) {
                float sin_phi = sin(phi), cos_theta = cos(theta);
                float sin_theta = sin(theta);
                float circle_x = cos_theta + 2;
                float inv_z = 15 / (sin_phi * circle_x * sin_a +
                                    sin_theta * cos_a + 5);
                float cos_phi = cos(phi);
                float t = sin_phi * circle_x * cos_a - sin_theta * sin_a;
                int x = 40 + 2 * inv_z *
                                 (cos_phi * circle_x * cos_b - t * sin_b);
                int y = 12 + inv_z * (cos_phi * circle_x * sin_b + t * cos_b);
                int pos = x + WIDTH * y;
                int luminance = 8 * ((sin_theta * sin_a -
                                      sin_phi * cos_theta * cos_a) * cos_b -
                                     sin_phi * cos_theta * sin_a -
                                     sin_theta * cos_a -
                                     cos_phi * cos_theta * sin_b);

                if (inv_z > depth[pos]) {
                    depth[pos] = inv_z;
                    screen[pos] = " ..,,-++=#$@"[luminance > 0 ? luminance : 0];
                }
            }
        }

        printf("\x1b[H"); /* cursor to the top-left corner */

        /* Column 0 of every row is replaced by the newline. */
        for (cell = 1; cell <= SCREEN_CELLS; cell++)
            putchar(cell % WIDTH ? screen[cell] : '\n');

        angle_a += 0.053;
        angle_b += 0.037;
    }
}
