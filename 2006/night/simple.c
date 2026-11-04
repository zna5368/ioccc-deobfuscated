#include <ncurses.h>

/*
 * The maze has 4 x 4 cells.  Each cell holds a 2-bit "piece", so the
 * whole layout (its "DNA") fits in 32 bits of an unsigned long.
 *
 * Cells are numbered along a snake path: cell / 4 is the row, and the
 * low two bits run right-to-left on even rows and left-to-right on odd
 * rows.  Cell 3 is the upper left corner (start) and cell 0 is the
 * upper right corner (exit through the top).
 *
 * Bit 1 of a piece selects the vertical opening and bit 0 the horizontal
 * opening.  A clear bit connects to the snake neighbour that is always
 * on the board (cell ^ 7 vertically, cell ^ 1 horizontally).  A set bit
 * connects to the other neighbour (cell ^ 15 or cell ^ 3), which only
 * exists when the row or column is one of the two middle ones.  At the
 * exit cell a set vertical bit opens the way out of the maze.
 *
 * Leaving a cell rotates its piece through 0 -> 1 -> 3 -> 2 -> 0.
 */

#define CELLS 16
#define START_CELL 3
#define EXIT_CELL 0
#define TARGET_MOVES 64
#define QUEUE_SIZE (1 << 16)
#define MOVE_COUNT (1 << 4) /* moves are kept above the 4 cell bits */

#define VERTICAL 2   /* piece bit / orientation bit for up-down   */
#define HORIZONTAL 1 /* piece bit / orientation bit for left-right */

#define CELL_HEIGHT 6
#define CELL_WIDTH 12
#define MAZE_TOP 4

/* breadth first search queue: maze layout and (moves << 4 | cell) */
unsigned long queue_maze[QUEUE_SIZE];
unsigned long queue_state[QUEUE_SIZE];

unsigned long best_moves; /* longest solution found so far */
unsigned long genome;     /* layout currently being evolved */

/* the message shown when the optimal maze is solved */
static const char message[] = "CONGRATULATIONS\t";

/* arrow pieces, spaces are transparent */
static const char up_arrow[6][7] = {
    "  jb  ", "jPWWbo", "^^WW^^", "  WW  ", "      ", "      "};
static const char down_arrow[6][7] = {
    "      ", "      ", "  WW  ", "__WW__", "^9WWP^", "  7F  "};
static const char left_arrow[3][12] = {
    " .jW       ", "<WWWWWWWW  ", " '9W       "};
static const char right_arrow[3][12] = {
    "       Wq. ", "  WWWWWWWWB", "       WP' "};

/* frames of the rotation animation drawn when leaving a cell */
static const char rotation_frames[4][6][12] = {
    {"........##.", "......#####", "...#...##..",
     "###....##..", ".#######...", ".....##...."},
    {"...........", "........##.", ".##....####",
     "####....##.", "..###..##..", ".....##...."},
    {"....####...", "....#.##...", ".......##..",
     ".#......##.", "##########.", ".###......."},
    {"...###.....", "...#.###...", "........##.",
     ".......####", "..#.####...", "..####....."},
};

/* 1 when a 2-bit value is 1 or 2 (a middle row/column, or a piece that
   rotates by flipping its high bit) */
static int is_middle(unsigned long value)
{
    value %= 4;
    return value == 1 || value == 2;
}

static unsigned long piece_at(unsigned long maze, unsigned long cell)
{
    return (maze >> 2 * cell) & 3;
}

/* rotate the piece in a cell one step: 0 -> 1 -> 3 -> 2 -> 0 */
static unsigned long rotate_piece(unsigned long maze, unsigned long cell)
{
    return maze ^ 1UL << (2 * cell + is_middle(piece_at(maze, cell)));
}

static int can_move(unsigned long piece, unsigned long cell, int axis)
{
    if (axis == VERTICAL)
        return !(piece & VERTICAL) || is_middle(cell / 4);
    return !(piece & HORIZONTAL) || is_middle(cell);
}

/* value to xor with the cell number to reach the neighbour */
static unsigned long neighbour_step(unsigned long piece, int axis)
{
    if (axis == VERTICAL)
        return piece & VERTICAL ? 15 : 7;
    return piece & HORIZONTAL ? 3 : 1;
}

/* number of moves needed to leave the maze, 0 if unsolvable or the
   search queue overflows */
static unsigned short solve(unsigned long maze)
{
    unsigned short tail = 1;
    unsigned short head = (unsigned short)-1;
    int axis;

    queue_maze[0] = maze;
    queue_state[0] = START_CELL;
    while (++head != tail) {
        unsigned long current = queue_maze[head];
        unsigned long state = queue_state[head] + MOVE_COUNT;
        unsigned long cell = state & 15;
        unsigned long piece = piece_at(current, cell);

        if (cell == EXIT_CELL && (current & VERTICAL))
            return state >> 4;

        for (axis = VERTICAL; axis >= HORIZONTAL; axis--) {
            if (can_move(piece, cell, axis)) {
                queue_maze[tail] = rotate_piece(current, cell);
                queue_state[tail] = state ^ neighbour_step(piece, axis);
                tail++;
            }
        }
    }
    return 0;
}

/* step through all masks of three distinct bits low < mid < high */
static int next_mutation(unsigned long *low, unsigned long *mid,
                         unsigned long *high)
{
    *low *= 2;
    if (*low < *mid)
        return 1;
    *low = 1;
    *mid <<= 1;
    if (*mid < *high)
        return 1;
    *mid = 2;
    *high *= 2;
    return *high != 0;
}

/* genetic algorithm with a population of one: keep flipping three bits
   of the layout and keep any change that makes the maze harder.
   Returns 1 when the optimum is found or a key is pressed, 0 when all
   mutations were tried and it should restart from a new random maze. */
static int evolve(void)
{
    unsigned long high = 4, mid = 1, low = 1;
    unsigned long candidate, moves;
    unsigned long current_moves = solve(genome = rand());

    while (next_mutation(&low, &mid, &high)) {
        while ((moves = solve(candidate = genome ^ high ^ mid ^ low)) >
               current_moves) {
            genome = candidate;
            current_moves = moves;
            if (current_moves > best_moves) {
                best_moves = current_moves;
                mvprintw(2, 40, "%d", (int)current_moves);
            }
            if (current_moves >= TARGET_MOVES)
                return 1;
            mid = 1;
            high = 2;
        }
        if (getch() != ERR)
            return 1;
    }
    return 0;
}

static void draw_sprite(const char *rows, int height, int width, int top,
                        int left)
{
    int y, x;
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            if (rows[y * (width + 1) + x] != ' ')
                mvaddch(top + y, left + x, rows[y * (width + 1) + x]);
}

/* draw a cell: one vertical and one horizontal arrow, dir & VERTICAL
   means up and dir & HORIZONTAL means right */
static void draw_cell(unsigned long dir, int top, int left)
{
    draw_sprite(dir & VERTICAL ? up_arrow[0] : down_arrow[0], 6, 6, top,
                left + (dir & HORIZONTAL ? 0 : 5));
    draw_sprite(dir & HORIZONTAL ? right_arrow[0] : left_arrow[0], 3, 11,
                top + (dir & VERTICAL ? 3 : 0), left);
}

/* animate the piece of the cell being left as it rotates */
static void animate_rotation(unsigned long cell, unsigned long dir, int top,
                             int left)
{
    unsigned long deadline = clock();
    int frame, t;

    for (frame = 0; frame < 4; frame++) {
        int axis = ((frame / 2) ^ (cell & 1)) ? 2 : 1;
        int mirror_x = ((dir ^ 2) & axis) != 0;
        int mirror_y = ((3 - dir) & (3 - axis)) != 0;
        const char(*image)[12] =
            rotation_frames[is_middle(frame) + 2 * is_middle(dir)];

        attrset(COLOR_PAIR(cell % 2 + 1));
        /* 64 steps cover the 6 x 11 block (minus two corner cells)
           because 6 and 11 share no factor */
        for (t = 0; t < 64; t++) {
            int y = t % 6, x = t % 11;
            int src_y = mirror_y ? 5 - y : y;
            int src_x = mirror_x ? 10 - x : x;
            mvaddch(top + y, left + x,
                    image[src_y][src_x] == '#' ? 'W' : ' ');
        }
        refresh();
        deadline += 1 << 16;
        while (deadline > clock())
            ;
    }
}

/* reveal the message one letter at a time, waiting one second for each
   of the first few letters, then wait for a key */
static unsigned short show_message(int top, int left, unsigned long step,
                                   unsigned long remaining,
                                   unsigned long deadline)
{
    attrset(A_BOLD * (step < remaining) | COLOR_PAIR(step % 2 + 1));
    if (!remaining) {
        if (step)
            getch();
        return deadline ^ endwin();
    }
    deadline += (unsigned long)(step < remaining) << 20;
    mvaddch(top, left + step % 16 * 3 - 34, message[step % 16]);
    refresh();
    while (deadline > clock())
        ;
    return deadline ^ show_message(top, left, step + 3, remaining - 1, deadline);
}

int main()
{
    unsigned long maze = 0, cell = 0, moves_left = 0;
    unsigned long orientation = 0; /* screen orientation of current cell */
    int key = 'r';

    srand(time(0));
    initscr();
    cbreak();
    noecho();
    start_color();
    nodelay(stdscr, TRUE);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    while (!evolve())
        ;

    nodelay(stdscr, FALSE);
    init_pair(1, COLOR_GREEN, COLOR_BLACK);

    while (key != 'q') {
        unsigned long piece, dir = 0, i;
        int top = 0, left = 0;

        if (key == 'r') {
            cell = START_CELL;
            orientation = 1;
            maze = genome;
            moves_left = best_moves;
        }

        clear();
        /* draw every cell, ending with the current one; the
           orientation flips between neighbours along the snake */
        for (i = 1; i <= CELLS; i++) {
            unsigned long c = (cell + i) & 15;
            unsigned long row = c / 4;
            unsigned long column = (row % 2 * 3) ^ 3 ^ (c % 4);

            orientation ^= (c & 3) ? HORIZONTAL : VERTICAL;
            dir = (orientation ^ piece_at(maze, c)) & 3;
            top = row * CELL_HEIGHT + MAZE_TOP;
            left = column * CELL_WIDTH;
            attrset(A_BOLD * (c == cell) | COLOR_PAIR(c % 2 + 1));
            draw_cell(dir, top, left);
        }
        mvprintw(2, 40, "%d", (int)moves_left);

        if (cell == EXIT_CELL && (maze & VERTICAL))
            return show_message(top, left, clear(),
                                best_moves >= TARGET_MOVES ? 16 : 0, clock());

        key = getch();
        piece = piece_at(maze, cell);
        {
            int axis = 0;
            if (can_move(piece, cell, VERTICAL) && moves_left &&
                key == (dir & VERTICAL ? 'w' : 's'))
                axis = VERTICAL;
            else if (can_move(piece, cell, HORIZONTAL) && moves_left &&
                     key == (dir & HORIZONTAL ? 'd' : 'a'))
                axis = HORIZONTAL;

            if (axis) {
                --moves_left;
                if (cell & 1)
                    dir ^= 2 - is_middle(dir);
                animate_rotation(cell, dir, top, left);
                orientation ^= axis;
                maze = rotate_piece(maze, cell);
                cell ^= neighbour_step(piece, axis);
            }
        }
    }
    return endwin();
}
