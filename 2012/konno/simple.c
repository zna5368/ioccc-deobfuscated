/*
 * Elementary cellular automaton printer.
 *
 * The first command line argument is a Wolfram rule number.  The program
 * prints 16 generations of a row of 31 cells, starting from one live cell in
 * the middle.  Cells past either end of the row copy the cell at that end.
 */

#define GENERATIONS 16
#define ROW_WIDTH 31
#define MIDDLE_CELL 16

#define ALIVE_CHAR '#'
#define DEAD_CHAR ' '

/*
 * Cells are numbered from 1 to ROW_WIDTH.  Slots 0 and ROW_WIDTH + 1 are the
 * "ghost" cells just outside the row, so every real cell has two neighbours.
 */
#define ROW_SLOTS (ROW_WIDTH + 2)

static void print_row(const int *row)
{
    int column;

    for (column = 1; column <= ROW_WIDTH; column++) {
        putchar(row[column] ? ALIVE_CHAR : DEAD_CHAR);
    }
    putchar('\n');
}

/*
 * The three cells form a number from 0 to 7, and that bit of the rule number
 * is the next state of the middle cell.
 */
static int next_state(int rule, int left, int self, int right)
{
    int neighbourhood = left * 4 + self * 2 + right;

    return (rule >> neighbourhood) & 1;
}

static void advance_row(int rule, int *row)
{
    int next_row[ROW_SLOTS];
    int column;

    row[0] = row[1];
    row[ROW_WIDTH + 1] = row[ROW_WIDTH];

    for (column = 1; column <= ROW_WIDTH; column++) {
        next_row[column] =
            next_state(rule, row[column - 1], row[column], row[column + 1]);
    }
    for (column = 1; column <= ROW_WIDTH; column++) {
        row[column] = next_row[column];
    }
}

int main(int argc, char **argv)
{
    int row[ROW_SLOTS] = {0};
    int rule;
    int generation;

    (void)argc; /* like the original, argv[1] is assumed to exist */
    rule = atoi(argv[1]);

    row[MIDDLE_CELL] = 1;
    for (generation = 0; generation < GENERATIONS; generation++) {
        print_row(row);
        advance_row(rule, row);
    }
    return 0;
}
