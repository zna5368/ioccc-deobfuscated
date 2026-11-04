#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

/* The universe is a 48x48 grid of cells packed one bit per cell. */
#define GRID_SIZE 48
#define GRID_BYTES (GRID_SIZE * GRID_SIZE / 8) /* 288 */

/* Each cell is drawn as a 20x20 pixel tile. */
#define CELL_PIXELS 20

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

/* The sprite file holds the 20x20 cell tile followed by four 5x5 frames of
 * Mr.C, which are scaled up 4 times to 20x20 pixels. */
#define SHIP_SCALE 4
#define SHIP_COLUMNS 5
#define SPRITES_HEIGHT 100

/* Mr.C is always drawn at this place in the window. */
#define SHIP_SCREEN_X 100
#define SHIP_SCREEN_Y 248

/* When the view scrolls this far the whole grid is shifted by 8 cells. */
#define SHIFT_CELLS 8
#define SCROLL_X_LIMIT 159 /* 8 cells of 20 pixels, minus one */
#define SCROLL_Y_MIN 81
#define SCROLL_Y_MAX 399
#define SCROLL_Y_START 240

#define START_SPEED 1000 /* scroll speed in 1/1000 of a pixel per frame */
#define FRAME_MICROSECONDS 16666 /* about 60 frames per second */

/* Game modes.  The odd modes are the ones where Mr.C is flying. */
enum {
  MODE_QUIT = 0,
  MODE_FLYING = 1,
  MODE_SPLASH = 2,
  MODE_FLYING_UP = 3,
  MODE_DEAD = 4,
  MODE_FLYING_DOWN = 5
};

/* A grid is wrapped in a struct so it can be copied with '=' */
typedef struct {
  char bits[GRID_BYTES];
} Grid;

Grid current, next;
int mode;
int speed;     /* grows every frame, so the game gets faster */
int frame;     /* counts frames until the next Life generation */
int scroll_x;  /* pixel offset of the window into the grid */
int scroll_y;

int is_flying(int m) { return m % 2; }

int cell_alive(Grid *grid, int x, int y) {
  int index = x * GRID_SIZE + y;
  return (grid->bits[index / 8] & (1 << index % 8)) ? 1 : 0;
}

void set_cell(Grid *grid, int x, int y, int alive) {
  int index = x * GRID_SIZE + y;
  if (alive)
    grid->bits[index / 8] |= 1 << index % 8;
  else
    grid->bits[index / 8] &= ~(1 << index % 8);
}

void load_grid(const char *filename) {
  freopen(filename, "r", stdin);
  fread(&current, GRID_BYTES, 1, stdin);
}

/* Change the game mode.  Entering a new kind of mode resets the game. */
void set_mode(int new_mode) {
  int old_mode = mode;
  int x, y;

  mode = new_mode;
  if (old_mode == new_mode || (is_flying(old_mode) && is_flying(new_mode)))
    return;

  if (new_mode == MODE_SPLASH) {
    load_grid("splash.d");
  } else if (new_mode == MODE_DEAD) {
    load_grid("dead.d");
  } else {
    /* a new game starts in an empty universe */
    for (x = 0; x < GRID_SIZE; ++x)
      for (y = 0; y < GRID_SIZE; ++y)
        set_cell(&next, x, y, 0);
    current = next;
  }
  speed = START_SPEED;
  scroll_y = SCROLL_Y_START;
  frame = 1;
  scroll_x = 0;
}

int mode_after_key_press(KeySym key) {
  if (key == XK_q)
    return MODE_QUIT;
  if (key == XK_Up)
    return mode == MODE_DEAD ? MODE_DEAD : MODE_FLYING_UP;
  if (key == XK_Down)
    return mode == MODE_DEAD ? MODE_DEAD : MODE_FLYING_DOWN;
  /* any other key starts the game or leaves the death screen */
  if (mode == MODE_SPLASH)
    return MODE_FLYING;
  if (mode == MODE_DEAD)
    return MODE_SPLASH;
  return mode;
}

int mode_after_key_release(KeySym key) {
  if (key == XK_Up)
    return mode == MODE_FLYING_UP ? MODE_FLYING : mode;
  return mode == MODE_FLYING_DOWN ? MODE_FLYING : mode;
}

/* One generation of Conway's Game of Life (B3/S23), no wrap around */
void life_step(void) {
  int x, y, dx, dy, neighbours;

  for (x = 0; x < GRID_SIZE; ++x)
    for (y = 0; y < GRID_SIZE; ++y) {
      neighbours = 0;
      for (dx = -1; dx < 2; ++dx)
        if (x + dx >= 0 && x + dx < GRID_SIZE)
          for (dy = -1; dy < 2; ++dy)
            if (y + dy >= 0 && y + dy < GRID_SIZE)
              neighbours += (dx || dy) && cell_alive(&current, x + dx, y + dy);
      if (cell_alive(&current, x, y))
        set_cell(&next, x, y, neighbours == 2 || neighbours == 3);
      else
        set_cell(&next, x, y, neighbours == 3);
    }
  current = next;
}

/* Move every cell by (shift_x, shift_y).  Cells that come in from outside
 * the grid are filled randomly, which makes the universe endless. */
void shift_grid(int shift_x, int shift_y) {
  int x, y, from_x, from_y;

  for (x = 0; x < GRID_SIZE; ++x)
    for (y = 0; y < GRID_SIZE; ++y) {
      from_x = x - shift_x;
      from_y = y - shift_y;
      if (from_x >= 0 && from_x < GRID_SIZE && from_y >= 0 &&
          from_y < GRID_SIZE)
        set_cell(&next, x, y, cell_alive(&current, from_x, from_y));
      else
        set_cell(&next, x, y, (rand() % 8) == 1);
    }
  current = next;
}

int main() {
  Display *display = XOpenDisplay(0);
  int screen = DefaultScreen(display);
  Window window =
      XCreateSimpleWindow(display, RootWindow(display, screen), 40, 40,
                          WINDOW_WIDTH, WINDOW_HEIGHT, 3, 0, 0);
  int depth = DefaultDepth(display, screen);
  Pixmap back_buffer =
      XCreatePixmap(display, window, WINDOW_WIDTH, WINDOW_HEIGHT, depth);
  Pixmap sprites =
      XCreatePixmap(display, window, CELL_PIXELS, SPRITES_HEIGHT, depth);
  GC gc = DefaultGC(display, screen);
  XGCValues white, black;
  XEvent event;
  int x, y, shift_x, shift_y, ship_frame;

  XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
  XMapWindow(display, window);
  white.foreground = WhitePixel(display, screen);
  black.foreground = BlackPixel(display, screen);

  /* Build the sprite pixmap: the cell tile at the top, then the four
   * frames of Mr.C stacked below it. */
  load_grid("sprites.d");
  for (x = 0; x < CELL_PIXELS + SHIP_COLUMNS; ++x)
    for (y = 0; y < CELL_PIXELS; ++y) {
      XChangeGC(display, gc, GCForeground,
                cell_alive(&current, x, y) ? &black : &white);
      if (x < CELL_PIXELS)
        XDrawPoint(display, sprites, gc, x, y);
      else
        XFillRectangle(display, sprites, gc, SHIP_SCALE * (x - CELL_PIXELS),
                       CELL_PIXELS + SHIP_SCALE * y, SHIP_SCALE, SHIP_SCALE);
    }
  /* the background is cleared with white */
  XChangeGC(display, gc, GCForeground, &white);

  srand(time(0));
  set_mode(MODE_SPLASH);

  while (usleep(FRAME_MICROSECONDS), mode != MODE_QUIT) {
    while (XPending(display)) {
      XNextEvent(display, &event);
      if (event.type == KeyPress)
        set_mode(mode_after_key_press(XLookupKeysym(&event.xkey, 0)));
      else if (event.type == KeyRelease)
        set_mode(mode_after_key_release(XLookupKeysym(&event.xkey, 0)));
    }

    if (frame == 0)
      life_step();
    /* Life runs every half second in the game, every 2 seconds otherwise */
    frame = (frame + 1) % (is_flying(mode) ? 30 : 120);

    speed += 2;
    if (is_flying(mode))
      scroll_x += speed / 1000;
    if (mode == MODE_FLYING_UP)
      scroll_y -= 2;
    else if (mode == MODE_FLYING_DOWN)
      scroll_y += 2;

    /* keep the view inside the grid by moving the grid instead */
    shift_x = 0;
    if (scroll_x > SCROLL_X_LIMIT) {
      shift_x = -SHIFT_CELLS;
      scroll_x = 0;
    }
    shift_y = 0;
    if (scroll_y > SCROLL_Y_MAX) {
      shift_y = -SHIFT_CELLS;
      scroll_y = SCROLL_Y_START;
    } else if (scroll_y < SCROLL_Y_MIN) {
      shift_y = SHIFT_CELLS;
      scroll_y = SCROLL_Y_START;
    }
    shift_grid(shift_x, shift_y);

    /* draw the live cells */
    XFillRectangle(display, back_buffer, gc, 0, 0, WINDOW_WIDTH,
                   WINDOW_HEIGHT);
    for (x = 0; x < GRID_SIZE; ++x)
      for (y = 0; y < GRID_SIZE; ++y)
        if (cell_alive(&current, x, y))
          XCopyArea(display, sprites, back_buffer, gc, 0, 0, CELL_PIXELS,
                    CELL_PIXELS, x * CELL_PIXELS - scroll_x,
                    y * CELL_PIXELS - scroll_y);

    if (is_flying(mode)) {
      /* Mr.C dies if any pixel under him is inside a live cell */
      for (x = scroll_x + SHIP_SCREEN_X;
           x < scroll_x + SHIP_SCREEN_X + CELL_PIXELS; ++x)
        for (y = scroll_y + SHIP_SCREEN_Y;
             y < scroll_y + SHIP_SCREEN_Y + CELL_PIXELS; ++y)
          if (cell_alive(&current, x / CELL_PIXELS, y / CELL_PIXELS))
            set_mode(MODE_DEAD);

      ship_frame = frame / 8 + 1;
      XCopyArea(display, sprites, back_buffer, gc, 0, CELL_PIXELS * ship_frame,
                CELL_PIXELS, CELL_PIXELS, SHIP_SCREEN_X, SHIP_SCREEN_Y);
    }

    XCopyArea(display, back_buffer, window, gc, 0, 0, WINDOW_WIDTH,
              WINDOW_HEIGHT, 0, 0);
  }
}
