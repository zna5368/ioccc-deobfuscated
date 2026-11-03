#include <stdio.h>

#define MAZE_SIZE 82
#define LINE_SIZE 256
#define CENTER_COLUMN 26 /* screen column of the middle of the view */
#define VIEW_HALF_HEIGHT 11 /* rows drawn above and below the middle */
#define MAX_DEPTH 5         /* how many squares ahead can be seen */

/* what each square of the maze holds */
#define WALL 0
#define CORRIDOR 1
#define EXIT 2

/* which side of the corridor is being drawn */
#define LEFT 1
#define RIGHT -1

/* the maze; a cell of the file is a corridor if it is a space */
char maze[MAZE_SIZE][MAZE_SIZE];

/* player position and the direction being faced
 * (0 = east, 1 = south, 2 = west, 3 = north) */
char player_x, player_y, facing;

/* current output column, used to place text with tabs and spaces */
int cursor_column = 0;

/* how far x changes when stepping in a direction */
int step_x(int direction)
{
  direction &= 3;
  if (direction == 0)
    return 1;
  if (direction == 2)
    return -1;
  return 0;
}

/* how far y changes when stepping in a direction */
int step_y(int direction)
{
  return step_x(direction + 3);
}

/* the maze square `ahead` steps in front of the player and `side` steps to
 * the left (positive) or right (negative) */
int cell(int ahead, int side)
{
  int row = player_y + step_y(facing) * ahead - side * step_x(facing);
  int col = player_x + step_x(facing) * ahead + side * step_y(facing);
  return maze[row][col];
}

/* half the height of a wall seen at a given distance */
int half_size(int distance)
{
  if (distance == 0)
    return VIEW_HALF_HEIGHT;
  return (5 - distance) * (6 - distance) / 2;
}

/* move to `column` with tabs (8 wide) and spaces, then print `text`;
 * if the cursor is already past `column` the text is just appended */
void print_at(int column, char *text)
{
  while (cursor_column >> 3 < column >> 3) {
    putchar('\t');
    cursor_column = (cursor_column + 8) & ~7;
  }
  while (cursor_column < column) {
    putchar(' ');
    ++cursor_column;
  }
  (void)printf("%s", text);
  cursor_column += strlen(text);
}

/* draw the part of one side of the corridor between distance `distance` and
 * `distance + 1` on the screen row `row` (distance from the middle row) */
void draw_side(int distance, int side, int y)
{
  int row = abs(y);
  int far = half_size(distance + 1);
  int near = half_size(distance);
  int far_edge = CENTER_COLUMN - side * (2 * far + 1);
  int i;

  if (row > near)
    return;
  if (row < far) {
    /* vertical edge where this piece of corridor ends */
    print_at(far_edge, "|");
  } else if (cell(distance, side) != WALL) {
    /* a side corridor: draw its ceiling and floor */
    if (row == far) {
      if (side == LEFT)
        for (i = -near; i != -far; ++i)
          print_at(CENTER_COLUMN + 2 * i, "--");
      else
        for (i = far; i != near; ++i)
          print_at(CENTER_COLUMN + 2 * i + 1, "--");
    }
  } else if (row == far) {
    print_at(far_edge, "|");
  } else {
    /* a solid wall seen at an angle */
    print_at(CENTER_COLUMN - row * side * 2,
             (side == LEFT) ^ (y > 0) ? "\\" : "/");
  }
}

/* draw the wall (or exit) that blocks the corridor at `distance` */
void draw_front(int distance, int y)
{
  int row = abs(y);
  int size = half_size(distance);
  int i;

  /* a wall only has a top and bottom edge, the exit is filled in */
  if (row == size || (row < size && cell(distance, 0) != WALL)) {
    for (i = -size; i < size; ++i)
      print_at(CENTER_COLUMN + i * 2, "--");
    print_at(0, "-");
  }
}

/* draw the 3D view of what the player sees */
void draw_view(void)
{
  int y, distance, depth = 0;

  /* how far the corridor goes straight ahead */
  while (depth < MAX_DEPTH && cell(depth, 0) == CORRIDOR)
    ++depth;

  for (y = -VIEW_HALF_HEIGHT; y <= VIEW_HALF_HEIGHT; ++y) {
    /* left side moving away, the end wall, then right side coming back */
    for (distance = 0; distance < depth; ++distance)
      draw_side(distance, LEFT, y);
    if (depth < MAX_DEPTH)
      draw_front(depth, y);
    for (distance = depth - 1; distance >= 0; --distance)
      draw_side(distance, RIGHT, y);
    puts("");
    cursor_column = 0;
  }
}

/* read the maze file; spaces are corridors, the exit character is an exit,
 * everything else is a wall */
void load_maze(char *filename, char exit_char)
{
  char line[LINE_SIZE];
  int x, y = 0;
  FILE *file = fopen(filename, "r");

  while (fgets(line, LINE_SIZE, file)) {
    for (x = 0; line[x]; ++x) {
      if (line[x] == ' ')
        maze[y][x] = CORRIDOR;
      else if (line[x] == exit_char)
        maze[y][x] = EXIT;
      else
        maze[y][x] = WALL;
    }
    ++y;
  }
  fclose(file);
}

int main(int argc, char **argv)
{
  int command;

  /* by default the maze is this program's own (original) source code and
   * the exit is the backslash */
  load_maze(argc > 1 ? argv[1] : "buzzard.c", argc > 2 ? *argv[2] : '\\');

  player_x = 4;
  player_y = 3;
  facing = 1; /* south */
  if (argc > 3) {
    player_x = atoi(argv[3]);
    player_y = atoi(argv[4]);
  }

  command = '\n';
  do {
    if (command == '\n')
      draw_view();
    if (command == 'r')
      facing = (facing + 1) & 3;
    if (command == 'l')
      facing = (facing - 1) & 3;
    if (command == 'f' && cell(1, 0) != WALL) {
      player_x += step_x(facing);
      player_y += step_y(facing);
    }
  } while ((command = getchar()) != EOF && cell(0, 0) == CORRIDOR);
  return 0;
}
