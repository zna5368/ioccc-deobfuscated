#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* ANSI background colour codes */
#define BLACK 40
#define WHITE 47
#define GREY 100
#define BRIGHT_RED 101
#define BRIGHT_WHITE 107

/* the ball's spin axis is tilted: sin and cos of roughly 17.5 degrees */
#define TILT_SIN .3
#define TILT_COS .954

/* microseconds to wait between frames */
#define FRAME_DELAY 79383

/* colour of a background pixel that the ball does not cover */
int background_colour(int row, int col, int rows, int cols, float x, float y,
                      float margin) {
  if (row > rows - margin) {
    /* floor: a trapezoid that narrows towards the wall to fake perspective */
    int rows_from_bottom = rows - row;
    if (col < rows_from_bottom || rows_from_bottom > cols - col)
      return BLACK;
    return GREY;
  }

  /* black borders on the left and right of the back wall */
  if (col < margin || col > cols - margin)
    return BLACK;

  /* shadow: a circle the size of the ball, shifted 0.3 radius to the right */
  if (x * (x + .6) + .09 + y * y < 1)
    return GREY;

  return WHITE;
}

/* checkerboard colour of a point on the ball's surface */
int ball_colour(float x, float y, float latitude, float spin) {
  /* position around the tilted spin axis (sine of the longitude) */
  double longitude = (TILT_COS * x + TILT_SIN * y) / sqrt(1 - latitude * latitude);

  /* 9 and 2 keep the values positive so that the int casts floor them */
  int segment = (int)(9 - spin + longitude);
  int band = (int)(2 + latitude * 2);

  return (segment + band) % 2 == 0 ? BRIGHT_WHITE : BRIGHT_RED;
}

main() {
  struct winsize size;
  ioctl(0, TIOCGWINSZ, &size);

  int col, row, rows = size.ws_row, cols = size.ws_col;
  float latitude, x, y;
  float radius = rows / 2 + rows % 2 + 1;
  float margin = rows / 5 - 1;
  float spin = 0;
  float ball_col = cols / 2;
  float ball_row = rows / 4;
  float col_speed = .01 * cols;
  float row_speed = 0;
  float spin_speed = .1;

  for (;;) {
    /* move cursor to the top left corner and hide it */
    printf("\x1b[H\x1B[?25l");
    usleep(FRAME_DELAY);

    for (row = 0; row < rows; row++) {
      for (col = 0; col < cols; col++) {
        /* position relative to the ball's centre in units of its radius;
           rows are counted twice since characters are about twice as tall */
        y = 2 * (ball_row - row) / radius;
        x = (ball_col - col) / radius;
        /* height along the tilted spin axis */
        latitude = -TILT_SIN * x + TILT_COS * y;

        if (x * x > 1 - y * y)
          printf("\x1B[%dm ",
                 background_colour(row, col, rows, cols, x, y, margin));
        else
          printf("\x1B[%dm ", ball_colour(x, y, latitude, spin));
      }
    }

    spin += spin_speed;

    /* vertical bouncing with gravity */
    ball_row += row_speed;
    if (ball_row > rows - 2 * margin)
      row_speed = -.04 * rows;
    else
      row_speed = row_speed + .002 * rows;

    /* horizontal movement; reverse the spin when bouncing off a side */
    ball_col += col_speed;
    if (ball_col < radius || ball_col > cols - radius) {
      spin_speed = -spin_speed;
      col_speed = -col_speed;
    }
  }
}
