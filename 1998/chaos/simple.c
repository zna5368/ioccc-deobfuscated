#include <curses.h>
#include <stdlib.h>
#include <string.h>

/*
 * The virtual screen is 79 columns by 48 "half rows".  Every text row of
 * the terminal holds two half rows: an even one drawn with '`' (top half)
 * and an odd one drawn with '.' (bottom half).  When both halves of a cell
 * are lit, ':' is drawn instead.
 */
#define SCREEN_WIDTH 79
#define SCREEN_HEIGHT 48
#define CENTER_X 40
#define CENTER_Y 24
#define FOCAL_LENGTH 40

/* sin(6 degrees) and cos(6 degrees): every key press rotates by 6 degrees */
#define SIN_STEP .1045284632
#define COS_STEP .9945418726

#define AUTO_ROTATE_KEY 'u'
#define IDLE_DELAY 30 /* tenths of a second before auto rotation starts */
#define SPIN_DELAY 1  /* tenths of a second between auto rotation steps */

#define MARKER_LENGTH 9 /* "AlWuzEre " */

/* one bit per half-row pixel, used to merge the top and bottom halves */
char lit[SCREEN_HEIGHT][SCREEN_WIDTH];

/* orientation of the object: 3x3 rotation matrix (rows are view axes) */
float orientation[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
float camera_distance = 4;

/*
 * Rotate the object by 6 degrees in the plane of axes a and b (the axis c
 * stays fixed).  sine is +sin(6) or -sin(6) to choose the direction.
 */
void rotate(int a, int b, int c, float sine) {
  float step[3][3], result[3][3];
  int row, col, k;

  step[a][c] = step[b][c] = step[c][a] = step[c][b] = 0;
  step[c][c] = 1;
  step[b][a] = sine;
  step[a][b] = -sine;
  step[a][a] = step[b][b] = COS_STEP;

  /* result = step * orientation */
  for (row = 0; row < 3; row++)
    for (col = 0; col < 3; col++) {
      result[row][col] = 0;
      for (k = 0; k < 3; k++)
        result[row][col] += orientation[k][col] * step[row][k];
    }
  memcpy(orientation, result, sizeof(orientation));
}

/* component `axis` of point p after applying the orientation matrix */
float view(const float *p, int axis) {
  return p[0] * orientation[axis][0] + p[1] * orientation[axis][1] +
         p[2] * orientation[axis][2];
}

/* light one half-row pixel and draw the matching character */
void plot(int x, int y) {
  int partner = (y & 1) ? y - 1 : y + 1; /* other half of the same cell */

  lit[y][x] = 1;
  mvaddch(y / 2, x, lit[partner][x] ? ':' : ((y & 1) ? '.' : '`'));
}

/* draw a straight line with a simple DDA (step along the longer axis) */
void draw_line(int x1, int y1, int x2, int y2) {
  float x = x1, y = y1, x_step, y_step, steps;
  int dx, dy, i, px, py;

  dx = x2 - x;
  dy = y2 - y;
  steps = (float)abs(abs(dx) > abs(dy) ? dx : dy);
  x_step = dx / steps;
  y_step = dy / steps;
  steps++; /* include both end points */

  for (i = 0; i < steps; i++) {
    px = (int)(x + 0.5);
    py = (int)(y + 0.5);
    if (px >= 0 && py >= 0 && px < SCREEN_WIDTH && py < SCREEN_HEIGHT)
      plot(px, py);
    x += x_step;
    y += y_step;
  }
}

int main(int argc, char **argv) {
  /*
   * Default object (a cube) used when no data file is given.  The program
   * then opens its own executable and searches it for the "AlWuzEre" marker,
   * finding this very string inside the compiled binary.  The format is the
   * same as the .data files: the marker, the number of floats N, then N
   * floats making N/6 line segments (x1 y1 z1 x2 y2 z2).
   */
  /*
   * NOTE: this is a local array, as in the original.  The compiler may
   * not keep its text in one contiguous piece inside the executable (it
   * can build it from separate chunks), in which case reading the cube
   * from the program itself gives a damaged object, exactly as the
   * original does.
   */
  char embedded_object[] = "AlWuzEre 72 "
                           "-1 1 1 1 1 1 "
                           "1 1 1 1 1 -1 "
                           "1 1 1 1 -1 1 "
                           "1 1 -1 1 -1 -1 "
                           "-1 1 -1 -1 1 1 "
                           "1 1 -1 -1 1 -1 "
                           "1 -1 1 -1 -1 1 "
                           "1 -1 -1 -1 -1 -1 "
                           "1 -1 1 1 -1 -1 "
                           "-1 1 1 -1 -1 1 "
                           "-1 1 -1 -1 -1 -1 "
                           "-1 -1 1 -1 -1 -1 ";
  char format[16];
  FILE *file;
  long file_size;
  float *points, scale;
  int *projected;
  int count, i, key = 0;

  /* without an argument the program reads its own executable */
  if (!(file = fopen(argv[argc < 2 ? 0 : 1], "r"))) {
    printf("Can't read\n");
    exit(-1);
  }
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  /*
   * Build the format "AlWuzEre %d" at run time so that it does not appear
   * in the executable itself.  Then skip forward one byte at a time until
   * the marker followed by a number is found.
   */
  memcpy(format, embedded_object, MARKER_LENGTH);
  strcpy(format + MARKER_LENGTH, "%d");
  while (fscanf(file, format, &count) != 1) {
    fseek(file, 1, SEEK_CUR);
    if (ftell(file) >= file_size) {
      printf("Bad input\n");
      exit(-1);
    }
  }

  points = malloc(count * sizeof(float));
  projected = malloc(count * sizeof(int));
  if (!points || !projected) {
    printf("No memory\n");
    exit(-1);
  }
  for (i = 0; i < count; i++)
    fscanf(file, "%f", &points[i]);
  fclose(file);

  initscr();
  halfdelay(IDLE_DELAY);
  noecho();

  for (;;) {
    switch (key) {
    case 'h':
      rotate(1, 2, 0, SIN_STEP);
      break;
    case 'j':
      rotate(2, 0, 1, SIN_STEP);
      break;
    case 'k':
      rotate(1, 0, 2, SIN_STEP);
      break;
    case 'y':
      rotate(1, 2, 0, -SIN_STEP);
      break;
    case 'u':
      rotate(2, 0, 1, -SIN_STEP);
      break;
    case 'i':
      rotate(1, 0, 2, -SIN_STEP);
      break;
    case 'a':
      camera_distance += -.1;
      break; /* zoom in */
    case 'z':
      camera_distance += .1;
      break; /* zoom out */
    case 'q':
      printf("%.9s\n", embedded_object);
      endwin();
      exit(0);
    }

    memset(lit, 0, sizeof(lit));
    erase();

    /* perspective projection of every point (x y z) to screen (x y) */
    for (i = 0; i < count; i += 3) {
      scale = FOCAL_LENGTH / (view(&points[i], 2) + camera_distance);
      projected[i] = (int)(view(&points[i], 0) * scale) + CENTER_X;
      projected[i + 1] = (int)(view(&points[i], 1) * scale) + CENTER_Y;
    }

    /* every 6 floats form one line segment between two points */
    for (i = 0; i < count; i += 6)
      draw_line(projected[i], projected[i + 1], projected[i + 3],
                projected[i + 4]);

    move(0, 0);
    refresh();

    /* wait for a key; on timeout keep spinning quickly */
    key = getch();
    if (key == ERR) {
      halfdelay(SPIN_DELAY);
      key = AUTO_ROTATE_KEY;
    } else
      halfdelay(IDLE_DELAY);
  }
}
