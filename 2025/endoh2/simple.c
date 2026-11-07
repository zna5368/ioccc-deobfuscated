#include <unistd.h>
#include <stdio.h>

/*
 * The terminal size comes from the compiler command line:
 *     cc -Dw=$COLUMNS -Dh=$LINES ...
 *
 * Every lattice node is drawn as two characters, so a screen line holds
 * HALF_WIDTH nodes.  Node n is drawn at output characters 2n and 2n+1, and
 * since a line is LINE_LEN (an odd number) characters long, every other line
 * is shifted by one character, which turns the grid into a triangular lattice.
 */
#define HALF_WIDTH  (w / 2)                         /* W */
#define NODE_ROWS   (h / 2)                         /* H */
#define LINE_LEN    (2 * HALF_WIDTH + 1)            /* M: 2W chars + newline */
#define NUM_NODES   (HALF_WIDTH * NODE_ROWS)        /* N */
#define OUTPUT_LEN  (LINE_LEN * NODE_ROWS * 2)      /* T: characters printed */

/* Direction in which a node lies as seen from its parent (low 3 bits). */
enum {
    UP_LEFT = 1,
    UP_RIGHT = 2,
    LEFT = 3,
    RIGHT = 4,
    DOWN_LEFT = 5,
    DOWN_RIGHT = 6,
    HIDDEN = 7      /* the root, or a branch too faint to draw */
};

#define DIRECTION(cell) ((cell) % 8)
#define CHARGE_STEP     8

/*
 * cell[n]   low 3 bits: direction from parent; upper bits: charge
 * parent[n] node the tree was grown from to reach n
 * order[]   nodes in the order they joined the tree, followed by the frontier
 */
int cell[OUTPUT_LEN];
int parent[OUTPUT_LEN];
int order[OUTPUT_LEN];

/* Offset of the neighbour lying in the given direction. */
static int neighbour_offset(int direction)
{
    switch (direction) {
    case UP_LEFT:    return -HALF_WIDTH - 1;
    case UP_RIGHT:   return -HALF_WIDTH;
    case LEFT:       return -1;
    case RIGHT:      return +1;
    case DOWN_LEFT:  return +HALF_WIDTH;
    default:         return +HALF_WIDTH + 1;   /* DOWN_RIGHT */
    }
}

/* Only nodes away from the edge of the lattice may grow new branches. */
static int is_interior(int node)
{
    int column = node % LINE_LEN;

    return node >= HALF_WIDTH &&
           node < LINE_LEN * NODE_ROWS &&
           column % HALF_WIDTH != 0 &&
           column != HALF_WIDTH - 1;
}

int main()
{
    int random_state = getpid();
    int visited = 0;          /* nodes already in the tree */
    int frontier_size = 1;    /* candidates following them in order[] */
    int start, root, k;

    /* place the starting terminal near the centre of the lattice */
    start = NUM_NODES + (2 - HALF_WIDTH - NODE_ROWS * 3) / 2;
    root = start
         + start % (2 * HALF_WIDTH - 3) / (HALF_WIDTH - 1) * 2
         + start / (LINE_LEN - 4) * 4
         + HALF_WIDTH + 1;
    order[0] = root;
    cell[root] = HIDDEN;

    /* 1. breakdown: grow a random spanning tree from the root */
    while (frontier_size) {
        int pick, node, direction;

        random_state = 65535 & (random_state * 3533 + 1);
        pick = random_state % frontier_size--;

        /* move the picked frontier node to the end of the visited list */
        node = order[visited + pick];
        order[visited + pick] = order[visited];
        order[visited] = node;
        visited++;

        if (!is_interior(node))
            continue;

        for (direction = DOWN_RIGHT; direction >= UP_LEFT; direction--) {
            int next = node + neighbour_offset(direction);

            if (!cell[next]) {
                order[visited + frontier_size++] = next;
                cell[next] = direction;
                parent[next] = node;
            }
        }
    }

    /* 2. current flow: pass charge from the leaves back towards the root */
    while (visited > 0) {
        int node = order[--visited];

        if (cell[node] * 20 < NUM_NODES)
            cell[node] |= HIDDEN;
        cell[parent[node]] += cell[node] / CHARGE_STEP * CHARGE_STEP
                            + CHARGE_STEP;
    }

    /* 3. the flash: draw every edge of the tree */
    for (k = 0; k < OUTPUT_LEN; k++) {
        int node = k / 2;
        int here = cell[node];
        int below_left = cell[node + HALF_WIDTH];
        int below_right = cell[node + HALF_WIDTH + 1];
        int left_half = k % 2 == 0;
        int total = here + below_left + below_right;
        int brightness, underline, ch;

        if (total > NUM_NODES * 4)
            brightness = 1;     /* bold */
        else if (total > NUM_NODES)
            brightness = 0;     /* normal */
        else
            brightness = 2;     /* dim */

        /* a horizontal edge between the two nodes below is an underline */
        if (DIRECTION(below_left) == LEFT || DIRECTION(below_right) == RIGHT)
            underline = 4;
        else
            underline = 33;     /* repeats the yellow colour: no effect */

        if ((k + 1) % LINE_LEN == 0)
            ch = '\n';
        else if (left_half)
            ch = DIRECTION(here) == UP_RIGHT ||
                 DIRECTION(below_left) == DOWN_LEFT ? '/' : ' ';
        else
            ch = DIRECTION(here) == UP_LEFT ||
                 DIRECTION(below_right) == DOWN_RIGHT ? '\\' : ' ';

        printf("\x1b[%d;33;%dm%c\x1b[0m", brightness, underline, ch);
    }

    return 0;
}
