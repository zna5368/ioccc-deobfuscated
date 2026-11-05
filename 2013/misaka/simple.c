/*
 * Horizontal cat: join text files side by side.
 *
 * Usage: ./simple [files...]
 *
 * Every file is loaded into memory first, so a file name given twice, or "-"
 * for standard input given twice, is read once and printed twice.  Each file
 * is padded with spaces on the right up to its widest line so that its columns
 * stay lined up.
 *
 * The original has no options.  It changes its behavior when copies of its own
 * source are pasted together and compiled, so here those copies are two
 * numbers that can be set when compiling:
 *
 *   COPIES_WIDE  copies side by side in the first row of the tiled source
 *   COPIES_HIGH  rows of copies stacked on top of each other
 *
 *   one copy                  horizontal cat, the default
 *   -DCOPIES_WIDE=2           vertical cat, like cat(1) but "-" can repeat
 *   -DCOPIES_HIGH=2           long cat, prints a picture of a cat instead
 *
 * Each extra row doubles the length of the long cat and each extra column
 * widens every character of it.
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifndef COPIES_WIDE
#define COPIES_WIDE 1
#endif
#ifndef COPIES_HIGH
#define COPIES_HIGH 1
#endif

#define LONG_CAT_MODE (COPIES_HIGH > 1)
#define VERTICAL_MODE (COPIES_WIDE > 1)

#define READ_CHUNK (1 << 16)

/*
 * Width of a file that stops the next argument from restarting its duplicate
 * search.  This is the line number of that restart in the original, see
 * keep_position below.
 */
#define SEARCH_RESTART_LINE 21

typedef struct {
    int width;       /* longest line */
    int lines;       /* number of lines */
    int length;      /* bytes of text */
    int capacity;    /* bytes allocated for text, not counting the end marker */
    char *text;      /* whole file, followed by one extra newline */
    char *cursor;    /* start of the next line to print */
} InputFile;

static const char cat_head[] =
    "\n"
    "                                      o\"\"8o\n"
    "                                     o\"  \"8\n"
    "                      o88ooo        o8    8\n"
    "                      \"8  \"\"8ooo8\"\"\"\"     8o\n"
    "                       \"8o          oooo  88\n"
    "                         8 o\"\"8  o\"8\"  \"\"oo8ooo\n"
    "      o\"\"\"\"oo\"\"\"\"\"\"88oooo8 8oo8  8o8o         \"8o\n"
    "      8o                 8  \"\"      \"\"\"8oo      \"8o\n"
    "       \"8ooooooooo       8        o      \"8o      8\n"
    "                 \"8oo    \"o   \"\"\"\"      o8\"8ooooo8\"\n"
    "                    \"8oo  \"8ooo    ooo8\"\"    \"8\n"
    "                       \"\"ooo888\"\"\"\"\"          8\n";

/* The cat gets longer by repeating this row. */
static const char cat_body[] = "                              8               8\n";

static InputFile *files;

/*
 * Shared between looking for an earlier copy of an argument, where it is an
 * argument number, and measuring lines, where it is an offset into a file.
 */
static int position;

/*
 * The search for an earlier copy of an argument normally restarts from the
 * first argument.  It carries on from position instead straight after a
 * duplicate was found, or when the widest line before the last line of the
 * previous file was exactly SEARCH_RESTART_LINE long.
 */
static int keep_position;

/* Print text with every character except newline repeated COPIES_WIDE times. */
static void print_widened(const char *text)
{
    int repeat;

    for (; *text; text++) {
        for (repeat = *text == '\n' ? 1 : COPIES_WIDE; repeat > 0; repeat--) {
            putchar(*text);
        }
    }
}

/* Each row of stacked copies doubles the number of body rows. */
static void print_long_cat(void)
{
    unsigned long row;

    print_widened(cat_head);
    for (row = 0; row < 1UL << COPIES_HIGH; row++) {
        print_widened(cat_body);
    }
}

/*
 * Read all of stream and close it.  Returns 0 if memory ran out.  The extra
 * newline after the text means that a last line with no newline of its own
 * still ends like the others.
 */
static int load_file(InputFile *file, FILE *stream)
{
    int got;

    do {
        file->capacity += READ_CHUNK;
        file->text = realloc(file->text, file->capacity + 1);
        if (!file->text) {
            return 0;
        }
        got = fread(file->text + file->length, 1, READ_CHUNK, stream);
        file->length += got;
    } while (got == READ_CHUNK);

    file->text[file->length] = '\n';
    fclose(stream);
    file->cursor = file->text;
    return 1;
}

/* Count the lines of a file and find the longest one. */
static void measure_lines(InputFile *file)
{
    int start;
    int width;

    for (start = 0; start < file->length; start = position + 1) {
        position = start;
        while (position < file->length && file->text[position] != '\n') {
            position++;
        }
        width = position - start;

        keep_position = file->width == SEARCH_RESTART_LINE;
        if (width > file->width) {
            file->width = width;
        }
        file->lines++;
    }
}

/* Print the next line of a file, padded with spaces to the width of the file. */
static void print_padded_line(InputFile *file)
{
    int printed = 0;

    if (file->cursor - file->text < file->length) {
        while (*file->cursor != '\n') {
            putchar(*file->cursor++);
            printed++;
        }
        file->cursor++;
    }
    while (printed++ < file->width) {
        putchar(' ');
    }
}

static void print_side_by_side(int argc)
{
    int most_lines = 0;
    int arg;
    int row;

    for (arg = 1; arg < argc; arg++) {
        if (files[arg].lines > most_lines) {
            most_lines = files[arg].lines;
        }
    }

    for (row = 0; row < most_lines; row++) {
        for (arg = 1; arg < argc; arg++) {
            print_padded_line(&files[arg]);
        }
        putchar('\n');
    }
}

static void print_one_after_another(int argc)
{
    int arg;

    for (arg = 1; arg < argc; arg++) {
        fwrite(files[arg].text, files[arg].length, 1, stdout);
    }
}

int main(int argc, char **argv)
{
    int arg;

    if (LONG_CAT_MODE) {
        print_long_cat();
        return 0;
    }

    files = calloc(argc, sizeof(InputFile));
    if (!files) {
        return 1;
    }

    for (arg = 1; arg < argc; arg++) {
        FILE *stream = stdin;

        if (!keep_position) {
            position = 1;
        }
        keep_position = 1;

        /* A repeated name shares the text that was already loaded. */
        while (position < arg && strcmp(argv[arg], argv[position])) {
            position++;
        }
        if (position < arg) {
            files[arg] = files[position];
            continue;
        }

        if (strcmp(argv[arg], "-")) {
            stream = fopen(argv[arg], "rb");
            if (!stream) {
                printf("Can not open %s\n", argv[arg]);
                return 1;
            }
        }
        if (!load_file(&files[arg], stream)) {
            return 1;
        }

        keep_position = 0;
        if (!VERTICAL_MODE) {
            measure_lines(&files[arg]);
        }
    }

    if (VERTICAL_MODE) {
        print_one_after_another(argc);
    } else {
        print_side_by_side(argc);
    }
    return 0;
}
