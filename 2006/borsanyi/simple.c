#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*
 * Each glyph is a 5x5 grid of pixels, drawn in a 6 pixel wide cell (one
 * column of spacing). The image is 8 pixels high: two blank rows on top,
 * five glyph rows, and one blank row below. There are 2 blank columns on
 * the left and 1 on the right.
 */
#define GLYPH_SIZE 5
#define CELL_WIDTH 6
#define IMAGE_HEIGHT 8
#define MARGIN 2
#define NUM_PIXELS (GLYPH_SIZE * GLYPH_SIZE)

/*
 * The font is stored "inside out": for every pixel of the 5x5 grid there is a
 * list of characters that have that pixel lit. Pixels are numbered row by
 * row. The symbols '@', '.' and '-' are represented by 'K', 'P' and 'M'.
 */
static const char *pixel_chars[NUM_PIXELS] = {
    /* row 0 */
    "bdefhklmnprtuvwxyz57",
    "bcdefgiopqrstz23567890K",
    "abcdefgjopqrstz123567890K",
    "cefghkoqstz23457890K",
    "mntuvwxyz7",
    /* row 1 */
    "bcdefghklmnopqrsuvw256890K",
    "aimnxy1",
    "jkt14",
    "abdhmprxyz0",
    "mnoquvw237890K",
    /* row 2 */
    "abcdefghklmnopqruvw560K",
    "befhikprs45689MK",
    "befghjmnqprstwxyz156890MK",
    "dghs234789M",
    "amnoquvw90K",
    /* row 3 */
    "abcdefghjklmnopqruw4680K",
    "aivxz40PK",
    "ajkrtwy1247PK",
    "abdghnqvx456K",
    "amnosuw34890K",
    /* row 4 */
    "abdefhklmnprxz25_",
    "bcdegijloqsuwz12356890_PK",
    "bcdegloqstuvyz123567890_PK",
    "cehklorsuwz1234890_K",
    "amnqxz2_K",
};

/*
 * GIF header (6 bytes), logical screen descriptor (7 bytes), a two entry
 * global colour table (black, white) and an image descriptor (10 bytes),
 * followed by the LZW minimum code size. The two width fields are filled in
 * once the text length is known.
 */
#define SCREEN_WIDTH_POS 6
#define IMAGE_WIDTH_POS 24
static char gif_header[30] = "GIF87a"
                             /* logical screen descriptor */
                             "\x78\x00"   /* width (placeholder) */
                             "\x08\x00"   /* height = 8 */
                             "\x80"       /* global colour table, 2 colours */
                             "\x01"       /* background colour index */
                             "\x00"       /* pixel aspect ratio */
                             /* global colour table */
                             "\x00\x00\x00" /* colour 0: black */
                             "\xff\xff\xff" /* colour 1: white */
                             /* image descriptor */
                             "\x2c"       /* ',' image separator */
                             "\x00\x00"   /* left */
                             "\x00\x00"   /* top */
                             "\x78\x00"   /* width (placeholder) */
                             "\x08\x00"   /* height = 8 */
                             "\x00"       /* no local colour table */
                             /* image data */
                             "\x03";      /* LZW minimum code size */

/*
 * With an LZW minimum code size of 3 every code is 4 bits wide. Each output
 * byte holds two codes: the low nibble is the CLEAR code (8) and the high
 * nibble is the colour index of one pixel. Clearing before every pixel keeps
 * the code width fixed at 4 bits, so no real compression is needed.
 */
#define LZW_CLEAR 0x08
#define PIXEL_ON 0x10 /* colour 1 in the high nibble */

/* last sub-block holding the END code (9), the block terminator, and ';' */
static const char gif_trailer[4] = "\x01\x09\x00\x3b";

static char image[IMAGE_HEIGHT][256];
static int width;

/* Translate the text in place to the font's alphabet; returns its length. */
static int translate_text(char *text)
{
    int length = 0;

    for (;; text++) {
        if (*text == '@')
            *text = 'K';
        else if (*text == '.')
            *text = 'P';
        else if (*text == '-')
            *text = 'M';
        else
            *text = tolower(*text);
        if (*text == '\0')
            break;
        length++;
    }
    return length;
}

struct pixel_job {
    int pixel;          /* which of the 25 glyph pixels to draw */
    const char *text;
};

/* One thread per glyph pixel: light that pixel for every matching char. */
static void *draw_pixel(void *arg)
{
    struct pixel_job *job = arg;
    int row = MARGIN + job->pixel / GLYPH_SIZE;
    int column = MARGIN + job->pixel % GLYPH_SIZE;
    int position;

    for (position = 0; job->text[position] != '\0'; position++) {
        if (strchr(pixel_chars[job->pixel], job->text[position]))
            image[row][column + position * CELL_WIDTH] = PIXEL_ON;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads[NUM_PIXELS];
    struct pixel_job jobs[NUM_PIXELS];
    int pixel, row, column;
    char *text = argv[1];

    width = translate_text(text) * CELL_WIDTH + MARGIN + 1;

    for (pixel = 0; pixel < NUM_PIXELS; pixel++) {
        jobs[pixel].pixel = pixel;
        jobs[pixel].text = text;
        pthread_create(&threads[pixel], NULL, draw_pixel, &jobs[pixel]);
    }
    for (pixel = 0; pixel < NUM_PIXELS; pixel++)
        pthread_join(threads[pixel], NULL);

    gif_header[SCREEN_WIDTH_POS] = gif_header[IMAGE_WIDTH_POS] = width;
    fwrite(gif_header, 1, sizeof gif_header, stdout);

    /* one data sub-block per image row: length byte, then one byte a pixel */
    for (row = 0; row < IMAGE_HEIGHT; row++) {
        putchar(width);
        for (column = 0; column < width; column++)
            putchar(image[row][column] | LZW_CLEAR);
    }

    return fwrite(gif_trailer, 1, sizeof gif_trailer, stdout) != sizeof gif_trailer;
}
