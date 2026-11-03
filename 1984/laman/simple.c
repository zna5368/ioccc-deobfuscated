/*
 * Fill a grid with the numbers 1..n in a square spiral, starting near the
 * middle, then print it row by row (mirrored left to right).
 *
 * The grid is stored flat: the cell at column x, row y is grid[x * 32 + y].
 */
int grid[900];

int cell_index(int column, int row)
{
    return column << 5 | row;
}

int main(int argc, char **argv)
{
    int limit, columns, rows;
    int column, row;
    int number = 1;
    int direction = 1;
    int run_length = 0;
    int step;

    (void)argc;
    limit = atoi(argv[1]);

    /* columns = ceil(sqrt(limit)); start in the middle column */
    for (columns = 0; columns * columns < limit; columns++)
        ;
    column = (columns - 1) >> 1;

    /* rows = floor(sqrt(limit)), plus one if the spiral needs another row */
    for (rows = 0; rows * rows <= limit; rows++)
        ;
    rows--;
    if (limit > rows * (rows + 1))
        rows++;
    row = (rows - 1) >> 1;

    /*
     * Walk the spiral: run_length steps along columns, run_length steps
     * along rows, then turn around and make each run one longer.
     */
    while (number <= limit) {
        run_length++;
        for (step = 0; step < run_length && number <= limit; step++) {
            grid[cell_index(column, row)] = number++;
            column += direction;
        }
        for (step = 0; step < run_length && number <= limit; step++) {
            grid[cell_index(column, row)] = number++;
            row += direction;
        }
        direction = -direction;
    }

    for (row = 0; row < rows; row++) {
        for (column = 0; column < columns; column++) {
            /* mirror the row in place by swapping with the opposite column */
            if (column < columns / 2) {
                int mirror = columns - (column + 1);
                int temp = grid[cell_index(column, row)];
                grid[cell_index(column, row)] = grid[cell_index(mirror, row)];
                grid[cell_index(mirror, row)] = temp;
            }
            if (grid[cell_index(column, row)])
                printf("%-4d", grid[cell_index(column, row)]);
            else
                printf("    ");
        }
        putchar('\n');
    }
    return 0;
}
