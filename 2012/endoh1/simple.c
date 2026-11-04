#include <complex.h>
#include <stdio.h>
#include <unistd.h>

/*
 * The factors of gravity, pressure and viscosity are given on the command
 * line as -DG=1 -DP=4 -DV=8.
 */
#define GRAVITY G
#define PRESSURE P
#define VISCOSITY V

#define WIDTH 80  /* terminal columns, the last one holds the newline */
#define HEIGHT 25 /* terminal rows */
#define MAX_PARTICLES 19537
#define WALL_CHAR '#'
#define FRAME_DELAY 12321 /* microseconds between frames */

/*
 * A 2D vector is stored in a double complex.  The real part is the row
 * (growing downwards) and the imaginary part is the negated column.  With this
 * layout adding a plain number such as GRAVITY pulls the fluid down.
 */
struct particle {
  double complex position;
  int is_wall; /* walls never move */
  double density;
  double complex force;
  double complex velocity;
};

struct particle particles[MAX_PARTICLES];
int num_particles;

/*
 * Each character on the screen is a corner shared by up to four particle
 * cells.  The bits record which of the four surrounding cells hold a particle
 * and select the character to draw (marching squares).
 */
#define TOP_LEFT 1
#define TOP_RIGHT 2
#define BOTTOM_LEFT 4
#define BOTTOM_RIGHT 8

const char glyphs[] = " '`-.|//,\\|\\_\\/#";

/*
 * ESC[2J clears the screen and ESC[1;1H moves the cursor home.  The screen is
 * cleared only before the first frame.  The grid holds the frame followed by
 * two extra cells that are always printed as spaces.
 */
#define CLEAR_LENGTH 4
#define HEADER_LENGTH 10
#define GRID_SIZE (WIDTH * HEIGHT + 2)

char screen[HEADER_LENGTH + GRID_SIZE + 1] = "\x1b[2J"
                                             "\x1b[1;1H";
char *grid = screen + HEADER_LENGTH;

/* Each character becomes two particles, one above the other */
void add_particle_pair(int row, int column, int is_wall) {
  double complex position = row - column * I;

  if (num_particles + 2 > MAX_PARTICLES)
    return;

  particles[num_particles].position = position;
  particles[num_particles].is_wall = is_wall;
  num_particles++;

  particles[num_particles].position = position + 1;
  particles[num_particles].is_wall = is_wall;
  num_particles++;
}

/*
 * Every non-space character of the input is a particle, '#' is a wall.  Any
 * control character (including the newline) starts a new line, which is two
 * particle rows further down.
 */
void read_particles(void) {
  int row = 0;
  int column = 0;
  int c;

  while ((c = getc(stdin)) > 0) {
    if (c > '\n') {
      if (c > ' ')
        add_particle_pair(row, column, c == WALL_CHAR);
      column++;
    } else {
      row += 2;
      column = 0;
    }
  }
}

/*
 * Smoothing kernel: particles closer than 2 units interact.  The weight goes
 * from -1 when the particles touch to 0 at the edge of the range.
 */
int interacts(struct particle *p, struct particle *q, double complex *offset,
              double *weight) {
  *offset = p->position - q->position;
  *weight = cabs(*offset) / 2 - 1;
  return (int)(1 - *weight) > 0;
}

void compute_densities(void) {
  struct particle *p, *q;
  double complex offset;
  double weight;

  for (p = particles; p < particles + num_particles; p++) {
    /* walls start out heavier */
    p->density = p->is_wall * 9;

    for (q = particles; q < particles + num_particles; q++)
      if (interacts(p, q, &offset, &weight))
        p->density += weight * weight;
  }
}

void compute_forces(void) {
  struct particle *p, *q;
  double complex offset;
  double weight;

  for (p = particles; p < particles + num_particles; p++) {
    p->force = GRAVITY;

    for (q = particles; q < particles + num_particles; q++)
      if (interacts(p, q, &offset, &weight))
        p->force += weight *
                    (offset * (3 - p->density - q->density) * PRESSURE +
                     p->velocity * VISCOSITY - q->velocity * VISCOSITY) /
                    p->density;
  }
}

/* Mark the four corners around a particle cell */
void plot(int row, int column) {
  char *cell = grid + column + WIDTH * row;

  cell[0] |= BOTTOM_RIGHT;
  cell[1] |= BOTTOM_LEFT;
  /* assigned rather than or-ed, so it erases earlier bits of that corner */
  cell[WIDTH + 1] = TOP_LEFT;
  cell[WIDTH] |= TOP_RIGHT;
}

/* Plot each particle where it was, then move the free ones */
void move_and_plot(void) {
  struct particle *p;
  int i, row, column;

  for (i = 0; i < GRID_SIZE; i++)
    grid[i] = 0;

  for (p = particles; p < particles + num_particles; p++) {
    column = -cimag(p->position);
    row = creal(p->position) / 2;

    if (!p->is_wall) {
      p->velocity += p->force / 10;
      p->position += p->velocity;
    }

    if (0 <= column && column < WIDTH - 1 && 0 <= row && row < HEIGHT - 2)
      plot(row, column);
  }

  for (i = 0; i < GRID_SIZE; i++)
    grid[i] = i % WIDTH == WIDTH - 1 ? '\n' : glyphs[(int)grid[i]];
}

int main() {
  char *output = screen;

  read_particles();

  for (;;) {
    compute_densities();
    compute_forces();
    move_and_plot();
    usleep(FRAME_DELAY);

    puts(output);
    output = screen + CLEAR_LENGTH;
  }
  return 0;
}
