#include <X11/Xlib.h>

/*
 * jonth - jon's t(h)etris for the X Window System
 *
 * Keys: j = left, l = right, k = rotate, SPACE = drop
 */

#define BOARD_WIDTH 10   /* columns */
#define BOARD_HEIGHT 22  /* rows */
#define CELL_SIZE 20     /* pixels per cell */
#define STATUS_HEIGHT 30 /* score bar above the board */
#define WINDOW_WIDTH (BOARD_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (BOARD_HEIGHT * CELL_SIZE + STATUS_HEIGHT)

#define NUM_SHAPES 19 /* 7 tetrominoes in all their distinct rotations */
#define SPAWN_COLUMN 3

#define ALARM_SIGNAL 14 /* SIGALRM */
#define REAL_TIMER 0    /* ITIMER_REAL */

/* Commands understood by move_piece() */
enum {
  CMD_PLACE_FIRST = 1,
  CMD_FALL = 2,
  CMD_SPAWN = 3,
  CMD_ROTATE = 4,
  CMD_LEFT = 'j',
  CMD_RIGHT = 'l',
  CMD_DROP = ' '
};

/* Each shape is four cells, each given as (column, row) inside a 4x4 box */
struct cell {
  int x, y;
};

static struct cell shapes[NUM_SHAPES][4] = {
    /* I */
    {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
    {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
    /* L */
    {{0, 2}, {1, 2}, {2, 2}, {2, 1}},
    {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
    {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
    {{1, 0}, {2, 0}, {2, 1}, {2, 2}},
    /* O */
    {{1, 1}, {2, 1}, {2, 2}, {1, 2}},
    /* J */
    {{0, 1}, {0, 2}, {1, 2}, {2, 2}},
    {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
    {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
    {{2, 0}, {2, 1}, {2, 2}, {1, 2}},
    /* T */
    {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
    {{2, 0}, {1, 1}, {2, 1}, {2, 2}},
    {{1, 1}, {0, 2}, {1, 2}, {2, 2}},
    {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
    /* S */
    {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
    {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
    /* Z */
    {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
    {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
};

/* The shape obtained by rotating each shape once */
static int next_rotation[NUM_SHAPES] = {1, 0,  3,  4,  5,  2,  6,  8,  9, 10,
                                        7, 12, 13, 14, 11, 16, 15, 18, 17};

static XRectangle rects[BOARD_WIDTH * BOARD_HEIGHT];
static XRectangle status_bar; /* x = y = 0 */

static Display *display;
static Window window;
static GC draw_gc;  /* foreground black: filled cells, text */
static GC erase_gc; /* foreground white: empty cells, background */

static long board[BOARD_WIDTH][BOARD_HEIGHT];
static long piece_col = SPAWN_COLUMN; /* position of the falling piece */
static long piece_row;
static long score;
static long level;
static long tick_pending; /* set by the timer: the piece should fall */
static char score_text[99];
static XEvent tick_event;
static long timer_value[4]; /* struct itimerval: interval {sec, usec},
                                                  value {sec, usec} */

static void install_timer_handler(void);

/* SIGALRM: wake the event loop by sending ourselves a fake key press */
static void on_timer(void) {
  if (!tick_pending) {
    tick_pending = 1;
    tick_event.type = KeyPress;
    XSendEvent(display, window, True, 0, &tick_event);
  }
  XFlush(display);
  install_timer_handler();
}

static void install_timer_handler(void) { signal(ALARM_SIGNAL, on_timer); }

/* The piece falls faster at each level */
static void start_timer(void) {
  timer_value[1] = timer_value[3] = (9 - level) * 40000; /* microseconds */
  setitimer(REAL_TIMER, timer_value, 0);
}

static void draw_status(void) {
  status_bar.width = WINDOW_WIDTH;
  status_bar.height = STATUS_HEIGHT;
  XFillRectangles(display, window, erase_gc, &status_bar, 1);
  sprintf(score_text, "Score%9d Level%9d", score, level);
  XDrawString(display, window, draw_gc, 7, 15, score_text, 29);
}

static void draw_board(void) {
  long col, row, count;

  count = 0;
  for (col = 0; col < BOARD_WIDTH; col++)
    for (row = 0; row < BOARD_HEIGHT; row++)
      if (board[col][row]) {
        rects[count].x = col * CELL_SIZE;
        rects[count].y = row * CELL_SIZE + STATUS_HEIGHT;
        rects[count].width = rects[count].height = CELL_SIZE - 1;
        count++;
      }
  XFillRectangles(display, window, draw_gc, rects, count);

  count = 0;
  for (col = 0; col < BOARD_WIDTH; col++)
    for (row = 0; row < BOARD_HEIGHT; row++)
      if (!board[col][row]) {
        rects[count].x = col * CELL_SIZE;
        rects[count].y = row * CELL_SIZE + STATUS_HEIGHT;
        rects[count].width = rects[count].height = CELL_SIZE;
        count++;
      }
  XFillRectangles(display, window, erase_gc, rects, count);

  draw_status();
}

/* Return the key pressed, or 0 for any other event (including timer ticks) */
static long read_key(void) {
  XEvent event;

  XNextEvent(display, &event);
  switch (event.type) {
  case KeyPress:
    if (event.xkey.send_event)
      return 0;
    return XKeycodeToKeysym(display, event.xkey.keycode, 0);
  case Expose:
    draw_board();
  }
  return 0;
}

static void draw_piece(int shape) {
  int i;

  for (i = 0; i < 4; i++) {
    rects[i].x = (shapes[shape][i].x + piece_col) * CELL_SIZE;
    rects[i].y = (shapes[shape][i].y + piece_row) * CELL_SIZE + STATUS_HEIGHT;
    rects[i].width = rects[i].height = CELL_SIZE - 1;
  }
  XFillRectangles(display, window, draw_gc, rects, 4);
}

static void erase_piece(int shape) {
  int i;

  for (i = 0; i < 4; i++) {
    rects[i].x = (shapes[shape][i].x + piece_col) * CELL_SIZE;
    rects[i].y = (shapes[shape][i].y + piece_row) * CELL_SIZE + STATUS_HEIGHT;
    rects[i].width = rects[i].height = CELL_SIZE;
  }
  XFillRectangles(display, window, erase_gc, rects, 4);
}

static void set_piece(int shape, long value) {
  int i;

  for (i = 0; i < 4; i++)
    board[shapes[shape][i].x + piece_col][shapes[shape][i].y + piece_row] =
        value;
}

/* Can the given shape sit at column offset dx, row offset dy from the
 * current piece position? */
static int fits(int shape, int dx, int dy) {
  int i;

  for (i = 0; i < 4; i++) {
    long col = shapes[shape][i].x + piece_col + dx;
    long row = shapes[shape][i].y + piece_row + dy;

    if (col < 0 || col >= BOARD_WIDTH || row >= BOARD_HEIGHT || board[col][row])
      return 0;
  }
  return 1;
}

/* Try to move the shape by (dx, dy). Returns 1 if it moved, else 0 */
static int shift_piece(int shape, int dx, int dy) {
  set_piece(shape, 0);
  if (!fits(shape, dx, dy)) {
    set_piece(shape, 1);
    return 0;
  }
  erase_piece(shape);
  piece_col += dx;
  piece_row += dy;
  draw_piece(shape);
  set_piece(shape, 1);
  return 1;
}

/* Apply a command to the falling piece. Returns 1 on success, else 0 */
static int move_piece(int shape, int old_shape, int command) {
  switch (command) {
  case CMD_PLACE_FIRST:
    set_piece(shape, 1);
    draw_piece(shape);
    return 1;

  case CMD_LEFT:
    return shift_piece(shape, -1, 0);

  case CMD_RIGHT:
    return shift_piece(shape, 1, 0);

  case CMD_FALL:
    return shift_piece(shape, 0, 1);

  case CMD_ROTATE:
    set_piece(old_shape, 0);
    if (!fits(shape, 0, 0)) {
      set_piece(old_shape, 1);
      return 0;
    }
    erase_piece(old_shape);
    draw_piece(shape);
    set_piece(shape, 1);
    return 1;

  case CMD_DROP:
    /* one point per row dropped; tick_pending becomes the score before
     * the last step, which makes the piece lock at once */
    while (move_piece(shape, shape, CMD_FALL))
      tick_pending = score++;
    return 1;

  case CMD_SPAWN:
    piece_row = 0;
    piece_col = SPAWN_COLUMN;
    if (!fits(shape, 0, 0))
      return 0;
    draw_piece(shape);
    set_piece(shape, 1);
    return 1;
  }
  return 0;
}

/* Move every row above full_row down by one */
static void remove_row(long full_row) {
  long col, row;

  for (row = full_row; row; row--)
    for (col = 0; col < BOARD_WIDTH; col++)
      board[col][row] = board[col][row - 1];
}

int main() {
  XSetWindowAttributes attributes;
  XGCValues gc_values;
  int screen;
  long key, col, row, full;
  long lines = 0;
  int shape = 0, old_shape;

  srand(time(0));

  display = XOpenDisplay(0);
  screen = XDefaultScreen(display);
  attributes.background_pixel = XWhitePixel(display, screen);
  window =
      XCreateWindow(display, XRootWindow(display, screen), 0, 0, WINDOW_WIDTH,
                    WINDOW_HEIGHT, 2, CopyFromParent, InputOutput,
                    (Visual *)CopyFromParent, CWBackPixel, &attributes);
  XMapWindow(display, window);
  XSelectInput(display, window, ExposureMask | KeyPressMask);

  gc_values.foreground = XWhitePixel(display, screen);
  erase_gc = XCreateGC(display, window, GCForeground, &gc_values);
  gc_values.foreground = XBlackPixel(display, screen);
  draw_gc = XCreateGC(display, window, GCForeground, &gc_values);

  install_timer_handler();
  start_timer();
  move_piece(shape, shape, CMD_PLACE_FIRST);

  for (;;) {
    key = read_key();
    old_shape = shape;
    switch (key) {
    case 'k':
      shape = next_rotation[shape];
      if (!move_piece(shape, old_shape, CMD_ROTATE))
        shape = old_shape;
      break;
    case 'j':
      move_piece(shape, old_shape, CMD_LEFT);
      break;
    case 'l':
      move_piece(shape, old_shape, CMD_RIGHT);
      break;
    case ' ':
      move_piece(shape, old_shape, CMD_DROP);
    }

    if (tick_pending) {
      if (!move_piece(shape, old_shape, CMD_FALL)) {
        /* the piece has landed: clear full rows (rows 0 and 1 are never
         * checked) */
        for (row = BOARD_HEIGHT - 1; row > 1; row--) {
          full = 1;
          for (col = 0; col < BOARD_WIDTH && full; col++)
            full = board[col][row];
          if (full) {
            remove_row(row);
            row++; /* check the same row again */
            lines++;
          }
        }
        draw_board();
        score++;

        shape = rand() % NUM_SHAPES;
        if (!move_piece(shape, shape, CMD_SPAWN))
          return puts(score_text); /* game over */

        level = lines / 9;
        start_timer();
        draw_status();
      }
      tick_pending = 0;
    }
  }
}
