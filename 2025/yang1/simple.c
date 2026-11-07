#include <stdio.h>
#include <stdlib.h>

/*
 * Slash: remove a single line from stdin and write the rest to stdout.
 *
 *   ./prog {line_number} < input > output
 *
 *   line_number > 0   counted from the start of the file (first line is 1)
 *   line_number < 0   counted from the end of the file (last line is -1)
 *   line_number == 0  copy input unchanged
 */

#define CHUNK_SIZE 4096

static char *buffer = NULL;   /* holds the tail of the input seen so far */
static int buffer_size;       /* allocated bytes in buffer */
static int buffer_used;       /* bytes of valid data in buffer */

/* Discard everything in the buffer before new_start, shifting the rest down. */
static void discard_before(char *new_start)
{
    char *dst = buffer;

    buffer_used -= new_start - buffer;
    while (buffer + buffer_used != dst)
        *dst++ = *new_start++;
}

/* Positive (or zero) line number: stream character by character. */
static void remove_line_from_start(int target)
{
    int ch;
    int line = 1;

    while ((ch = getchar()) != EOF) {
        if (line != target)
            putc(ch, stdout);
        if (ch == '\n')
            line++;
    }
}

/*
 * Negative line number: buffer the input, but only keep the last -target
 * lines in memory.  Everything before those lines is written out as soon
 * as it is known not to be the line to remove.
 */
static int remove_line_from_end(int target)
{
    int at_eof = 0;
    int line;
    char *line_start;   /* start of the line `target` counted from the end */
    char *next_line;    /* start of the line after it */
    char *scan;         /* backwards scanning position */

    buffer_size = buffer_used = 0;
    while (!at_eof) {
        if (buffer_used == buffer_size) {
            buffer_size += CHUNK_SIZE;
            buffer = (char *)realloc(buffer, buffer_size);
            if (buffer == NULL)
                return 1 | fputs("  X_X\n", stderr);
        }
        buffer_used += fread(buffer + buffer_used, 1, buffer_size - buffer_used, stdin);
        at_eof = buffer_used < buffer_size;

        if (buffer_used == 0)
            continue;

        /* Walk backwards from the end, counting lines (last line is -1). */
        line = -1;
        next_line = buffer + buffer_used;
        scan = next_line;
        if (scan[-1] == '\n')
            scan--;                     /* ignore the final newline */
        line_start = buffer;            /* if not enough lines, keep it all */
        while (scan != buffer) {
            scan--;
            if (*scan == '\n') {
                if (line <= target) {
                    line_start = scan + 1;
                    break;
                }
                line--;
                next_line = scan + 1;
            }
        }

        /* Everything before the last -target lines can be printed now. */
        fwrite(buffer, line_start - buffer, 1, stdout);
        discard_before(line_start);

        if (at_eof) {
            /* All input seen: if the file has enough lines, drop the target. */
            if (line == target) {
                next_line -= line_start - buffer;
                discard_before(next_line);
            }
            fwrite(buffer, buffer_used, 1, stdout);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int target;

    if (argc != 2)
        return 1 | printf("%s %s\n", argv[0], "{line_number}");

    target = atoi(argv[1]);
    if (target >= 0)
        remove_line_from_start(target);
    else
        return remove_line_from_end(target);
    return 0;
}
