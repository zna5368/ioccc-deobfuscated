#include<stdio.h>

#define LINE_SIZE 256

/*
 * Character drawn for a space in the current line, indexed by which of
 * its six neighbouring cells (in the lines above and below) are also
 * spaces:
 *
 *   bit 0 (1)  below-left     bit 3 (8)   above-left
 *   bit 1 (2)  below          bit 4 (16)  above
 *   bit 2 (4)  below-right    bit 5 (32)  above-right
 *
 * Each row of the table below is one combination of the "above" bits;
 * the eight entries in a row cover every combination of the "below" bits.
 * Non-space characters are drawn as table entry 0, a blank.
 */
static const char river_chars[64] =
    " ......."      /* nothing above                  */
    "')|)\\\\\\\\"  /* above-left                     */
    "'|||||||"      /* above                          */
    "')|)\\\\\\\\"  /* above-left + above             */
    "'/|/(/(/"      /* above-right                    */
    "'/|/\\|\\|"    /* above-left + above-right       */
    "'/|/(/(/"      /* above + above-right            */
    "'/|/\\|\\|";   /* above-left + above + above-right */

/* Is line[pos] a space?  Only positions inside the string count. */
static int is_space(const char *line, int pos)
{
    for (int i = 0; i <= pos; i++) {
        if (line[i] == 0) {
            return 0;
        }
    }
    return line[pos] == ' ';
}

int main()
{
    char buffers[3][LINE_SIZE] = { { 0 } };
    char *prev_line = buffers[0];
    char *curr_line = buffers[1];
    char *next_line = buffers[2];
    char *have_curr;
    char *have_next = fgets(next_line, LINE_SIZE, stdin);

    for (;;) {
        /* shift the three-line window down by one line */
        char *spare = prev_line;
        spare[0] = 0;
        prev_line = curr_line;
        curr_line = next_line;
        next_line = spare;

        have_curr = have_next;
        have_next = fgets(next_line, LINE_SIZE, stdin);
        if (!have_curr) {
            break;
        }

        for (int col = 0; curr_line[col]; col++) {
            int index = 0;

            if (curr_line[col] == ' ') {
                if (col > 0 && is_space(next_line, col - 1)) index |= 1;
                if (is_space(next_line, col))                index |= 2;
                if (is_space(next_line, col + 1))            index |= 4;
                if (col > 0 && is_space(prev_line, col - 1)) index |= 8;
                if (is_space(prev_line, col))                index |= 16;
                if (is_space(prev_line, col + 1))            index |= 32;
            }
            putchar(river_chars[index]);
        }
        putchar('\n');
    }
}
