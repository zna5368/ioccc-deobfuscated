#include <stdio.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 100

int line_count = 0;
int line;
int max_length = 0;
int column = 0;

/*
 * Transpose standard input: row N of the input becomes column N of the
 * output.  Lines are not stored; only their length and starting offset in
 * the file are remembered, and characters are later fetched with fseek().
 */
main()
{
    char buffer[MAX_LINE_LENGTH], ch;
    int length[MAX_LINES], offset[MAX_LINES];

    /* first pass: record where each line starts and how long it is */
    offset[0] = ftell(stdin);
    while (gets(buffer)) {
        length[line_count] = strlen(buffer);
        if (length[line_count] > max_length)
            max_length = length[line_count];
        offset[++line_count] = ftell(stdin);
    }

    /* second pass: each output row is one column of the input */
    for (column = 0; column < max_length; column++) {
        for (line = 0; line < line_count; line++) {
            ch = ' ';
            if (column < length[line]) {
                fseek(stdin, offset[line] + column, SEEK_SET);
                scanf("%c", &ch);
            }
            printf("%c", ch);
        }
        printf("\n");
    }
}
