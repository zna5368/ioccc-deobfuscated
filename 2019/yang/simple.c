#include<stdio.h>
#include<stdlib.h>

/*
 * Text scrambler / unscrambler for terminals.
 *
 * The original prog.c is also a Ruby script.  Ruby sees this line, which
 * prints its input slowly, pausing after every printed character that is not
 * part of an ANSI escape sequence:
 *
 *   e=nil;ARGF.each_char{|c|print c;e=e ?c<"A"||c>"Z":c=="\e"?1:!(sleep 0.1)}
 *
 * and the C code that follows is swallowed by a Ruby %q{...} string literal.
 */

/* One visible character placed on the virtual screen. */
typedef struct {
    int x;      /* column */
    int y;      /* row */
    int code;   /* Unicode code point */
} Glyph;

/* Inclusive ranges of code points that take up two terminal columns. */
static const int wide_ranges[][2] = {
    { 0x01100, 0x0115F },   /* Hangul Jamo */
    { 0x02329, 0x0232A },   /* angle brackets */
    { 0x02E80, 0x0303E },   /* CJK radicals, punctuation */
    { 0x03040, 0x0A4CF },   /* Kana, CJK ideographs, Yi */
    { 0x0AC00, 0x0D7A3 },   /* Hangul syllables */
    { 0x0F900, 0x0FAFF },   /* CJK compatibility ideographs */
    { 0x0FE10, 0x0FE19 },   /* vertical forms */
    { 0x0FE30, 0x0FE6F },   /* CJK compatibility forms */
    { 0x0FF00, 0x0FF60 },   /* full-width forms */
    { 0x0FFE0, 0x0FFE6 },   /* full-width signs */
    { 0x1F000, 0x3FFFF },   /* emoji and supplementary ideographs */
};
#define WIDE_RANGE_COUNT (int)(sizeof(wide_ranges) / sizeof(wide_ranges[0]))

static Glyph *glyphs;           /* 2 * capacity entries; upper half is sort scratch */
static int glyph_count;
static int capacity = 1;

static int cursor_x, cursor_y;  /* current cursor position */
static int min_row, max_row;    /* rows spanned by the glyphs (always include 0) */

static FILE *stream;

/* Number of terminal columns used by a code point. */
static int char_width(int code)
{
    int i;

    for (i = 0; i < WIDE_RANGE_COUNT; i++) {
        if (code >= wide_ranges[i][0] && code <= wide_ranges[i][1])
            return 2;
    }
    return 1;
}

/* Record a visible character at the cursor, then advance the cursor. */
static void add_glyph(int code)
{
    if (glyph_count + 2 > capacity) {
        capacity *= 2;
        glyphs = (Glyph *)realloc(glyphs, 2 * capacity * sizeof(Glyph));
    }
    glyphs[glyph_count].x = cursor_x;
    glyphs[glyph_count].y = cursor_y;
    glyphs[glyph_count].code = code;
    glyph_count++;

    if (cursor_y < min_row)
        min_row = cursor_y;
    if (cursor_y > max_row)
        max_row = cursor_y;

    cursor_x += char_width(code);
}

/* Write a code point as UTF-8. */
static void put_utf8(int code)
{
    unsigned char bytes[4];
    int n = 0;

    if (code < 0x80) {
        bytes[n] = code;
    } else {
        /* emit 6-bit continuation bytes until the rest fits in the lead byte */
        while (code > (63 >> n)) {
            bytes[n++] = 0x80 | (code & 0x3F);
            code >>= 6;
        }
        /* lead byte prefix: 110xxxxx, 1110xxxx or 11110xxx */
        bytes[n] = ((0x780 >> n) & 0xF0) | code;
    }
    while (n >= 0)
        fputc(bytes[n--], stream);
}

/* ANSI cursor movement: ESC [ count letter */
static void move_vertical(int rows)
{
    if (rows)
        fprintf(stream, "\33[%d%c", abs(rows), rows > 0 ? 'B' : 'A');
}

static void move_horizontal(int columns)
{
    if (columns)
        fprintf(stream, "\33[%d%c", abs(columns), columns > 0 ? 'C' : 'D');
}

/* Ordering used to sort glyphs top-to-bottom, left-to-right. */
static double position_key(const Glyph *g)
{
    return g->y * 1e6 + g->x;
}

/* Stable merge sort of glyphs[lo, hi) using the area after the glyphs as scratch. */
static void sort_glyphs(int lo, int hi)
{
    Glyph *scratch = glyphs + glyph_count;
    int mid = (lo + hi) / 2;
    int left, right, k;

    if (mid == lo)
        return;

    sort_glyphs(lo, mid);
    sort_glyphs(mid, hi);

    left = lo;
    right = mid;
    for (k = lo; k < hi; k++) {
        if (left >= mid ||
            (right < hi && position_key(&glyphs[left]) > position_key(&glyphs[right])))
            scratch[k] = glyphs[right++];
        else
            scratch[k] = glyphs[left++];
    }
    for (k = lo; k < hi; k++)
        glyphs[k] = scratch[k];
}

/*
 * Read text (which may contain cursor movement escape codes) and place every
 * visible character at the position a terminal would draw it.  Returns a hash
 * of the raw input bytes which seeds the shuffle.
 */
static int read_screen(void)
{
    int c;
    int hash = 0;
    int state = 0;      /* <0: in escape sequence, >0: UTF-8 bytes still expected */
    int value = 0;      /* escape sequence number or partial UTF-8 code point */

    while ((c = fgetc(stream)) > 0) {
        hash = (int)((unsigned)hash + (((unsigned)hash + c) << 10));
        hash ^= hash >> 6;

        if (state < 0) {
            /* inside "ESC [ number letter" */
            if (c == '[') {
                /* stay in escape mode */
            } else if (c >= '0' && c <= ';') {
                value = value * 10 + (c - '0');
                state = ~value;
            } else {
                /* 'A' (up) and 'D' (left) move in the negative direction */
                value *= (c & 2) ? 1 : -1;
                if (c == 'A' || c == 'B') {
                    cursor_y += value;
                } else if (c < 'E') {
                    cursor_x += value;
                    if (cursor_x < 0)
                        cursor_x = 0;
                }
                state = 0;
            }
        } else if (state > 0) {
            /* UTF-8 continuation byte */
            value = (value << 6) | (c & 0x3F);
            if (--state == 0)
                add_glyph(value);
        } else if ((c & 0xE0) == 0xC0) {
            value = c & 0x1F;
            state = 1;
        } else if ((c & 0xF0) == 0xE0) {
            value = c & 0x0F;
            state = 2;
        } else if ((c & 0xF8) == 0xF0) {
            value = c & 0x07;
            state = 3;
        } else if (c == 27) {
            value = 0;
            state = -1;
        } else if (c == '\n') {
            cursor_y++;
            cursor_x = 0;
        } else if (c == ' ') {
            cursor_x++;
        } else if (c == '\t') {
            cursor_x = (cursor_x + 8) & ~7;
        } else if (c > ' ') {
            add_glyph(c);
        }
        /* all other control characters are ignored */
    }
    return hash;
}

/* Print the glyphs as plain text without any escape codes. */
static void unscramble(void)
{
    int h = 0;

    sort_glyphs(0, glyph_count);

    cursor_x = 0;
    cursor_y = min_row;
    while (h < glyph_count) {
        for (; cursor_y < glyphs[h].y; cursor_y++) {
            put_utf8('\n');
            cursor_x = 0;
        }
        for (; cursor_x < glyphs[h].x; cursor_x++)
            put_utf8(' ');

        /* of several glyphs drawn at the cursor, only the last one is visible */
        do {
            h++;
        } while (h < glyph_count && glyphs[h].y == cursor_y && glyphs[h].x == cursor_x);

        put_utf8(glyphs[h - 1].code);
        cursor_x += char_width(glyphs[h - 1].code);
    }
    if (cursor_x)
        put_utf8('\n');
}

static int open_output(const char *name)
{
    stream = fopen(name, "w");
    if (!stream) {
        perror(name);
        return 0;
    }
    return 1;
}

/* Print the glyphs in random order, split across part_count outputs. */
static int scramble(int seed, int part_count, char **names)
{
    int i, j, part, end, h;
    Glyph tmp;

    /* Fisher-Yates shuffle */
    srand(seed);
    for (i = glyph_count - 1; i > 0; i--) {
        j = rand() % (i + 1);
        tmp = glyphs[i];
        glyphs[i] = glyphs[j];
        glyphs[j] = tmp;
    }

    if (part_count == 0) {
        part_count = 1;         /* everything goes to stdout */
    } else if (!open_output(names[0])) {
        return part_count;
    }

    h = 0;
    cursor_x = 0;
    cursor_y = max_row + 1;
    for (part = 0; h < glyph_count && part < part_count; part++) {
        if (part > 0 && !open_output(names[part]))
            return part_count;

        /* first part reserves the screen area by scrolling blank lines */
        if (part == 0) {
            for (i = 0; i < max_row - min_row + 1; i++)
                put_utf8('\n');
        }

        end = glyph_count * (part + 1) / part_count;
        for (; h < end; h++) {
            move_vertical(glyphs[h].y - cursor_y);
            move_horizontal(glyphs[h].x - cursor_x);
            put_utf8(glyphs[h].code);
            cursor_x = glyphs[h].x + char_width(glyphs[h].code);
            cursor_y = glyphs[h].y;
        }

        /* leave the cursor just below the picture */
        move_vertical(max_row - cursor_y);
        put_utf8('\n');
        cursor_y = max_row + 1;
        cursor_x = 0;

        fclose(stream);
    }
    return 0;
}

int main(int argc, char **argv)
{
    int arg_count = argc - 1;
    int seed;

    stream = stdin;
    if (arg_count && !(argv[1][0] == '-' && argv[1][1] == '\0')) {
        stream = fopen(argv[1], "r");
        if (!stream) {
            perror(argv[1]);
            return arg_count;
        }
    }

    seed = read_screen();
    fclose(stream);
    stream = stdout;

    if (glyphs) {
        if (arg_count < 1)
            unscramble();
        else
            return scramble(seed, arg_count - 1, argv + 2);
    }
    return 0;
}
