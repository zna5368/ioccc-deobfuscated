#include <time.h>
#include <curses.h>

/*
 * The maze is a 7 x 7 x 7 cube of cells stored in a flat array.
 *   cell = level * 49 + column * 7 + row
 */
#define SIZE 7
#define LEVEL_CELLS (SIZE * SIZE)          /* 49 */
#define CELLS (SIZE * SIZE * SIZE)         /* 343 */

/* Directions. The opposite of direction d is always 5 - d. */
#define NORTH 0 /* row - 1    ('k') */
#define EAST 1  /* column + 1 ('l') */
#define UP 2    /* level + 1  ('u') */
#define DOWN 3  /* level - 1  ('d') */
#define WEST 4  /* column - 1 ('h') */
#define SOUTH 5 /* row + 1    ('j') */

/* Bits stored in each cell: an open passage in each direction, plus seen */
#define OPEN_NORTH (1 << NORTH)
#define OPEN_EAST (1 << EAST)
#define OPEN_UP (1 << UP)
#define OPEN_DOWN (1 << DOWN)
#define OPEN_WEST (1 << WEST)
#define OPEN_SOUTH (1 << SOUTH)
#define VISITED (1 << 8)

#define WALL '#'
#define FLOOR ' '

/* Random direction picker: up and down are chosen much less often */
int direction_table[27] = {NORTH, EAST,  UP,    EAST,  DOWN,  WEST,  SOUTH,
                           NORTH, EAST,  WEST,  SOUTH, NORTH, EAST,  WEST,
                           SOUTH, NORTH, EAST,  WEST,  SOUTH, NORTH, EAST,
                           WEST,  SOUTH, NORTH, EAST,  WEST,  SOUTH};

/* How far a cell index moves for each direction */
int step[6] = {-1, SIZE, LEVEL_CELLS, -LEVEL_CELLS, -SIZE, 1};

int maze[CELLS];
int pos;        /* current cell (while carving, then the player) */
int cell;       /* cell being drawn */
int dir;
int remaining = CELLS - 1;
int left, top;  /* screen position of the cell being drawn */
int wall = WALL;

#define ROW(c) ((c) % SIZE)
#define COLUMN(c) (((c) / SIZE) % SIZE)
#define LEVEL(c) ((c) / LEVEL_CELLS)

/* Draw a wall piece of a cell: open floor if the bit is set, else '#' */
#define DRAW(dy, dx, bit, open, closed)                                        \
  mvaddch(top + (dy), left + (dx), (maze[cell] & (bit)) ? (open) : (closed));

int main() {
  int next;

  srand(time(0));
  for (pos = 0; pos < CELLS; pos++)
    maze[pos] = 0;

  /* Carve the maze with a random walk until every cell is connected */
  pos = rand() % CELLS;
  while (remaining) {
    dir = direction_table[rand() % 27];
    next = pos + step[dir];
    /* Exactly one coordinate may change, and we must stay inside the cube */
    if ((ROW(pos) == ROW(next)) + (LEVEL(pos) == LEVEL(next)) +
                (COLUMN(pos) == COLUMN(next)) ==
            2 &&
        next >= 0 && next < CELLS) {
      if (!maze[next]) {
        /* Knock down the wall on both sides and move into the new cell */
        maze[pos] += 1 << dir;
        pos = next;
        maze[pos] += 1 << (5 - dir);
        remaining--;
      } else if (rand() < rand() / 7) {
        /* Sometimes jump to a random cell that is already carved */
        do {
          pos = rand() % CELLS;
        } while (!maze[pos]);
      }
    }
  }

  /* Start somewhere on the top level */
  pos = (SIZE - 1) * LEVEL_CELLS + rand() % LEVEL_CELLS;

  initscr();
  noecho();
  crmode();
  clear();
  refresh();

  /* Reaching cell 0 escapes; quitting sets pos to -1 */
  while (pos > 0) {
    move(1, 60);
    printw("Level %d", LEVEL(pos) + 1);
    maze[pos] |= VISITED;

    /* Draw every visited cell on the current level as a 4x4 block */
    for (cell = LEVEL(pos) * LEVEL_CELLS;
         cell < (LEVEL(pos) + 1) * LEVEL_CELLS; cell++)
      if (maze[cell] & VISITED) {
        left = 1 + 3 * COLUMN(cell);
        top = 1 + 3 * ROW(cell);
        mvaddch(top + 1, left + 1, FLOOR);
        /* corners */
        DRAW(0, 0, 0, 0, wall)
        DRAW(3, 0, 0, 0, wall)
        DRAW(0, 3, 0, 0, wall)
        DRAW(3, 3, 0, 0, wall)
        /* sides and stairs */
        DRAW(0, 1, OPEN_NORTH, FLOOR, wall)
        DRAW(0, 2, OPEN_NORTH, FLOOR, wall)
        DRAW(1, 3, OPEN_EAST, FLOOR, wall)
        DRAW(2, 3, OPEN_EAST, FLOOR, wall)
        DRAW(2, 1, OPEN_UP, 'U', FLOOR)
        DRAW(2, 2, OPEN_DOWN, 'D', FLOOR)
        DRAW(1, 0, OPEN_WEST, FLOOR, wall)
        DRAW(2, 0, OPEN_WEST, FLOOR, wall)
        DRAW(3, 1, OPEN_SOUTH, FLOOR, wall)
        DRAW(3, 2, OPEN_SOUTH, FLOOR, wall)
      }

    mvaddch(2 + ROW(pos) * 3, 2 + COLUMN(pos) * 3, '@');
    refresh();

    switch (getchar()) {
    case 'k':
      if (maze[pos] & OPEN_NORTH)
        pos += step[NORTH];
      break;
    case 'j':
      if (maze[pos] & OPEN_SOUTH)
        pos += step[SOUTH];
      break;
    case 'l':
      if (maze[pos] & OPEN_EAST)
        pos += step[EAST];
      break;
    case 'h':
      if (maze[pos] & OPEN_WEST)
        pos += step[WEST];
      break;
    case 'u':
      if (maze[pos] & OPEN_UP) {
        pos += step[UP];
        clear();
      }
      break;
    case 'd':
      if (maze[pos] & OPEN_DOWN) {
        pos += step[DOWN];
        clear();
      }
      break;
    case 'q':
      pos = -1;
      break;
    }
  }

  clear();
  refresh();
  nocrmode();
  echo();
  endwin();
  if (!pos)
    printf("You Escaped!\n");
  exit(0);
}
