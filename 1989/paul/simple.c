#include <stdio.h>

/*
 * A Turing machine simulator.
 *
 * The tape is a doubly linked list of cells. Each cell is a single long that
 * holds the XOR of the addresses of its left and right neighbours (0 for a
 * neighbour that has not been allocated yet). Pointers are always even, so
 * the lowest bit is free and holds the tape symbol (0 or 1).
 */

#define SYMBOL_BIT 1L
#define LINK_MASK (~1L)

/* bits of the third byte of an instruction */
#define READ_SYMBOL 2  /* symbol that must be under the head */
#define WRITE_SYMBOL 4 /* symbol to write */
#define MOVE_RIGHT 8   /* 1 = move right, 0 = move left */
#define PRINT_NEXT 16  /* print the byte after the instruction */

/* which side of the current cell the previous cell is on */
#define LEFT 0
#define RIGHT 1
#define NOWHERE 3 /* no previous cell yet */

#define STATE(ch) (~1 & (ch)) /* the low bit of a state is ignored */
#define START_STATE STATE('j')
#define HALT_STATE 'x'

/*
 * Instructions are three bytes: state, next state, flags. The usage and error
 * messages are part of the Turing program: the search for an instruction steps
 * through them three bytes at a time and the steps land exactly on the start
 * of "jt,s@m@ ...". Some triples inside "Usage: black" are real instructions.
 */
static char program[] =
    "Usage: black [string]\n"
    "\0No more memory\n"
    "\0Unusable memory alignment\n"
    "\0jt,s@m@ (beleY%XX&Yz {z&z}i|R(|)*((.)i)hiniFiGJ%FG.JJgJ: ;;&;z {z&z}"
    "-RS/ROiOV OP+PsaPh+ijainnjmamfmfAlnnnnphppopv%vvgv.aABiB1/BVP11/1.%..&."
    "OhrR-WV V1#1VP1CcC0R\n\n'CVP0\n!\n\n'\n"
    "EaEEnEamat!akckk'kwaww'wz,zzozEit +";

#define USAGE_MESSAGE (program + 0)
#define NO_MEMORY_MESSAGE (program + 23)
#define ALIGNMENT_MESSAGE (program + 39)

/* given a cell and one of its neighbours, return the other neighbour */
static long *other_neighbour(long *cell, long *neighbour) {
  return (long *)(LINK_MASK & (*cell ^ (long)neighbour));
}

static long *new_cell(void) {
  char *malloc();
  long *cell = (long *)malloc(sizeof(long *));

  if (!cell) {
    (void)printf(NO_MEMORY_MESSAGE);
    exit(1);
  }
  if (SYMBOL_BIT & (long)cell) {
    (void)printf(ALIGNMENT_MESSAGE);
    exit(1);
  }
  return cell;
}

/*
 * Debug trace (not called, as in the original): show 40 cells to the left of
 * the head, the head cell in column 40, and 38 cells to the right, then wait.
 */
void trace(long *head, long *previous, long previous_side) {
  char line[81];
  long *cell, *came_from, *next;
  long i;

  line[40] = '0' + (SYMBOL_BIT & *head);

  /* walk left: start from the head, coming from its right neighbour */
  came_from = previous_side ? previous : other_neighbour(head, previous);
  cell = head;
  for (i = 39; i >= 0; i--) {
    if (cell) {
      next = other_neighbour(cell, came_from);
      came_from = cell;
      cell = next;
    }
    line[i] = cell ? '0' + (SYMBOL_BIT & *cell) : '0';
  }

  /* walk right: start from the head, coming from its left neighbour */
  came_from = previous_side ? other_neighbour(head, previous) : previous;
  cell = head;
  for (i = 41; i < 79; i++) {
    if (cell) {
      next = other_neighbour(cell, came_from);
      came_from = cell;
      cell = next;
    }
    line[i] = cell ? '0' + (SYMBOL_BIT & *cell) : '0';
  }
  line[i++] = '\r';
  line[i++] = '\0';

  (void)fputs(line, stdout);
  (void)fflush(stdout);
  sleep(1);
}

int main(int argc, char **argv) {
  char *start = program; /* the Turing program */
  char *instruction;
  long *head;     /* cell under the scan head */
  long *previous = 0; /* cell the head just came from */
  long *next;
  long previous_side = NOWHERE;
  long move_right;
  int state;

  if (argc > 2) {
    (void)printf(USAGE_MESSAGE);
    exit(1);
  }
  if (argc > 1)
    start = argv[1];

  /* (void)printf("\t\t\t\t\tV\n"); marks the head column for trace() */

  head = new_cell();
  *head = 0;

  instruction = start;
  state = START_STATE;
  while (state != HALT_STATE) {
    /* trace(head, previous, previous_side); */

    /* search forward (wrapping around) for state + symbol under the head */
    for (;; instruction += 3) {
      if (!*instruction)
        instruction = start;
      if (STATE(instruction[0]) == state &&
          (SYMBOL_BIT & *head) << 1 == (READ_SYMBOL & instruction[2]))
        break;
    }

    state = STATE(instruction[1]);
    move_right = (MOVE_RIGHT & instruction[2]) >> 3;

    if (PRINT_NEXT & instruction[2])
      putchar(instruction[3]);

    if (WRITE_SYMBOL & instruction[2])
      *head |= SYMBOL_BIT;
    else
      *head &= ~SYMBOL_BIT;

    if (previous_side == move_right) {
      /* moving back to where we came from */
      next = previous;
    } else {
      next = other_neighbour(head, previous);
      if (!next) {
        /* first visit: grow the tape by one blank cell */
        next = new_cell();
        *next = (long)head;
        *head ^= (long)next;
      }
    }

    previous = head;
    head = next;
    previous_side = move_right ? LEFT : RIGHT;
  }

  /* trace(head, previous, previous_side); (void)printf("\n"); */
  exit(0);
}
