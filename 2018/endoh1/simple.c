#include <stdio.h>

/*
 * Each glyph is a grid of 6 x 10 cells and every cell is drawn as a block of
 * 2 x 2 pixels, so a character takes 12 x 20 pixels in the image.
 */
#define GLYPH_COLUMNS   6
#define GLYPH_ROWS      10
#define CELL_SIZE       2
#define CHAR_WIDTH      (GLYPH_COLUMNS * CELL_SIZE)    /* 12 */
#define LINE_HEIGHT     (GLYPH_ROWS * CELL_SIZE)       /* 20 */
#define MARGIN          6

/* Glyphs are stored eight to a row of the table below. */
#define GLYPHS_PER_ROW  8

#define FRAME_COUNT     63      /* background fades white -> black -> white */
#define GRAY_LEVELS     32      /* palette entries 0 (black) .. 31 (white) */

#define CLEAR_CODE      256
#define END_CODE        257
#define MAX_CODE        4094

/*
 * Font for the printable characters ' ' (32) to DEL (127).
 *
 *   '.'  empty, painted with the background color of the frame
 *   '#'  outline, always black
 *   'o'  inside of a closed letter, always white
 *
 * While the background is white only the black outlines can be seen.  When
 * the background turns black the outlines disappear and only the white
 * insides of the closed letters remain.
 */
static const char *font[] = {
    /* ' '    '!'    '"'    '#'    '$'    '%'    '&'    '\'' */
    "...... ...... .#.#.. ...... ...... ...... ...... ..#...",
    "...... ..#... .#.#.. ...... ..#... ...... .##... ..#...",
    "...... ..#... .#.#.. .#.#.. .####. .#...# #oo#.. ..#...",
    "...... ..#... ...... #####. #o#... #o#.#. #oo#.. ......",
    "...... ..#... ...... .#o#.. .###.. .#.#.. .##... ......",
    "...... ..#... ...... .#o#.. ..#o#. ..#.#. #oo#.# ......",
    "...... ...... ...... #####. ####.. .#.#o# #ooo#. ......",
    "...... ..#... ...... .#.#.. ..#... #...#. .###.# ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* '('    ')'    '*'    '+'    ','    '-'    '.'    '/' */
    "...#.. .#.... ...... ...... ...... ...... ...... ....#.",
    "..#... ..#... ...... ...... ...... ...... ...... ....#.",
    ".#.... ...#.. ..#... ..#... ...... ...... ...... ...#..",
    ".#.... ...#.. #.#.#. ..#... ...... ...... ...... ...#..",
    ".#.... ...#.. .###.. #####. ...... #####. ...... ..#...",
    ".#.... ...#.. #.#.#. ..#... ...... ...... ...... ..#...",
    ".#.... ...#.. ..#... ..#... ...... ...... ...... .#....",
    "..#... ..#... ...... ...... ..#... ...... ..#... .#....",
    "...#.. .#.... ...... ...... .#.... ...... ...... #.....",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* '0'    '1'    '2'    '3'    '4'    '5'    '6'    '7' */
    "...... ...... ...... ...... ...... ...... ...... ......",
    ".###.. ..#... .###.. .###.. ...#.. #####. ..##.. #####.",
    "#ooo#. .##... #...#. #...#. ..##.. #..... .#.... ....#.",
    "#ooo#. ..#... ....#. ....#. .#o#.. #..... #..... ...#..",
    "#o#o#. ..#... ..##.. ..##.. #oo#.. ####.. ####.. ...#..",
    "#ooo#. ..#... .#.... ....#. #####. ....#. #ooo#. ..#...",
    "#ooo#. ..#... #..... #...#. ...#.. #...#. #ooo#. ..#...",
    ".###.. .###.. #####. .###.. ...#.. .###.. .###.. .#....",
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* '8'    '9'    ':'    ';'    '<'    '='    '>'    '?' */
    "...... ...... ...... ...... ...... ...... ...... ......",
    ".###.. .###.. ...... ...... ...... ...... ...... .###..",
    "#ooo#. #ooo#. ...... ...... ....#. ...... #..... #...#.",
    "#ooo#. #ooo#. ..#... ..#... ..##.. #####. .##... ....#.",
    ".###.. .####. ...... ...... ##.... ...... ...##. ...#..",
    "#ooo#. ....#. ...... ...... ..##.. #####. .##... ..#...",
    "#ooo#. ...#.. ..#... ..#... ....#. ...... #..... ......",
    ".###.. .##... ...... .#.... ...... ...... ...... ..#...",
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* '@'    'A'    'B'    'C'    'D'    'E'    'F'    'G' */
    "...... ...... ...... ...... ...... ...... ...... ......",
    ".####. ..#... ####.. .###.. ###... #####. #####. .###..",
    "#....# ..#... #ooo#. #...#. #oo#.. #..... #..... #...#.",
    "#.##.# .#o#.. #ooo#. #..... #ooo#. #..... #..... #.....",
    "#.#o## .#o#.. ####.. #..... #ooo#. ####.. ####.. #..##.",
    "#.#### .###.. #ooo#. #..... #ooo#. #..... #..... #...#.",
    "#..... #...#. #ooo#. #...#. #oo#.. #..... #..... #...#.",
    ".##### #...#. ####.. .###.. ###... #####. #..... .####.",
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* 'H'    'I'    'J'    'K'    'L'    'M'    'N'    'O' */
    "...... ...... ...... ...... ...... ...... ...... ......",
    "#...#. .###.. ..###. #...#. #..... #...#. ##..#. .###..",
    "#...#. ..#... ....#. #..#.. #..... #...#. ##..#. #ooo#.",
    "#...#. ..#... ....#. #.#... #..... ##.##. #.#.#. #ooo#.",
    "#####. ..#... ....#. ##.... #..... ##.##. #.#.#. #ooo#.",
    "#...#. ..#... ....#. #.#... #..... #.#.#. #.#.#. #ooo#.",
    "#...#. ..#... ....#. #..#.. #..... #.#.#. #..##. #ooo#.",
    "#...#. .###.. ####.. #...#. #####. #...#. #..##. .###..",
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* 'P'    'Q'    'R'    'S'    'T'    'U'    'V'    'W' */
    "...... ...... ...... ...... ...... ...... ...... ......",
    "####.. .###.. ####.. .###.. #####. #...#. #...#. #...#.",
    "#ooo#. #ooo#. #ooo#. #...#. ..#... #...#. #...#. #.#.#.",
    "#ooo#. #ooo#. #ooo#. #..... ..#... #...#. #...#. #.#.#.",
    "####.. #ooo#. ####.. .###.. ..#... #...#. .#.#.. #.#.#.",
    "#..... #ooo#. #...#. ....#. ..#... #...#. .#.#.. .#.#..",
    "#..... #oo##. #...#. #...#. ..#... #...#. ..#... .#.#..",
    "#..... .####. #...#. .###.. ..#... .###.. ..#... .#.#..",
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* 'X'    'Y'    'Z'    '['    '\\'   ']'    '^'    '_' */
    "...... ...... ...... .###.. #..... .###.. ...... ......",
    "#...#. #...#. #####. .#.... #..... ...#.. ..#... ......",
    "#...#. #...#. ....#. .#.... .#.... ...#.. .#.#.. ......",
    ".#.#.. #...#. ...#.. .#.... .#.... ...#.. #...#. ......",
    "..#... .###.. ..#... .#.... ..#... ...#.. ...... ......",
    ".#.#.. ..#... .#.... .#.... ..#... ...#.. ...... ......",
    "#...#. ..#... #..... .#.... ...#.. ...#.. ...... ......",
    "#...#. ..#... #####. .#.... ...#.. ...#.. ...... ......",
    "...... ...... ...... .###.. ....#. .###.. ...... ######",
    "...... ...... ...... ...... ...... ...... ...... ......",
    /* '`'    'a'    'b'    'c'    'd'    'e'    'f'    'g' */
    "...... ...... #..... ...... ....#. ...... ..###. ......",
    "..#... ...... #..... ...... ....#. ...... .#.... ......",
    "...#.. ...... #..... ...... ....#. ...... .#.... ......",
    "...... .###.. ####.. .####. .####. .###.. ####.. .###..",
    "...... ....#. #ooo#. #..... #ooo#. #ooo#. .#.... #ooo#.",
    "...... .####. #ooo#. #..... #ooo#. #####. .#.... #ooo#.",
    "...... #ooo#. #ooo#. #..... #ooo#. #..... .#.... #ooo#.",
    "...... .####. ####.. .####. .####. .####. .#.... .####.",
    "...... ...... ...... ...... ...... ...... ...... ....#.",
    "...... ...... ...... ...... ...... ...... ...... .###..",
    /* 'h'    'i'    'j'    'k'    'l'    'm'    'n'    'o' */
    "#..... ...... ...... .#.... .##... ...... ...... ......",
    "#..... ..#... ...#.. .#.... ..#... ...... ...... ......",
    "#..... ...... ...... .#.... ..#... ...... ...... ......",
    "####.. .##... ..##.. .#..#. ..#... ##.#.. ####.. .###..",
    "#...#. ..#... ...#.. .#.#.. ..#... #.#.#. #...#. #ooo#.",
    "#...#. ..#... ...#.. .##... ..#... #.#.#. #...#. #ooo#.",
    "#...#. ..#... ...#.. .#.#.. ..#... #.#.#. #...#. #ooo#.",
    "#...#. ..#... ...#.. .#..#. ..#... #.#.#. #...#. .###..",
    "...... ...... ...#.. ...... ...... ...... ...... ......",
    "...... ...... ###... ...... ...... ...... ...... ......",
    /* 'p'    'q'    'r'    's'    't'    'u'    'v'    'w' */
    "...... ...... ...... ...... ...... ...... ...... ......",
    "...... ...... ...... ...... .#.... ...... ...... ......",
    "...... ...... ...... ...... .#.... ...... ...... ......",
    "####.. .####. #.##.. .####. .###.. #...#. #...#. #...#.",
    "#ooo#. #ooo#. ##..#. #..... .#.... #...#. #...#. #.#.#.",
    "#ooo#. #ooo#. #..... .###.. .#.... #...#. .#.#.. #.#.#.",
    "#ooo#. #ooo#. #..... ....#. .#.... #...#. .#.#.. .#.#..",
    "####.. .####. #..... ####.. ..###. .####. ..#... .#.#..",
    "#..... ....#. ...... ...... ...... ...... ...... ......",
    "#..... ....#. ...... ...... ...... ...... ...... ......",
    /* 'x'    'y'    'z'    '{'    '|'    '}'    '~'    '\x7f' */
    "...... ...... ...... ...#.. ..#... .#.... ...... ......",
    "...... ...... ...... ..#... ..#... ..#... ...... ......",
    "...... ...... ...... ..#... ..#... ..#... ...... ......",
    "#...#. #...#. #####. ..#... ..#... ..#... ...... ......",
    ".#.#.. #...#. ...#.. .#.... ..#... ...#.. .##..# ......",
    "..#... #...#. ..#... ..#... ..#... ..#... #..##. ......",
    ".#.#.. #...#. .#.... ..#... ..#... ..#... ...... ......",
    "#...#. .####. #####. ..#... ..#... ..#... ...... ......",
    "...... ....#. ...... ...#.. ..#... .#.... ...... ......",
    "...... .###.. ...... ...... ...... ...... ...... ......",
};

static char text[1 << 21];
static int text_length;

/*
 * The pixels of the image followed directly by the compressed data of the
 * current frame.  Each pixel holds 0 (background), 1 (black) or 2 (white).
 */
static unsigned char canvas[1 << 22];
static int width, height = 11;

/* LZW string table: entry (prefix + 1) * 256 + pixel holds the code of
 * the string "prefix followed by pixel", or -1 if there is none yet.  Row 0
 * holds the single pixel strings. */
static int dictionary[1 << 21];
static int last_code;

static unsigned char *output;   /* byte of the compressed data being filled */
static int bit_position;        /* next free bit in that byte */

void put_byte(int n)
{
    putchar(n);
}

/* 16 bit little endian number */
void put_word(int n)
{
    put_byte(n % 256);
    put_byte(n / 256);
}

/* Value 0, 1 or 2 of one cell of the glyph for character ch. */
int glyph_cell(int ch, int row, int column)
{
    int glyph = ch - ' ';
    const char *line = font[glyph / GLYPHS_PER_ROW * GLYPH_ROWS + row];
    char cell = line[glyph % GLYPHS_PER_ROW * (GLYPH_COLUMNS + 1) + column];

    return cell == '#' ? 1 : cell == 'o' ? 2 : 0;
}

void draw_char(int ch, int column, int top)
{
    int row, col, dy, dx;

    for (row = 0; row < GLYPH_ROWS; row++)
        for (col = 0; col < GLYPH_COLUMNS; col++)
            for (dy = 0; dy < CELL_SIZE; dy++)
                for (dx = 0; dx < CELL_SIZE; dx++) {
                    int y = top + MARGIN + row * CELL_SIZE + dy;
                    int x = MARGIN + column * CHAR_WIDTH + col * CELL_SIZE + dx;

                    canvas[y * width + x] = glyph_cell(ch, row, col);
                }
}

/*
 * Write an LZW code with as many bits as the last assigned code has,
 * least significant bit first.
 */
void write_code(int code)
{
    int bits;

    for (bits = last_code; bits; bits /= 2) {
        if (bit_position == 0)
            *output = 0;
        *output |= (code % 2) << bit_position;
        if (bit_position == 7) {
            output++;
            bit_position = 0;
        } else {
            bit_position++;
        }
        code /= 2;
    }
}

/* Color of pixel i when the background has the gray level background. */
int pixel_color(int i, int background)
{
    /*
     * The original program stores the end of input marker (-1) in the first
     * pixel.  Its lookup then reads memory just before the string table,
     * which is zero, so the first pixel is always encoded as color 0.
     */
    if (i == 0)
        return 0;
    if (canvas[i])
        return (canvas[i] - 1) * (GRAY_LEVELS - 1);
    return background;
}

void write_frame(int frame)
{
    int background = frame > 31 ? frame - 31 : 31 - frame;
    int pixels = width * height;
    unsigned char *data, *block;
    int *row, code = 0, color, i, size, remaining;

    /* graphic control extension: keep previous frame, delay in 1/100 s */
    last_code = END_CODE;
    put_word(0xF921);
    put_byte(4);
    put_byte(4);
    put_word(frame % 32 ? 11 : 511);
    put_byte(0);                /* transparent color index (unused) */
    put_byte(0);                /* block terminator */

    /* image descriptor covering the whole screen */
    put_byte(',');
    put_word(0);
    put_word(0);
    put_word(width);
    put_word(height);
    put_byte(0);                /* no local color table */
    put_byte(8);                /* LZW minimum code size */

    bit_position = 0;
    data = output = canvas + pixels;

    for (i = 0; i < 1 << 21; i++)
        dictionary[i] = i < 256 ? i : -1;

    write_code(CLEAR_CODE);
    row = dictionary;
    for (i = 0; i < pixels; i++) {
        color = pixel_color(i, background);
        if (row[color] < 0) {
            /* string not known yet: emit its prefix and remember it */
            write_code(code);
            if (last_code < MAX_CODE)
                row[color] = ++last_code;
            code = color;
        } else {
            code = row[color];
        }
        row = dictionary + (code + 1) * 256;
    }
    write_code(code);
    write_code(END_CODE);

    /* the data is written in sub-blocks of at most 255 bytes */
    output++;
    block = data;
    remaining = output - data;
    do {
        size = remaining > 255 ? 255 : remaining;
        put_byte(size);
        for (i = 0; i < size; i++)
            put_byte(*block++);
        remaining -= size;
    } while (size);
}

int main()
{
    int ch, i, line_width = CHAR_WIDTH, column = 0, top = 0;

    /* read the text, keeping only printable ASCII and newlines */
    while ((ch = getchar()) != EOF && ch < 128) {
        if (ch == '\n') {
            height += LINE_HEIGHT;
            if (width < line_width)
                width = line_width;
            line_width = CHAR_WIDTH;
        } else {
            line_width += CHAR_WIDTH;
        }
        if (ch == '\n' || (ch >= ' ' && ch < 127))
            text[text_length++] = ch;
    }

    for (i = 0; i < text_length; i++) {
        if (text[i] == '\n') {
            column = 0;
            top += LINE_HEIGHT;
        } else {
            draw_char(text[i], column, top);
            column++;
        }
    }

    /* header and logical screen descriptor */
    printf("GIF89a");
    put_word(width);
    put_word(height);
    put_byte(0xF4);             /* global color table with 32 entries */
    put_byte(0);                /* background color index */
    put_byte(0);                /* pixel aspect ratio */

    /* gray scale palette from black to white */
    for (i = 0; i < GRAY_LEVELS * 3; i++)
        put_byte(i / 3 * 255 / (GRAY_LEVELS - 1));

    /* application extension: loop the animation forever */
    put_byte('!');
    put_byte(0xFF);
    put_byte(11);
    printf("NETSCAPE2.0");
    put_byte(3);
    put_byte(1);
    put_word(0);                /* loop count 0 = forever */
    put_byte(0);

    for (i = 0; i < FRAME_COUNT; i++)
        write_frame(i);

    put_byte(';');
    return 0;
}
