#include <ncurses.h>

/*
 * Each maze cell is a small set of bit flags.  The low four bits record
 * which neighbouring cells are walls that have already been seen; they pick
 * the line-drawing glyph used to draw the wall.
 */
#define NEIGHBOUR_RIGHT 1
#define NEIGHBOUR_DOWN 2
#define NEIGHBOUR_LEFT 4
#define NEIGHBOUR_UP 8
#define NEIGHBOUR_MASK 15
#define WALL 16
#define SEEN 32
#define EXIT 64

#define MAZE_SIZE 256
#define MAZE_MAX (MAZE_SIZE - 1)
#define SCROLL_MARGIN 5

#define KEY_CTRL_C 3

int maze[MAZE_SIZE][MAZE_SIZE]; /* indexed as maze[column][row] */
int view_left, view_top;        /* top left corner of the visible window */
WINDOW *pad;

/*
 * Alternate character set letters indexed by the neighbour bits:
 * none, right, down, right+down, left, left+right, ... all four.
 */
char *wall_glyph_letter = "~qxlqqkwxmxtjvun";

/* line drawing character for the wall at column x, row y */
chtype wall_glyph(int x, int y) {
  return acs_map[(int)wall_glyph_letter[maze[x][y] & NEIGHBOUR_MASK]];
}

/*
 * The wall at (x, y) has just been seen: tell each neighbour that it has a
 * visible wall beside it, then draw this wall.
 *
 * The neighbour glyphs are drawn at (x, y) as well, and are immediately
 * overwritten by the final glyph for (x, y), just like the original.
 */
void reveal_wall(int x, int y) {
  if (y > 0) {
    maze[x][y - 1] |= NEIGHBOUR_DOWN;
    if (maze[x][y - 1] & (WALL | SEEN))
      mvwaddch(pad, y, x, wall_glyph(x, y - 1));
  }
  if (x > 0) {
    maze[x - 1][y] |= NEIGHBOUR_RIGHT;
    if (maze[x - 1][y] & (WALL | SEEN))
      mvwaddch(pad, y, x, wall_glyph(x - 1, y));
  }
  if (y < MAZE_MAX) {
    maze[x][y + 1] |= NEIGHBOUR_UP;
    if (maze[x][y + 1] & (WALL | SEEN))
      mvwaddch(pad, y, x, wall_glyph(x, y + 1));
  }
  if (x < MAZE_MAX) {
    maze[x + 1][y] |= NEIGHBOUR_LEFT;
    if (maze[x + 1][y] & (WALL | SEEN))
      mvwaddch(pad, y, x, wall_glyph(x + 1, y));
  }
  mvwaddch(pad, y, x, wall_glyph(x, y));
}

/* Mark one cell on a sight line as seen; returns 1 when it blocks sight. */
int see_cell(int x, int y) {
  maze[x][y] |= SEEN;
  mvwaddch(pad, y, x, maze[x][y] & EXIT ? '<' : '.');
  if (maze[x][y] & WALL) {
    reveal_wall(x, y);
    return 1;
  }
  return 0;
}

/*
 * Walk a Bresenham line from (x0, y0) towards (x1, y1) marking every cell
 * passed as seen, until the target is reached or a wall blocks the view.
 */
void cast_ray(int x0, int y0, int x1, int y1) {
  int dx = x1 - x0, dy = y1 - y0;
  int step_x, step_y;
  int error;

  if (dy < 0) {
    dy = -dy;
    step_y = -1;
  } else {
    step_y = 1;
  }
  if (dx < 0) {
    dx = -dx;
    step_x = -1;
  } else {
    step_x = 1;
  }
  dy <<= 1;
  dx <<= 1;

  if (dx > dy) {
    error = dy - (dx >> 1);
    while (x0 != x1) {
      if (error >= 0) {
        y0 += step_y;
        error -= dx;
      }
      x0 += step_x;
      error += dy;
      if (see_cell(x0, y0))
        return;
    }
  } else {
    error = dx - (dy >> 1);
    while (y0 != y1) {
      if (error >= 0) {
        x0 += step_x;
        error -= dy;
      }
      y0 += step_y;
      error += dx;
      if (see_cell(x0, y0))
        return;
    }
  }
}

/*
 * Cast rays from the player to every cell on the edge of the visible screen,
 * draw the player and show the visible part of the pad.
 */
void draw_view(int player_x, int player_y) {
  int i;

  for (i = view_left; i <= view_left + COLS; i++) {
    cast_ray(player_x, player_y, i, view_top);
    cast_ray(player_x, player_y, i, view_top + LINES);
  }
  for (i = view_top; i <= view_top + LINES; i++) {
    cast_ray(player_x, player_y, view_left, i);
    cast_ray(player_x, player_y, view_left + COLS, i);
  }
  mvwaddch(pad, player_y, player_x, '@');
  prefresh(pad, view_top, view_left, 0, 0, LINES - 1, COLS - 1);
}

/* Scroll the view by most of a screen when the player gets near an edge. */
void scroll_view(int player_x, int player_y) {
  if (player_x - view_left < SCROLL_MARGIN) {
    if (view_left > COLS - SCROLL_MARGIN)
      view_left -= COLS - SCROLL_MARGIN;
    else
      view_left = 0;
  }
  if (player_x - view_left > COLS - SCROLL_MARGIN) {
    if (view_left < MAZE_MAX - COLS * 2)
      view_left += COLS - SCROLL_MARGIN;
    else
      view_left = MAZE_SIZE - COLS;
  }
  if (player_y - view_top < SCROLL_MARGIN) {
    if (view_top > LINES - SCROLL_MARGIN)
      view_top -= LINES - SCROLL_MARGIN;
    else
      view_top = 0;
  }
  if (player_y - view_top > LINES - SCROLL_MARGIN) {
    if (view_top < MAZE_MAX - LINES * 2)
      view_top += LINES - SCROLL_MARGIN;
    else
      view_top = MAZE_SIZE - LINES;
  }
}

int main(int argc, char *argv[]) {
  FILE *maze_file = fopen(argc == 1 ? "arachnid.c" : argv[1], "r");
  int x, y, key, moves = 0;

  initscr();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);
  raw();
  pad = newpad(300, 300);

  for (x = MAZE_MAX; x >= 0; x--)
    for (y = MAZE_MAX; y >= 0; y--)
      maze[x][y] = 0;

  /* load the maze: spaces are open, '~' is the exit, anything else a wall */
  x = y = 0;
  refresh();
  while ((key = fgetc(maze_file)) != EOF) {
    if (key == '\n' || x == MAZE_SIZE) {
      x = 0;
      y++;
      if (y == MAZE_SIZE)
        break;
    } else {
      maze[x][y] = key == '~' ? EXIT : key == ' ' ? 0 : WALL;
      x++;
    }
  }

  /* surround the maze with walls */
  for (x = 0; x < MAZE_SIZE; x++)
    maze[x][0] = WALL, maze[x][MAZE_MAX] = WALL;
  for (y = 0; y < MAZE_SIZE; y++)
    maze[0][y] = WALL, maze[MAZE_MAX][y] = WALL;

  view_left = view_top = key = 0;
  x = y = 1;
  do {
    moves++;

    /* erase the player from the cell being left */
    if (maze[x][y] & SEEN)
      mvwaddch(pad, y, x, maze[x][y] & WALL ? wall_glyph(x, y) : '.');
    else
      mvwaddch(pad, y, x, ' ');

    if (key == 's' && !(maze[x][y + 1] & WALL))
      y++;
    if (key == 'w' && !(maze[x][y - 1] & WALL))
      y--;
    if (key == 'a' && !(maze[x - 1][y] & WALL))
      x--;
    if (key == 'd' && !(maze[x + 1][y] & WALL))
      x++;
    if (key == KEY_CTRL_C) {
      endwin();
      return 0;
    }

    scroll_view(x, y);
    draw_view(x, y);
    if (maze[x][y] & EXIT)
      break;
  } while ((key = getch()) != ERR);

  endwin();
  printf("You escaped after %d moves!\n", moves);
}
