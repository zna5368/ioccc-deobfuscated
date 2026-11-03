/*
 * Flying spheres - ray traced ASCII art on an 80x24 VT100 terminal.
 *
 * The screen is a 24 row by 81 column character buffer (80 visible columns
 * plus a newline). Every frame a ray is shot from the eye through each
 * character cell and tested against five spheres. A hit is shaded with a
 * character whose "weight" depends on how thick the sphere is along the ray.
 * The spheres move towards the viewer and respawn far away once they have
 * passed behind the eye.
 */

#define COLUMNS 81          /* 80 visible columns + 1 newline column */
#define SCREEN_CELLS 1944.0f /* 24 rows * 81 columns */
#define NUM_SPHERES 5
#define FIRST_CELL 33       /* the first 33 cells hold the author's name */
#define LAST_PRINTED 1941   /* the last 3 cells of the buffer are not printed */

float sphere_x[NUM_SPHERES], sphere_y[NUM_SPHERES], sphere_z[NUM_SPHERES];
float sphere_radius[NUM_SPHERES];

int spawn_range;     /* random coordinates are offset by this amount */
int random_seed = 1;

/* simple linear congruential pseudo random number generator: 0..65345 */
int next_random() {
  random_seed = (random_seed * 6478 + 1) % 65346;
  return random_seed;
}

/* give a sphere a new random x/y position and radius */
void place_sphere(int n) {
  sphere_x[n] = next_random() - spawn_range;
  sphere_y[n] = (next_random() - spawn_range) / 4;
  sphere_radius[n] = next_random() >> 4;
}

/* square root by Newton's method, accurate to about 1% */
float approx_sqrt(float value) {
  float root;

  for (root = value; root * root > value * 1.01 || root * root < value * .99;
       root -= .5 * (root * root - value) / root)
    ;
  return root;
}

int main() {
  /* screen buffer, starting with the author's name on the first line */
  char screen[1945] = "Thor AAge Eldby [eldby@online.no]";
  /*
   * shading characters, from a thin slice of sphere to its full thickness;
   * a ray through the very centre of a sphere can reach index 8, which in the
   * original reads the encoded string terminator ('\0' - 5 = byte 0xfb)
   */
  char *shades = ".oegl4OS\373";
  int cell, n;
  float ray_y, ray_x, ray_dot_center, discriminant, ray_length2, thickness;

  spawn_range = SCREEN_CELLS * 20;

  for (cell = FIRST_CELL; cell < SCREEN_CELLS; cell++)
    screen[cell] = '\n';

  /* line the spheres up one behind the other */
  for (n = 0; n < NUM_SPHERES; n++) {
    place_sphere(n);
    sphere_z[n] = (n ? sphere_z[n - 1] : 0) + spawn_range / 3 + 1;
  }

  while (1) {
    /* render: shoot a ray (ray_x, ray_y, 1) through every character cell */
    for (cell = FIRST_CELL; cell < SCREEN_CELLS; cell++) {
      /* skip the newline at the end of every row */
      if ((cell + 1) % COLUMNS == 0)
        cell++;

      ray_y = cell / SCREEN_CELLS - .5;
      ray_x = cell % COLUMNS / 40.0 - 1;
      screen[cell] = ' ';

      /* spheres with lower numbers are drawn over higher numbers */
      for (n = NUM_SPHERES - 1; n >= 0; n--) {
        /* solve |t * ray - center|^2 = radius^2 for t */
        ray_dot_center =
            ray_x * sphere_x[n] + ray_y * sphere_y[n] + sphere_z[n];
        ray_length2 = 1 + ray_y * ray_y + ray_x * ray_x;
        discriminant =
            ray_dot_center * ray_dot_center -
            ray_length2 * (-sphere_radius[n] * sphere_radius[n] +
                           sphere_x[n] * sphere_x[n] +
                           sphere_y[n] * sphere_y[n] +
                           sphere_z[n] * sphere_z[n]);

        if (discriminant > 0) {
          /* the ray hits: shade by the length of the chord through it */
          thickness = approx_sqrt(discriminant);
          screen[cell] =
              shades[(int)(8 * thickness / ray_length2 / sphere_radius[n])];
        }
      }
    }

    /* move the spheres towards the viewer, respawn those that passed by */
    for (n = NUM_SPHERES - 1; n >= 0; n--) {
      if (sphere_z[n] < 0) {
        place_sphere(n);
        sphere_z[n] = spawn_range * 2 + 1;
      }
      sphere_z[n] -= SCREEN_CELLS / 2;
    }

    /*
     * home the cursor and draw the frame (the original also prints one
     * uninitialized byte from just before its buffer at the start of every
     * frame, an out of bounds read that is not reproduced here)
     */
    printf("\033[H");
    for (cell = 0; cell < LAST_PRINTED; cell++)
      putchar(screen[cell]);
  }
}
