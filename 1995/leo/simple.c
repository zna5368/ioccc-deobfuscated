#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Size of the drawing area in PostScript points (a letter page). */
#define WIDTH 608
#define HEIGHT 792

/* Command line flags.  A word is turned into a flag from its first letter. */
#define NO_LOOP 1     /* ":"        secret: draw only one picture        */
#define HAIRY 2       /* "H..."     secret: fix symmetry, but hairier     */
#define VARIABLE 4    /* "Variable" pick every branching at random        */
#define DEEP 8        /* "deep"     depth first, left first on odd steps   */
#define RIGHT 16      /* "right"    depth first, always left branch first   */
#define NO_VISITED 32 /* char 0x80  secret: never mark points as visited   */

/*
 * The two bitmaps hold one bit per point of the page.  The author assumed
 * that an unsigned long is 32 bits: only 32 bits of each word are used and
 * only (WIDTH * HEIGHT + 31) / 8 bytes are allocated.  With a 64 bit
 * unsigned long the index runs past the end of the buffer, exactly as in
 * the original program (which crashes on such machines).
 */
typedef unsigned long word;
#define BIT_WORD(map, x, y) map[(y) * WIDTH / 32 + (x) / 32]
#define TEST_BIT(map, x, y) (BIT_WORD(map, x, y) >> ((x) & 31) & 1)
#define SET_BIT(map, x, y) (BIT_WORD(map, x, y) |= 1 << ((x) & 31))

word *visited;   /* points where an open end has already been        */
word *diagonals; /* grid squares already crossed by a diagonal line  */
unsigned long cycles, frequency, flags;

/* Step for each of the 8 compass directions: 0 = north, clockwise. */
int dx[8] = {0, 4, 4, 4, 0, -4, -4, -4};
int dy[8] = {4, 4, 0, -4, -4, -4, 0, 4};

/*
 * An open end of the drawing waiting to grow.  'side' is the direction 90
 * degrees to the left of where the end points.  step 0 means it was cancelled.
 */
struct end {
  unsigned long x, y, side, step;
  struct end *next;
};

struct end *head, *tail; /* queue of open ends */

/* Insert a new open end after 'after' (or at the front when it is NULL). */
void add_end(int x, int y, int side, int step, struct end *after) {
  struct end *following = after ? after->next : head;
  struct end *end = calloc(sizeof(struct end), 1);

  if (after)
    after->next = end;
  else
    head = end;
  if (!following)
    tail = end;

  end->side = side;
  end->step = step;
  end->next = following;
  end->x = x;
  end->y = y;
  if (!(flags & NO_VISITED))
    SET_BIT(visited, x, y);
}

/*
 * An end of step 'step' has reached the visited point (x, y).  If a new end
 * that was just added there (one not belonging to 'step') is still waiting,
 * the two branches met "simultaneously" and that end is cancelled as well.
 */
void cancel_meeting(int x, int y, int step) {
  struct end *end;

  for (end = head; end; end = end->next) {
    if (end->step && end->x == x && end->y == y && end->step != step) {
      end->step = 0;
      return;
    }
  }
}

/* Pick a branching at random: 0 = left, 1 = both, 2 = right. */
int random_branching(void) {
  unsigned long percent = rand() % 100;

  if (percent < frequency / 2)
    return 0;
  if (percent < frequency)
    return 2;
  return 1;
}

/*
 * Draw a line from the end in direction 'dir'.  A diagonal line that crosses
 * another diagonal in the same grid square is drawn at half length and
 * blocks this branch.  Returns 1 when the branch is blocked.
 */
int draw_branch(struct end *end, int dir, unsigned long nx, unsigned long ny) {
  int crossed = 0;

  if (end->x != nx && end->y != ny && nx < WIDTH && ny < HEIGHT) {
    unsigned long cx = end->x < nx ? end->x : nx;
    unsigned long cy = end->y < ny ? end->y : ny;
    crossed = TEST_BIT(diagonals, cx, cy);
    if (!(flags & HAIRY))
      SET_BIT(diagonals, cx, cy);
  }
  printf("%d %d %lu %lu O ", dx[dir] >> crossed, dy[dir] >> crossed, end->x,
         end->y);
  return crossed;
}

/* Turn the branch into a new open end if it is allowed to grow. */
void grow_branch(struct end *end, int dir, unsigned long nx, unsigned long ny,
                 int crossed) {
  if (nx >= WIDTH || ny >= HEIGHT || crossed)
    return;
  if (TEST_BIT(visited, nx, ny))
    cancel_meeting(nx, ny, end->step);
  else
    add_end(nx, ny, (dir + 6) & 7, end->step + 1,
            flags & (DEEP | RIGHT) ? end : tail);
}

void main(int argc, char **argv) {
  char *branchings, line[106];
  unsigned long i;
  (void)argc;
  srand(time(0));

  /*
   * Numbers set the cycle count and then the frequency (0 is ignored as a
   * cycle).  Other words set a flag chosen by their first letter.  Parsing
   * stops at the first word that matches nothing while no flag is set.
   */
  while (*++argv) {
    char *arg = *argv;
    char letter;

    if (sscanf(arg, "%lu", cycles ? &frequency : &cycles))
      continue;
    letter = arg[0] - 1; /* ':' 'H' 'V' 'd' 'r' become 57 + 14 * bit */
    flags |= !((letter - 57) % 14) << (letter - 57) / 14;
    if (!flags)
      break;
  }

  /*
   * PostScript header: S = thinnest lines, N = show the page,
   * "dx dy x y O" = draw a line from (x, y) by (dx, dy).
   */
  puts("%!\n/S{ 0 setlinewidth} /N{ showpage} /O{ moveto rlineto stroke} def "
       "def def S");

  do {
    visited = calloc((HEIGHT * WIDTH + 31) / 8, 1);
    diagonals = calloc((HEIGHT * WIDTH + 31) / 8, 1);
    branchings = calloc(cycles, 1);
    for (i = 0; i < cycles; i++)
      branchings[i] = random_branching();

    /* Start with a cross: two ends at the centre facing NW and SE. */
    add_end(WIDTH / 2, HEIGHT / 2, 5, 1, head);
    add_end(WIDTH / 2, HEIGHT / 2, 1, 1, head);

    while (head) {
      struct end *end = head;

      if (end->step) {
        /* Each open end grows a branch 45 degrees left and/or right. */
        int left = (end->side + 1) & 7, right = (end->side + 3) & 7;
        unsigned long lx = end->x + dx[left], ly = end->y + dy[left];
        unsigned long rx = end->x + dx[right], ry = end->y + dy[right];
        int branching, left_crossed = 0, right_crossed = 0, left_first;

        if (flags & VARIABLE)
          branching = random_branching();
        else if (cycles)
          branching = branchings[end->step % cycles];
        else
          branching = (end->step & 85) % 3;

        if (branching != 2)
          left_crossed = draw_branch(end, left, lx, ly);
        if (branching != 0)
          right_crossed = draw_branch(end, right, rx, ry);

        left_first = flags & RIGHT || (flags & DEEP && end->step & 1);
        if (branching == 1 && !left_first)
          grow_branch(end, right, rx, ry, right_crossed);
        if (branching != 2)
          grow_branch(end, left, lx, ly, left_crossed);
        if (branching == 2 || (branching == 1 && left_first))
          grow_branch(end, right, rx, ry, right_crossed);
      }
      head = end->next;
      free(end);
    }

    fgets(line, 2, stdin); /* wait for return before the next page */
    puts("N S");
    fflush(stdout);
    free(branchings);
    free(visited);
    free(diagonals);
  } while (cycles >= 2 && !(flags & NO_LOOP));
}
