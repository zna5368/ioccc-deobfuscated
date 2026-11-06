#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

/*
 * Animated breadth first search through an ASCII text file.
 *
 * '@' is the start, '!' is the goal, spaces are open cells and every other
 * character is a wall.  The text is drawn on the terminal, each newly
 * discovered cell is painted with a green 'X' and, once the goal is reached,
 * the path back to the start is traced with magenta '/' characters.
 */

#define MAX_LINES     1000
#define LINE_BUF      512

#define START_CHAR    '@'
#define GOAL_CHAR     '!'
#define OPEN_CHAR     ' '

/* ANSI escape sequences */
#define CLEAR_SCREEN  "\033[H\033[J"
#define MOVE_CURSOR   "\033[%d;%dH"
#define SET_COLOR     "\033[%dm"

#define GREEN         32
#define YELLOW        33
#define MAGENTA       35
#define CYAN          36

#define SEARCH_DELAY  25000   /* microseconds per discovered cell */
#define PATH_DELAY    50000   /* microseconds per path step */

static char *grid;            /* width * height characters of the input */
static int width, height;

/* move the cursor to the screen position of a grid cell */
static void move_to_cell(int cell)
{
    printf(MOVE_CURSOR, 1 + cell / width, 1 + cell % width);
}

static int passable(int cell)
{
    return grid[cell] == OPEN_CHAR || grid[cell] == GOAL_CHAR;
}

main()
{
    /* the original allocated only 1000 bytes (not 1000 pointers) for this,
       so inputs longer than ~125 lines corrupted the first line's text */
    char *lines[MAX_LINES];
    int start = -1, goal = -1;
    int size, cell, i, n;
    int *queue, head, tail;
    int *parent, *discovered, *expanded;
    int neighbors[4], count;

    /* read the input, remembering the longest line as the grid width */
    width = 0;
    height = 0;
    while (height < MAX_LINES) {
        char *line = malloc(LINE_BUF);

        line[0] = '\0';
        if (fgets(line, LINE_BUF, stdin) == NULL && !feof(stdin))
            goto finish;
        if (strlen(line) > width)
            width = strlen(line);
        if (feof(stdin))        /* a final line without '\n' is dropped */
            break;
        lines[height++] = line;
    }

    /* copy the lines into a space padded rectangular grid */
    grid = malloc(width * height);
    for (i = 0; i < width * height; i++)
        grid[i] = ' ';
    for (int row = 0; row < height; row++) {
        int len = strlen(lines[row]);

        memcpy(grid + row * width, lines[row], len);
        for (int col = 0; col < len; col++) {
            char ch = lines[row][col];

            if (ch == START_CHAR)
                start = row * width + col;
            else if (ch == GOAL_CHAR)
                goal = row * width + col;
            else if (ch == '\n')
                grid[row * width + col] = ' ';
        }
    }
    if (start == -1 || goal == -1)
        goto finish;

    /* draw the grid */
    fflush(stdout);
    size = width * height;
    queue = malloc(sizeof(int) * size);
    printf(CLEAR_SCREEN);
    for (i = 0; i < size; i++) {
        move_to_cell(i);
        putchar(grid[i]);
    }

    /* breadth first search from the start cell */
    parent = malloc(sizeof(int) * size);
    discovered = calloc(size, sizeof(int));
    expanded = calloc(size, sizeof(int));
    parent[start] = 0;
    queue[0] = start;
    head = 0;
    tail = 1;

    while (head != tail) {
        int x, y;

        cell = queue[head++];
        x = cell % width;
        y = cell / width;

        count = 0;
        if (x + 1 < width && passable(cell + 1))
            neighbors[count++] = cell + 1;
        if (x - 1 > -1 && passable(cell - 1))
            neighbors[count++] = cell - 1;
        /* precedence quirk kept from the original: the goal test of the
           vertical neighbors is not covered by the bounds check */
        if ((y + 1 < height && grid[cell + width] == OPEN_CHAR)
            || grid[cell + width] == GOAL_CHAR)
            neighbors[count++] = cell + width;
        if ((y - 1 > -1 && grid[cell - width] == OPEN_CHAR)
            || grid[cell - width] == GOAL_CHAR)
            neighbors[count++] = cell - width;

        if (expanded[cell])
            continue;

        /* visit the neighbors, last found first */
        while (count-- > 0) {
            n = neighbors[count];
            if (discovered[n])
                continue;

            parent[n] = cell;
            usleep(SEARCH_DELAY);
            fflush(stdout);
            printf(SET_COLOR, GREEN);
            move_to_cell(n);
            putchar('X');

            if (n == goal)
                goto found;

            discovered[n] = 1;
            queue[tail++] = n;
        }
        expanded[cell] = 1;
    }
    goto finish;

found:
    /* mark the goal and trace the path back to the start */
    printf(SET_COLOR, CYAN);
    move_to_cell(goal);
    putchar(GOAL_CHAR);

    for (cell = goal; parent[cell] != start; ) {
        cell = parent[cell];
        usleep(PATH_DELAY);

        printf(SET_COLOR, MAGENTA);
        move_to_cell(cell);
        putchar('/');
        fflush(stdout);

        printf(SET_COLOR, YELLOW);
        move_to_cell(start);
        putchar('_');
    }

finish:
    /* park the cursor below the grid (the colors are left as they are) */
    printf(MOVE_CURSOR, height + 3, 1);
    fflush(stdout);
}
