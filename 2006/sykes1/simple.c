/*
 * Bedlam Cube solver.
 *
 * Fits the 13 Bedlam Cube pieces into a 4x4x4 cube, prints the chosen
 * solution as 3D ASCII art (one picture per building step) and writes an
 * animated, uncompressed GIF "cube.gif" showing the same steps.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_PIECES 13
#define MAX_PLACEMENTS 432
#define LAST_DEPTH (NUM_PIECES - 1)
#define FULL_CUBE (~0ULL)

/*
 * A 4x4x4 cube is stored in 64 bits: bit = layer * 16 + row * 4 + column.
 * Layer 0 is the bottom, row 0 is the back, column 0 is the left side.
 */
#define CELL(layer, row, col) ((layer) * 16 + (row) * 4 + (col))

/* bits of column 3 in every row, used to detect shapes wrapping into the next row */
#define COLUMN3_MASK 0x888888888888888ULL
/* bits of row 3 in layers 0..2, used to detect shapes wrapping into the next layer */
#define ROW3_MASK 0xF000F000F000ULL

/*
 * Each piece is a 3x3x3 bitmap: bit = layer * 9 + row * 3 + column.
 * For example 186 = 010 111 010 is the cross shaped piece.
 */
int pieces[] = {186, 94, 1426, 3098, 1047, 122, 1082, 3083, 1039,
                569, 527, 1054, 531};

/* placements[depth][piece][n]: the n-th position still possible for piece */
unsigned long long placements[NUM_PIECES][NUM_PIECES][MAX_PLACEMENTS];
/* placement_count[piece][depth]: number of entries in placements[depth][piece] */
int placement_count[NUM_PIECES][NUM_PIECES];

int depth;                         /* number of pieces placed so far */
int next_piece;                    /* piece with the fewest possible placements */
int used_pieces;                   /* bit mask of the pieces already placed */
unsigned long long occupied;       /* cells of the cube that are already filled */
unsigned long long placed[NUM_PIECES]; /* where each piece has been placed */
int solutions_to_skip;             /* count down to the wanted solution */

/* ------------------------------------------------------------------ */
/* Piece orientations                                                 */
/* ------------------------------------------------------------------ */

/* rotate a 3x3x3 piece a quarter turn around the vertical axis */
static int rotate_vertical(int shape)
{
    int rotated = 0, bit;

    for (bit = 0; bit < 27; bit++) {
        int layer = bit / 9, row = bit / 3 % 3, col = bit % 3;
        rotated |= (shape >> bit & 1) << (layer * 9 + col * 3 + (2 - row));
    }
    return rotated;
}

/* rotate a 3x3x3 piece a quarter turn around the left-right axis */
static int rotate_horizontal(int shape)
{
    int rotated = 0, bit;

    for (bit = 0; bit < 27; bit++) {
        int layer = bit / 9, row = bit / 3 % 3, col = bit % 3;
        rotated |= (shape >> bit & 1) << (row * 9 + (2 - layer) * 3 + col);
    }
    return rotated;
}

/* swap the top and bottom layer, which turns a piece into its mirror image */
static int mirror(int shape)
{
    int diff = (shape >> 18) ^ (shape & 511);

    return shape ^ (diff << 18 | diff);
}

/* record every position where one orientation of a piece fits in the cube */
static void add_positions(int piece, int shape)
{
    unsigned long long cube = 0;
    int shift, line;

    /* copy the nine 3-bit lines of the piece into the 4x4x4 layout */
    for (line = 0; line < 9; line++)
        cube |= ((unsigned long long)shape >> line * 3 & 7)
                << CELL(line / 3, line % 3, 0);

    for (shift = -37; shift < 59; shift++) {
        unsigned long long moved = shift < 0 ? cube >> -shift : cube << shift;
        unsigned long long back = shift < 0 ? moved << -shift : moved >> shift;
        int *count = &placement_count[piece][0];
        int n;

        if (back != cube)                         /* fell off the cube */
            continue;
        if (moved & moved >> 1 & COLUMN3_MASK)    /* wrapped to the next row */
            continue;
        if (moved & moved >> 4 & ROW3_MASK)       /* wrapped to the next layer */
            continue;

        for (n = 0; n < *count; n++)
            if (placements[0][piece][n] == moved)
                break;
        if (n == *count)                          /* not a duplicate */
            placements[0][piece][(*count)++] = moved;
    }
}

/* list every placement of every piece in the empty cube */
static void generate_placements(int mirrored)
{
    int piece, turn, spin;

    for (piece = 0; piece < NUM_PIECES; piece++) {
        int shape = pieces[piece];

        if (mirrored)
            shape = mirror(shape);

        /* visit all 24 orientations (some more than once) */
        for (turn = 0; turn < 7; turn++) {
            if (turn == 4)
                shape = rotate_vertical(shape);
            shape = rotate_horizontal(shape);
            for (spin = 0; spin < 4; spin++) {
                shape = rotate_vertical(shape);
                add_positions(piece, shape);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Output                                                             */
/* ------------------------------------------------------------------ */

#define SCREEN_ROWS 21
#define SCREEN_COLS 59
#define PANEL_TEXT_WIDTH 30

#define GIF_WIDTH 586
#define GIF_HEIGHT 288
#define GIF_BLOCKS 1340          /* data sub-blocks per frame */
#define GIF_BLOCK_PIXELS 126     /* pixels per sub-block */
#define PANEL_GIF_WIDTH 298

char screen[SCREEN_ROWS][SCREEN_COLS + 1];
char frames[NUM_PIECES][GIF_BLOCKS * GIF_BLOCK_PIXELS];

/* 3D ASCII picture of one small cube */
static const char *const cube_template[6] = {
    "+----+",
    "/    /|",
    "+----+ |",
    "|    | +",
    "|    |/",
    "+----+",
};
static const int template_indent[6] = {2, 1, 0, 0, 0, 0};

/*
 * Draw the cell at (layer,row,col) of a piece into the text screen.
 * owner[] holds the piece order number of each cell (-1 for empty), and
 * is_new tells whether this is the piece added in this building step;
 * that piece is drawn shaded, the older ones only as outlines.
 * Edges between cells of the same piece are removed.
 */
static void draw_text_cell(const int *owner, int cell, int panel, int is_new)
{
    int layer = cell / 16, row = cell / 4 % 4, col = cell % 4;
    int me = owner[cell];
    char glyph[6][9];
    int line;

    for (line = 0; line < 6; line++)
        strcpy(glyph[line], cube_template[line]);

    /* same old piece to the left: remove the left edge */
    if (!is_new && col > 0 && owner[cell - 1] == me)
        glyph[1][0] = glyph[3][0] = glyph[4][0] = ' ';

    /* same piece behind: remove the back edge of the top face */
    if (row > 0 && owner[cell - 4] == me) {
        memcpy(glyph[0], is_new ? "//////" : "+    +", 6);
        if (col == 3 || owner[cell - 4 + 1] < 0)
            glyph[1][6] = glyph[2][7] = is_new ? '/' : ' ';
    }

    /* same piece below: remove the bottom edge of the front face */
    if (layer > 0 && owner[cell - 16] == me) {
        if (row == 3 || owner[cell - 16 + 4] < 0)
            memcpy(glyph[5], is_new ? "||||||" : "+    +", 6);
        if (!is_new)
            if (col == 3 || owner[cell - 16 + 1] < 0)
                glyph[4][6] = ' ';
    }

    /* the newest piece is filled with shading */
    if (is_new) {
        memcpy(glyph[1] + 1, "////", 4);
        glyph[2][6] = glyph[3][6] = '/';
        memcpy(glyph[3] + 1, "||||", 4);
        memcpy(glyph[4] + 1, "||||", 4);
    }

    for (line = 0; line < 6; line++)
        memcpy(screen[9 - layer * 3 + row * 2 + line]
                   + col * 5 - row * 2 + panel * PANEL_TEXT_WIDTH
                   + 6 + template_indent[line],
               glyph[line], strlen(glyph[line]));
}

/* draw one small cube into a GIF frame with top, front and right faces */
static void draw_gif_cell(char *frame, int cell, int panel, int order)
{
    int layer = cell / 16, row = cell / 4 % 4, col = cell % 4;
    int x = 88 + panel * PANEL_GIF_WIDTH + col * 48 - row * 20;
    int y = 152 + row * 20 - layer * 48;
    int top = 17 + order;                /* palette group 1 */
    int side = 33 + order;               /* palette group 2 */
    int front = 49 + order + row * 16;   /* groups 3..6, brighter toward the front */
    int i;

    for (i = 0; i < 48; i++) {
        memset(frame + (y + i) * GIF_WIDTH + x + 28, side, 20);
        memset(frame + (y + i + 20) * GIF_WIDTH + x - 20, front, 48);
    }
    for (i = 0; i < 20; i++) {
        memset(frame + (y + i) * GIF_WIDTH + x - i, top, 48);
        memset(frame + (y + i) * GIF_WIDTH + x + 48 - i, side, i);
        memset(frame + (y + i + 48) * GIF_WIDTH + x + 28, side, 20 - i);
    }
}

/* base colours of the palette: background then one colour per piece */
static const unsigned char base_colors[16][3] = {
    {44, 45, 46},    {138, 48, 49},  {50, 78, 52},   {53, 54, 142},
    {122, 84, 58},   {59, 125, 126}, {89, 63, 129},  {130, 131, 132},
    {161, 134, 70},  {87, 162, 89},  {90, 133, 166}, {167, 171, 79},
    {171, 81, 172},  {99, 174, 178}, {86, 87, 88},   {89, 90, 91},
};
/* brightness added for each of the 8 groups of 16 palette entries */
static const int group_brightness[8] = {0, 40, 0, 58, 66, 74, 77, 0};

static void put_word(int value, FILE *out)
{
    fputc(value & 255, out);
    fputc(value >> 8, out);
}

/* write all frames as an uncompressed, looping animated GIF */
static void write_gif(void)
{
    FILE *gif = fopen("cube.gif", "wb");
    int frame, group, entry, rgb, block;

    /* header and logical screen descriptor: 128 colour global table */
    fputs("GIF89a", gif);
    put_word(GIF_WIDTH, gif);
    put_word(GIF_HEIGHT, gif);
    fputc(0xF6, gif);
    fputc(0, gif);
    fputc(0, gif);

    for (group = 0; group < 8; group++)
        for (entry = 0; entry < 16; entry++)
            for (rgb = 0; rgb < 3; rgb++)
                fputc(base_colors[entry][rgb] + group_brightness[group], gif);

    /* NETSCAPE2.0 application extension: loop forever */
    fputc(0x21, gif);
    fputc(0xFF, gif);
    fputc(11, gif);
    fputs("NETSCAPE2.0", gif);
    fputc(3, gif);
    fputc(1, gif);
    put_word(0, gif);
    fputc(0, gif);

    for (frame = 0; frame < NUM_PIECES; frame++) {
        /* graphic control extension: 2.46 s delay, colour 0 transparent */
        fputc(0x21, gif);
        fputc(0xF9, gif);
        fputc(4, gif);
        fputc(9, gif);
        put_word(246, gif);
        fputc(0, gif);
        fputc(0, gif);

        /* image descriptor */
        fputc(0x2C, gif);
        put_word(0, gif);
        put_word(0, gif);
        put_word(GIF_WIDTH, gif);
        put_word(GIF_HEIGHT, gif);
        fputc(0, gif);

        /*
         * "Uncompressed" LZW with 8 bit codes: every sub-block starts with
         * a clear code so the code size never grows, then 126 raw pixels.
         */
        fputc(7, gif);
        for (block = 0; block < GIF_BLOCKS; block++) {
            fputc(GIF_BLOCK_PIXELS + 1, gif);
            fputc(0x80, gif);  /* clear code */
            fwrite(frames[frame] + block * GIF_BLOCK_PIXELS, 1,
                   GIF_BLOCK_PIXELS, gif);
        }
        fputc(1, gif);
        fputc(0x81, gif);      /* end of information code */
        fputc(0, gif);
    }
    fputc(';', gif);           /* trailer */
    fclose(gif);
}

/* print the building steps of the solution and write the GIF */
static void show_solution(void)
{
    int total[64];             /* cells of all pieces placed so far */
    int current[64];           /* cells of the piece added in this step */
    unsigned long long previous = 0, piece_cells;
    int order, j, cell, panel;

    for (cell = 0; cell < 64; cell++)
        total[cell] = -1;

    /* add the pieces in increasing order of their bitmaps (bottom first) */
    for (order = 0; order < NUM_PIECES; order++) {
        piece_cells = FULL_CUBE;
        for (j = 0; j < NUM_PIECES; j++)
            if (placed[j] < piece_cells && placed[j] > previous)
                piece_cells = placed[j];

        for (cell = 0; cell < 64; cell++)
            current[cell] = -1;
        for (j = 0; j < SCREEN_ROWS; j++)
            memset(screen[j], ' ', SCREEN_COLS);

        /* draw back to front, bottom to top so nearer cubes hide farther ones */
        for (cell = 0; cell < 64; cell++) {
            if (piece_cells >> cell & 1)
                total[cell] = current[cell] = order;
            for (panel = 0; panel < 2; panel++) {
                const int *owner = panel ? total : current;
                int who = owner[cell];

                if (who != -1) {
                    draw_text_cell(owner, cell, panel, who == order);
                    draw_gif_cell(frames[order], cell, panel, who);
                }
            }
        }
        for (j = 0; j < SCREEN_ROWS; j++)
            puts(screen[j]);
        previous = piece_cells;
    }
    write_gif();
}

/* ------------------------------------------------------------------ */
/* Search                                                             */
/* ------------------------------------------------------------------ */

/*
 * After a piece is placed, work out which placements of the remaining
 * pieces still fit.  A piece with no placement left is a dead end, a piece
 * with only one placement is placed at once.  Returns 1 if searching deeper
 * is worthwhile: every piece still fits somewhere and every empty cell can
 * still be covered by some placement.
 */
static int prune(int level)
{
    for (;;) {
        unsigned long long coverage;
        int fewest = 186;      /* larger than any count that matters */
        int piece, forced = 0;

        depth++;
        coverage = occupied;
        for (piece = 1; piece < NUM_PIECES && !forced; piece++) {
            unsigned long long fit_union = 0;
            int k, fits = 0;

            if (used_pieces >> piece & 1)
                continue;

            for (k = 0; k < placement_count[piece][level]; k++) {
                unsigned long long cells = placements[level][piece][k];
                if (!(occupied & cells))
                    fit_union |= placements[depth][piece][fits++] = cells;
            }

            if (fits == 0)
                return 0;

            if (fits == 1) {
                occupied |= placed[piece] = fit_union;
                used_pieces |= 1 << piece;
                if (depth == LAST_DEPTH) {
                    if (--solutions_to_skip < 1) {
                        show_solution();
                        exit(0);
                    }
                    return 0;
                }
                forced = 1;    /* start over with this piece placed */
                continue;
            }

            if (fits < fewest) {
                next_piece = piece;
                fewest = fits;
            }
            placement_count[piece][depth] = fits;
            coverage |= fit_union;
        }
        if (!forced)
            return coverage == FULL_CUBE;
    }
}

/* try every remaining placement of next_piece, recursing deeper */
static void search(void)
{
    int level = depth;
    unsigned long long saved_occupied = occupied;
    int piece = next_piece;
    int count = placement_count[piece][level];
    int saved_used, i;

    used_pieces |= 1 << piece;
    saved_used = used_pieces;
    for (i = 0; i < count; i++) {
        occupied |= placed[piece] = placements[level][piece][i];
        if (prune(level))
            search();
        used_pieces = saved_used;
        occupied = saved_occupied;
        depth = level;
    }
    used_pieces ^= 1 << piece;
}

int main(int argc, char **argv)
{
    int wanted = 0;

    if (argc > 1)
        wanted = solutions_to_skip = atoi(argv[1]);

    /* a negative number solves the mirror image of the puzzle */
    if (wanted < 0)
        solutions_to_skip = -wanted;
    generate_placements(wanted < 0);

    /* the cross shaped piece 0 is always placed first */
    next_piece = 0;
    search();
    return 0;
}
