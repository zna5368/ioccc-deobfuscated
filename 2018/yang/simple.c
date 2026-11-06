#include <stdio.h>

/*
 * Rotate text from stdin 90 degrees counterclockwise and write it to stdout.
 *
 * Column c of input line l is stored in grid[c][l].  Printing the rows of
 * the grid from the last column back to the first gives the rotated text.
 * Only the first 1024 columns of the first 1024 lines are used.
 */

#define MAX_COLUMNS 1024
#define MAX_LINES   1024

/* one row per input column, one cell per input line plus a terminator */
static char grid[MAX_COLUMNS][MAX_LINES + 1];

int main(void)
{
    int column, line, row, ch;
    int widest = 0;  /* length of the longest input line */

    /* fill the grid with spaces so short lines are padded */
    for (row = 0; row < MAX_COLUMNS; row++)
        for (line = 0; line <= MAX_LINES; line++)
            grid[row][line] = ' ';

    column = 0;
    line = 0;
    while ((ch = getchar()) != EOF && line < MAX_LINES) {
        if (ch == '\n') {
            column = 0;
            line++;
        } else if (column < MAX_COLUMNS) {
            grid[column][line] = ch;
            column++;
            if (widest < column)
                widest = column;
        }
    }

    /*
     * The last input column becomes the first output line.  Each output
     * line is as long as the number of complete input lines, so text after
     * the final newline is cut off by the terminator.
     */
    for (row = widest - 1; row >= 0; row--) {
        grid[row][line] = '\0';
        puts(grid[row]);
    }

    return 0;
}
