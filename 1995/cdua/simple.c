#include <time.h>

/*
 * The maze is a screen of 23 rows by 80 columns stored as one string.
 * Column 79 of every row is a newline so the whole board can be printed with
 * a single printf. The outer ring (row 0, row 22, column 0, column 78) is
 * blank space and every other cell starts out as a wall '#'.
 */
#define WIDTH 80
#define HEIGHT 23
#define BOARD_SIZE (WIDTH * HEIGHT)

#define CARVE_START (12 * WIDTH + 40) /* 1000: centre of the screen */
#define MAZE_ENTRANCE (2 * WIDTH + 2) /* 162: top left corner */
#define MAZE_EXIT (20 * WIDTH + 76)   /* 1676: bottom right corner */

#define WALL '#'
#define FLOOR ' '
#define TRAIL '.'

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[1;1H"

char board[BOARD_SIZE + 1];

/* Draw one character at the screen position of a board cell. */
void draw_cell(int pos, char ch) {
  printf("\033[%d;%dH%c", pos / WIDTH + 1, pos % WIDTH + 1, ch);
}

/* Show a message in the status line at the top of the screen. */
void show_status(char *message) { printf(CURSOR_HOME "%s", message); }

/* Fill the board with walls inside a blank border and draw it. */
void init_board(void) {
  int pos, row, col;

  for (pos = 0; pos < BOARD_SIZE; pos++) {
    row = pos / WIDTH;
    col = pos % WIDTH;
    if (col == WIDTH - 1)
      board[pos] = '\n';
    else if (row == 0 || row == HEIGHT - 1 || col == 0 || col == WIDTH - 2)
      board[pos] = FLOOR;
    else
      board[pos] = WALL;
  }
  printf("%s%s%s", CLEAR_SCREEN, CURSOR_HOME, board);
}

/* Pick a random direction: down, up, right or left. */
int random_direction(void) {
  switch (rand() % 4) {
  case 0:
    return WIDTH;
  case 1:
    return -WIDTH;
  case 2:
    return 1;
  default:
    return -1;
  }
}

/*
 * Recursive backtracker maze generator. Rooms are two cells apart, so a
 * room can be dug into while the cell two steps away is still solid wall.
 */
void carve(int pos) {
  int step, next;

  while (board[pos + 2] == WALL || board[pos - 2] == WALL ||
         board[pos + 2 * WIDTH] == WALL || board[pos - 2 * WIDTH] == WALL) {
    do
      step = random_direction();
    while (board[pos + 2 * step] != WALL);

    next = pos + 2 * step;
    board[pos] = board[pos + step] = board[next] = FLOOR;
    draw_cell(pos, FLOOR);
    draw_cell(pos + step, FLOOR);
    draw_cell(next, FLOOR);
    carve(next);
  }
}

void wait_for_start(void) {
  show_status("<RET> to begin... ");
  getchar();
}

/*
 * Depth first search from pos to the exit, leaving a trail of dots and
 * erasing them again when backing out of a dead end.
 */
int solve(int pos) {
  /*
   * Quirk of the original: stepping onto the carving start cell runs the
   * "begin" code again, which waits for a key and moves back to the
   * entrance, where the trail already is. So this cell acts like a wall.
   */
  if (pos == CARVE_START) {
    wait_for_start();
    pos = MAZE_ENTRANCE;
  }

  if (board[pos] != FLOOR) {
    show_status(board[pos] == TRAIL ? "Already been here!"
                                    : "Found a wall!     ");
    return 0;
  }

  board[pos] = TRAIL;
  show_status("Walking...        ");
  draw_cell(pos, TRAIL);

  if (pos == MAZE_EXIT) {
    show_status("Finished.         ");
    printf("\033[23;1HDone!!\n");
    return 1;
  }

  if (solve(pos + 1) || solve(pos + WIDTH) || solve(pos - WIDTH) ||
      solve(pos - 1))
    return 1;

  board[pos] = FLOOR;
  show_status("Going back...     ");
  draw_cell(pos, FLOOR);
  return 0;
}

int main(void) {
  srand(time(0));
  init_board();
  carve(CARVE_START);
  wait_for_start();
  return solve(MAZE_ENTRANCE);
}
