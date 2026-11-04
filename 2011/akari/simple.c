#include <stdio.h>

/*
 * Akari: downsample a PGM (P5), PPM (P6) or ASCII art image by half in
 * each direction.  By default the odd pixels of the odd scanlines are kept;
 * with a third argument the even pixels of the even scanlines are kept.
 */

#define BUFFER_SIZE 1024

static FILE *input;
static char buffer[BUFFER_SIZE];
static int buffer_length;
static int buffer_position;

/* Return the next input byte, or EOF once the input is exhausted. */
static int next_byte(void)
{
    if (buffer_position == buffer_length) {
        buffer_length = fread(buffer, 1, BUFFER_SIZE, input);
        if (buffer_length <= 0)
            return EOF;
        buffer_position = 0;
    }
    return (unsigned char)buffer[buffer_position++];
}

/*
 * Downsample raw netpbm pixel data.  The header is skipped first by
 * counting three lines.  The byte right after each newline is skipped
 * without being looked at, so empty header lines are not counted.
 */
static void downsample_netpbm(FILE *output, int bytes_per_pixel, int width,
                              int keep_parity)
{
    int c;
    int header_lines = 3;
    int row_parity = 0, column_parity = 0;
    int pixels_left_in_row = width;
    int byte_in_pixel = 0;

    if ((c = next_byte()) == EOF)
        return;
    for (;;) {
        if (c == '\n') {
            if ((c = next_byte()) == EOF)
                return;
            if (--header_lines == 0)
                break;
        }
        if ((c = next_byte()) == EOF)
            return;
    }

    /* c now holds the first byte of pixel data */
    do {
        if (row_parity == keep_parity && column_parity == keep_parity)
            fputc(c, output);

        if (++byte_in_pixel == bytes_per_pixel) {
            byte_in_pixel = 0;
            column_parity ^= 1;
            if (--pixels_left_in_row == 0) {
                pixels_left_in_row = width;
                row_parity ^= 1;
                column_parity = 0;
            }
        }
    } while ((c = next_byte()) != EOF);
}

/* Downsample ASCII art: keep every other character of every other line. */
static void downsample_text(FILE *output, int keep_parity)
{
    int c;
    int line_parity = 0, column_parity = 0;

    while ((c = next_byte()) != EOF) {
        if (c == '\n') {
            if (line_parity == keep_parity)
                fputc(c, output);
            line_parity ^= 1;
            column_parity = 0;
        } else {
            if (line_parity == keep_parity && column_parity == keep_parity)
                fputc(c, output);
            column_parity ^= 1;
        }
    }
}

int main(int argc, char **argv)
{
    const char *header_format = "P%d\n%d %d\n%d\n";
    FILE *output;
    int magic, width, height, max_value;
    int keep_even = argc > 3;
    int keep_parity = keep_even ? 0 : 1;

    /* "-" or a missing argument means standard input / standard output */
    input = (argc > 1 && (argv[1][0] != '-' || argv[1][1] != '\0'))
                ? fopen(argv[1], "rb")
                : stdin;
    output = (argc < 3 || (argv[2][0] == '-' && argv[2][1] == '\0'))
                 ? stdout
                 : fopen(argv[2], "wb+");
    if (!input || !output)
        return printf("Can not open %s for %sing\n", argv[!input ? 1 : 2],
                      !input ? "read" : "writ");

    buffer_length = fread(buffer, 1, BUFFER_SIZE, input);
    buffer_position = 0;

    if (buffer_length > 2 && buffer[0] == 'P' &&
        sscanf(buffer, header_format, &magic, &width, &height, &max_value) ==
            4 &&
        (magic == 6 || magic == 5) && max_value == 255) {
        int new_width = width, new_height = height;
        if (keep_even) {
            new_width++;
            new_height++;
        }
        fprintf(output, header_format, magic, new_width >> 1,
                new_height >> 1, max_value);
        downsample_netpbm(output, magic == 6 ? 3 : 1, width, keep_parity);
    } else {
        downsample_text(output, keep_parity);
    }

    fclose(input);
    return fclose(output);
}
