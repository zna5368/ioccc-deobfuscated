#ifndef C
#define C 39
#elif C <= 0 || C >= 5000
#undef C
#define C 39
#endif

#define WIDTH C
#define HEIGHT 22
#define CELLS (WIDTH * HEIGHT)
#define MAX_CELLS 5000
#define RAND_LIMIT 2147483647 /* RAND_MAX on the original system */

/* cell bits: a set bit means there is an opening in that direction */
#define OPEN_EAST 1
#define OPEN_NORTH 2
#define OPEN_WEST 4
#define OPEN_SOUTH 8
/* cells belonging to the tree grown from the exit carry this mark */
#define EXIT_REGION 16

/* direction index: 0 east, 1 north, 2 west, 3 south */
int step[4] = {1, -WIDTH, -1, WIDTH};
/* column a neighbour must not be in (the move wrapped around a row edge) */
int wrapped_column[4] = {0, -1, WIDTH - 1, -1};

/* maze cells; the entrance (top left) starts with its north side open */
int maze[MAX_CELLS] = {OPEN_NORTH};
/* queue of carved cells used to restart a walker that got stuck */
int queue[MAX_CELLS], *queue_head = queue, *queue_tail = queue;

int is_region_marked(int value) { return value >= EXIT_REGION; }

/* can a walker on a cell holding `here` move in `direction` to cell `to` */
int can_enter(int to, int direction, int here, int joined) {
  if (to < 0 || to >= CELLS || wrapped_column[direction] == to % WIDTH)
    return 0;
  if (!maze[to])
    return 1;
  /* the two trees may be joined once, where they first touch */
  return !joined && is_region_marked(here) != is_region_marked(maze[to]);
}

main() {
  int choices[4];
  int count = 0, direction, here, next, current, value;
  int joined = 0; /* becomes non-zero once the two trees are connected */
  int walker = CELLS - 1, other_walker = 0;
  int row, col, *cell = maze;

  /* exit (bottom right) has its south side open and starts the marked tree */
  maze[CELLS - 1] = OPEN_SOUTH | EXIT_REGION;

  /*
   * Two random walkers take turns: one starts at the exit, one at the
   * entrance.  Each step carves a passage into a random neighbour.
   */
  while (count || queue_head <= queue_tail) {
    current = walker;
    here = maze[current];
    walker = other_walker;

    count = 0;
    for (direction = 0; direction < 4; direction++)
      if (can_enter(step[direction] + current, direction, here,
                    joined))
        choices[count++] = direction;

    if (count) {
      direction = choices[rand() / (1 + RAND_LIMIT / count)];
      next = current + step[direction];
      value = maze[next];
      /* entering a non-empty cell means the two trees are now joined */
      if (joined < value)
        joined = value;
      if (!joined)
        value += here & -EXIT_REGION; /* inherit the region mark */
      /* open the wall on both sides of the passage */
      maze[current] = here | 1 << direction;
      maze[next] = value | 1 << (direction + 2) % 4;
      *queue_tail++ = next;
      other_walker = next;
    } else {
      /* stuck: restart from the oldest carved cell in the queue */
      if (queue_tail > queue_head++)
        other_walker = queue_head[-1];
    }
  }

  /* top border */
  write(1, " ", 1);
  for (col = 1; col < WIDTH; col++) {
    write(1, " ", 1);
    write(1, "_", 1);
  }

  /* each row: west wall then floor of every cell, then the east border */
  for (row = 0; row < HEIGHT; row++) {
    write(1, "\n", 1);
    for (col = 0; col < WIDTH; col++, cell++) {
      write(1, (*cell & OPEN_WEST) ? " " : "|", 1);
      write(1, (*cell & OPEN_SOUTH) ? " " : "_", 1);
    }
    write(1, "|", 1);
  }
  write(1, "\n", 1);
}
