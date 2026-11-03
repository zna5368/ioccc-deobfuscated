#include <stdio.h>
#include <time.h>

/*
 * Bemazing: builds a random 3D-looking maze of beams on a triangular grid and
 * writes it as gzip-compressed PostScript.
 *
 * The grid is drawn in isometric view.  Each cell can send a beam in one of
 * three axis directions (0, 1, 2).  Seen from a cell, a beam can therefore
 * leave in one of six directions around a hexagon: 0, 1, 2 going "forward"
 * along an axis and 3, 4, 5 going "backward" (the far end of a beam that
 * started in another cell).
 */

#define ROWS 26
#define COLS 30
#define EDGE_SLOTS 32768 /* candidate beams are stored at random slots */

/* row/column step for each of the three axis directions */
#define ROW_STEP(dir) ((dir) != 0)
#define COL_STEP(dir) ((dir) != 2)

/* bits of a cell value: a beam leaving the cell in hexagon direction d */
#define BEAM(d) (32 >> (d))
#define BEAM_START(dir) (32 >> (dir)) /* beam starts here along axis dir */
#define BEAM_END(dir) (4 >> (dir))    /* beam ends here along axis dir */
#define BEAM_THROUGH(dir) (36 >> (dir)) /* beam passes straight through */

/*
 * The gzip header and the deflate-compressed PostScript prolog (the /A ... /Q
 * drawing procedures).  The deflate block uses a Huffman table chosen so that
 * every following line "X\n" (a single letter command and a newline)
 * compresses to exactly one byte.
 */
static const unsigned char gzip_header[] = {
    0x1f, 0x8b, 0x08, 0x01, 0x49, 0x4f, 0x43, 0x43, 0x43, 0x21, 0x94, 0x93,
    0xcb, 0x6e, 0x83, 0x30, 0x10, 0x45, 0xf7, 0x7c, 0xc5, 0x74, 0xd1, 0x25,
    0xc4, 0x90, 0x12, 0x75, 0x9b, 0xbe, 0xdf, 0x4d, 0xd4, 0x2f, 0x70, 0x8a,
    0x43, 0x50, 0x29, 0x4e, 0x6d, 0x8b, 0xa8, 0xaa, 0xfa, 0xef, 0xb5, 0x21,
    0x21, 0xb6, 0xe2, 0xa1, 0x61, 0x83, 0xc0, 0x33, 0x73, 0xee, 0xdc, 0x19,
    0x73, 0x7a, 0x32, 0x7b, 0x0b, 0xa7, 0x19, 0x5f, 0xb0, 0x30, 0x89, 0x48,
    0x30, 0x9a, 0xc2, 0x0f, 0x84, 0x29, 0x24, 0x31, 0x8c, 0x41, 0x7e, 0x09,
    0x05, 0x59, 0x51, 0xc3, 0x27, 0xaf, 0x99, 0xe2, 0x90, 0x02, 0x01, 0x51,
    0x16, 0x95, 0x79, 0x27, 0x10, 0x13, 0x3b, 0x65, 0x7b, 0xac, 0x4b, 0xe3,
    0xd4, 0x73, 0xbe, 0x2c, 0xca, 0x12, 0x7e, 0x61, 0x51, 0x54, 0x19, 0x64,
    0x6c, 0x19, 0x8c, 0x2e, 0xb4, 0x0e, 0xf9, 0x5f, 0x06, 0xc1, 0x21, 0xea,
    0x07, 0x2a, 0x97, 0xad, 0x1b, 0x97, 0xb1, 0x95, 0xc1, 0x1d, 0x1c, 0x65,
    0xe0, 0xaa, 0x31, 0xe0, 0x22, 0x3a, 0x32, 0x39, 0x86, 0x16, 0x4e, 0xb4,
    0x4f, 0xae, 0xa8, 0x62, 0x3a, 0x5f, 0x32, 0x95, 0x0b, 0xfa, 0xed, 0x48,
    0x5c, 0xf7, 0x74, 0x7f, 0xb0, 0x23, 0x64, 0x60, 0xdd, 0x4c, 0xbd, 0xd9,
    0x52, 0x09, 0xfe, 0xc1, 0x20, 0x97, 0xb4, 0xd6, 0x4d, 0x44, 0xa9, 0x69,
    0xc3, 0x84, 0x36, 0x45, 0xa6, 0x56, 0xb8, 0xbd, 0x5e, 0x96, 0x60, 0x52,
    0x71, 0xc1, 0x1c, 0x27, 0x37, 0xc3, 0xf7, 0xb0, 0xc5, 0xd9, 0x94, 0x5b,
    0x7c, 0xe4, 0xc8, 0x3d, 0xf1, 0x40, 0xee, 0x70, 0x08, 0xb2, 0x2c, 0x0f,
    0xe4, 0x5e, 0x43, 0x86, 0x0e, 0x8d, 0x60, 0x83, 0xb2, 0xee, 0x81, 0xad,
    0xf1, 0x60, 0x66, 0xe6, 0x0f, 0x3d, 0x1a, 0xf9, 0x5d, 0x79, 0xf7, 0xa2,
    0xb7, 0x12, 0xba, 0x9b, 0x51, 0x82, 0x56, 0xb2, 0x34, 0xd5, 0x6d, 0xb3,
    0xed, 0xd3, 0x26, 0x3d, 0x61, 0xa4, 0xa1, 0xa0, 0x67, 0x1b, 0x44, 0xe0,
    0x2c, 0xe9, 0x2b, 0xb7, 0x0b, 0x5f, 0xbc, 0x1d, 0xc8, 0x15, 0xdf, 0xac,
    0x69, 0xee, 0xa6, 0xbe, 0xea, 0xd4, 0xd8, 0xfb, 0xa7, 0xcc, 0xcc, 0x52,
    0xa3, 0x73, 0x6f, 0x6c, 0xde, 0xc4, 0x26, 0xbe, 0x58, 0xbc, 0x5b, 0xdd,
    0x3b, 0x5d, 0xef, 0x3f, 0x9a, 0x3d, 0x06, 0x63, 0x7d, 0x19, 0xc8, 0xbe,
    0xf3, 0xc0, 0x32, 0x1e, 0xfc, 0x15, 0x0c, 0xde, 0x06, 0x00, 0xc3, 0x30,
    0x00, 0xda, 0xf9, 0x2a, 0xbd, 0x93, 0xe4, 0xff, 0x67, 0x68
};

/* PostScript prolog contents (needed for the gzip CRC and length) */
#define PROLOG_CRC 953351751UL
#define PROLOG_LENGTH 1333UL

/* compressed byte for each line "A\n" ... "Q\n" */
static const unsigned char command_code[17] = {
    3, 35, 19, 51, 11, 43, 27, 59, 7, 39, 23, 55, 15, 2, 47, 31, 63};

/* compressed byte holding four "\n" lines */
#define CODE_FOUR_NEWLINES 0
/* compressed byte holding two "\n" lines and the end of the deflate block */
#define CODE_TWO_NEWLINES_END 1

/*
 * crc is the CRC-32 of the uncompressed PostScript text written so far.  While
 * the maze is generated, the same register is used as the random number
 * generator.  length counts the uncompressed bytes.
 */
unsigned long crc, length;

/* candidate beams: column << 7 | length << 13 | row * 4 + direction */
unsigned long edges[EDGE_SLOTS];

/*
 * cell[row][col][0] holds the beam bits of a cell.  When other beams cross
 * over a cell that already has a beam passing through it, they are stored in
 * cell[row][col][1] and cell[row][col][2] (they are drawn as separate layers
 * and are not connected to the beams below them).
 *
 * The entrance is an unfinished beam ending at the corner cell (0, 0).
 */
unsigned long cell[32][48][3] = {{{BEAM_END(1)}}};

/* union-find forest of connected cells: NULL marks the root of a set */
void *parent[32][48];

void **find(void **node) {
    if (*node == NULL)
        return node;
    *node = find(*node);
    return *node;
}

/* feed one byte to the CRC-32 (least significant bit first) */
void crc_byte(unsigned int byte) {
    int bit;

    for (bit = 0; bit < 8; bit++, byte >>= 1) {
        /*
         * CRC-32 step on the bit-inverted register: the result is the
         * finished (inverted) gzip CRC without a final xor.
         */
        if ((crc ^ byte) & 1)
            crc = crc >> 1 ^ 0x80000000UL;
        else
            crc = crc >> 1 ^ 0x6DB88320UL;
    }
    length++;
}

/* step the random number generator */
void next_random(void) { crc_byte('\n'); }

/* write the PostScript line "letter\n" as one compressed byte */
void emit(int letter) {
    crc_byte(letter);
    crc_byte('\n');
    putchar(command_code[letter - 'A']);
}

/* PostScript gray level commands: O = white, P = light, Q = dark */
void shade(int side) { emit("OOPPQQ"[side % 6]); }

int in_grid(unsigned long row, unsigned long col) {
    return row < ROWS && col >= row / 2 && col - row / 2 < COLS;
}

/* only an empty cell or one with a single straight beam can be crossed */
int can_cross(unsigned long bits) {
    return bits == 0 || bits == BEAM_THROUGH(0) || bits == BEAM_THROUGH(1) ||
           bits == BEAM_THROUGH(2);
}

/* fill every slot of edges[] with every possible beam, in random order */
void make_candidates(unsigned long min_length) {
    unsigned long row, col, dir, len, slot;

    for (row = 0; row < ROWS; ++row)
        for (col = row / 2; col < COLS + row / 2; ++col)
            for (dir = 0; dir < 3; ++dir)
                for (len = min_length;
                     in_grid(row + len * ROW_STEP(dir),
                             col + len * COL_STEP(dir));
                     ++len) {
                    while (edges[slot = crc & (EDGE_SLOTS - 1)])
                        next_random();
                    edges[slot] = col << 7 | len << 13 | (row * 4 + dir);
                }
}

/* Kruskal's algorithm: keep a beam when it joins two unconnected parts */
void build_maze(void) {
    unsigned long slot, edge, row, col, dir, len, k;
    unsigned long dr, dc;
    void **from, **to;

    for (slot = 0; slot < EDGE_SLOTS; slot++) {
        edge = edges[slot];
        row = edge / 4 & 31;
        col = edge >> 7 & 63;
        dir = edge % 4;
        len = edge >> 13;
        dr = ROW_STEP(dir);
        dc = COL_STEP(dir);

        from = find(&parent[row][col]);
        to = find(&parent[row + len * dr][col + len * dc]);
        if (from == to || cell[row][col][1] ||
            cell[row + len * dr][col + len * dc][1])
            continue;

        /* every cell between the two ends must be crossable */
        for (k = len - 1; k > 0; k--)
            if (!can_cross(cell[row + k * dr][col + k * dc][0]))
                break;
        if (k > 0)
            continue;

        *to = from;
        cell[row][col][0] |= BEAM_START(dir);
        cell[row + len * dr][col + len * dc][0] |= BEAM_END(dir);
        for (k = len - 1; k > 0; k--) {
            unsigned long *c = cell[row + k * dr][col + k * dc];

            if (c[0] == 0) {
                /* an empty cell becomes part of the beam */
                parent[row + k * dr][col + k * dc] = to;
                c[0] = BEAM_THROUGH(dir);
            } else if (c[1] == 0) {
                c[1] = BEAM_THROUGH(dir);
            } else {
                c[2] = BEAM_THROUGH(dir);
            }
        }
    }
}

/* draw one layer of a cell as six triangles of a hexagon */
void draw_cell(unsigned long bits) {
    int d;

#define HAS(d) (bits & BEAM((d) % 6))

    /* filled faces */
    for (d = 0; d < 6; d++) {
        if (HAS(d)) {
            shade(d + 5);
            emit('A');
            shade(d + 2);
            emit('B');
        }
        if (d & 1)
            shade(d + (HAS(d + 5) ? 1 : 0));
        else
            shade(d + (HAS(d) ? 4 : 1));
        if (HAS(d) || HAS(d + 5))
            emit('C');
        emit('D');
    }

    /* outlines */
    for (d = 0; d < 6; d++) {
        if (!HAS(d) && !HAS(d + 5))
            emit('H');
        if (HAS(d))
            emit('E');
        if ((d & 1) || !HAS(d)) {
            if (HAS(d + 5))
                emit('F');
            if (HAS(d + 1))
                emit('G');
        }
        if (d & 1)
            emit(!HAS(d + 1) != !HAS(d + 5) ? 'J' : 'I');
        else
            emit(HAS(d) ? 'I' : 'J');
    }

#undef HAS
}

void draw_maze(void) {
    unsigned long row, col, layer;

    /* the exit: an unfinished beam leaving the opposite corner */
    cell[ROWS - 1][COLS - 1 + (ROWS - 1) / 2][0] |= BEAM_START(1);

    for (row = 0; row < ROWS; ++row) {
        for (col = row / 2; col < COLS + row / 2; ++col) {
            for (layer = 0; layer < 3; ++layer)
                if (cell[row][col][layer])
                    draw_cell(cell[row][col][layer]);
            emit('M'); /* next cell */
        }
        emit(row & 1 ? 'L' : 'K'); /* next row */
    }
    emit('N'); /* showpage */
}

/* write the gzip trailer: CRC-32 and length, little endian */
void finish(void) {
    unsigned char trailer[8];
    int i, bad;

    crc_byte('\n');
    crc_byte('\n');
    do {
        /* avoid newline and carriage return bytes in the output */
        bad = 0;
        for (i = 0; i < 4; i++) {
            trailer[i] = crc >> (8 * i);
            trailer[4 + i] = length >> (8 * i);
        }
        for (i = 0; i < 8; i++)
            if (trailer[i] == '\n' || trailer[i] == '\r')
                bad = 1;
        if (bad) {
            putchar(CODE_FOUR_NEWLINES);
            for (i = 0; i < 4; i++)
                crc_byte('\n');
        }
    } while (bad);
    putchar(CODE_TWO_NEWLINES_END);

    for (i = 0; i < 8; i++)
        putchar(trailer[i]);
}

int main(int argc, char **argv) {
    unsigned long i;

    /* more arguments make the shortest allowed beam longer */
    crc = time(NULL);
    make_candidates(argc);
    build_maze();

    for (i = 0; i < sizeof gzip_header; i++)
        putchar(gzip_header[i]);

    crc = PROLOG_CRC;
    length = PROLOG_LENGTH;
    draw_maze();
    finish();
    return 0;
}
