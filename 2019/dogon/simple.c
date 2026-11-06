#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Conway's Game of Life using Bill Gosper's hashlife algorithm, drawn
 * with Xlib.  X (window width), Y (window height) and Z (number of hash
 * table slots, a power of two) are supplied by the Makefile.
 *
 * Cells of the quadtree ("cells" means both leaves and nodes) live in the
 * array mem[] and are referred to by 32 bit indices into it:
 *
 *   leaf (an 8x8 block of Life cells, level 0), two longs:
 *     mem[c]   = 64 bits of the 8x8 block, bit (row * 8 + col)
 *     mem[c+1] = high 32 bits: result (middle 4x4 after 2^step generations)
 *                low 6 bits:   log2 of the time step of the result
 *
 *   node (level >= 1, covering 8 << level Life cells), three longs:
 *     mem[c]   = top-left quadrant | top-right quadrant << 32
 *     mem[c+1] = high 32 bits: result cell, NODE_FLAG, low 6 bits: step
 *     mem[c+2] = bottom-left quadrant | bottom-right quadrant << 32
 *
 * A quarter of a leaf (4x4 cells) is stored in an unsigned using the low
 * nibble of each of its four bytes, one byte per row.
 *
 * Quadrants are passed around in arrays of 16 unsigneds that are treated as
 * a 4x4 grid (row stride 4).  The four quadrants of one cell are stored at
 * grid positions 0, 1, 4 and 5 (see QUAD_POS).
 */

#define NIBBLE_MASK 0x0F0F0F0Fu          /* low nibble of every byte */
#define NIBBLE_ONES 0x1111111111111111ul /* a 1 in every nibble */

#define STEP_MASK 63  /* low bits of mem[c+1]: log2 of result step */
#define NODE_FLAG 64  /* set in mem[c+1] of nodes, clear for leaves */

#define MAX_LEVEL 64
/* the empty cell of each level is created first, at index 3*level+2 */
#define EMPTY(level) (3 * (level) + 2)
#define FIRST_NON_EMPTY EMPTY(MAX_LEVEL)

/* flags in the mode argument of make_cell() (mode = level * 4 + flags) */
#define MIDDLE    1 /* return the middle quadrant made from the 4 inputs */
#define NODE_ONLY 2 /* return the joined cell itself, not its result */

/* grid position of quadrant i (0..3) inside a 4x4 quadrant grid */
#define QUAD_POS(i) ((i) + ((i) & 2))

/* layout of the single big memory block (offsets counted in unsigneds) */
#define PIXELS_OFFSET  (NIBBLE_MASK / 256) /* gap between hash and pixels */
#define HISTORY_OFFSET (NIBBLE_MASK / 32)  /* pixels to history buffer */
#define CELLS_OFFSET   99999               /* history buffer to mem[] */
#define STRIDE         (X + 128)           /* pixels per image row */

#define WHITE 0xFFFFFFFFu
#define BORDER_COLOR 99           /* leaf border when zoomed in */
#define BACKGROUND_BITS (513 << 12) /* 0x201000: dim red and dim green */

/* run_mode bits */
#define RUNNING     1
#define SINGLE_STEP 2

long view_x, view_y;      /* centre of the view in Life coordinates */
long scale;               /* log2 of Life cells per pixel, plus 4 */
long step_log;            /* log2 of the generations per step */
long run_mode;
long generation;
unsigned next_free = 2;   /* index of the next free slot in mem[] */
unsigned root;            /* the cell holding the whole universe */
long level;               /* level of root */
unsigned backward;        /* arrow of time is reversed */

unsigned long *mem;       /* leaves and nodes */
unsigned *hash_table;     /* Z slots holding indices into mem[] */
unsigned *pixels;         /* X by Y image with STRIDE pixels per row */
unsigned *history;        /* stack of (cell, level, generation) */

/* Split a cell into its four quadrants, stored at quad[0,1,4,5]. */
void split(unsigned cell, unsigned *quad)
{
    for (int i = 0; i < 4; i++) {
        int k = QUAD_POS(i);
        if (mem[cell + 1] & NODE_FLAG)
            quad[k] = mem[cell + (i & 2)] >> i % 2 * 32;
        else
            quad[k] = mem[cell] >> (i % 2 * 4 + 16 * (i & 2)) & NIBBLE_MASK;
    }
}

/*
 * Run Life on an 8x8 leaf for 2 generations (1 if step_log is 0), all 64
 * cells at once, and return the middle 4x4 as a quarter leaf shifted into
 * the high 32 bits.
 */
unsigned long life_leaf(unsigned long bits)
{
    for (int gen = 0; gen < 2 - !step_log; gen++) {
        unsigned long next = 0;
        /* Each pass handles the columns bit, bit+4 of every row, keeping
         * one 4 bit counter per cell in the 16 nibbles of a long. */
        for (int bit = 0; bit < 4; bit++) {
            unsigned long sum = bits >> bit & NIBBLE_ONES;  /* the cell */
            sum += bits * 2 >> bit & NIBBLE_ONES;           /* left */
            sum += bits / 2 >> bit & NIBBLE_ONES;           /* right */
            sum += (sum << 8) + (sum >> 8);                 /* rows above/below */
            /* sum is now the 3x3 count including the cell itself.  The
             * cell lives if sum is 3, or if sum is 4 and it was alive:
             * ((sum + 1) & 6) == 4 only for 3 and 4, and the xor/add
             * turns that into bit 3 of the nibble. */
            unsigned long three_or_four =
                ((((sum + NIBBLE_ONES) & NIBBLE_ONES * 6) ^ 3 * NIBBLE_ONES)
                 + NIBBLE_ONES) >> 3;
            next |= (three_or_four & (bits >> bit | sum) & NIBBLE_ONES) << bit;
        }
        bits = next;
    }
    return (bits >> 18 & NIBBLE_MASK) << 32;
}

/*
 * Find (or create) the cell at the given level made from the four
 * quadrants quad[0,1,4,5] and return the cell or its result (the middle
 * half advanced in time).  With MIDDLE the middle quadrant formed by the
 * four quadrants is returned instead.  This is the hashlife recursion.
 */
unsigned long make_cell(unsigned *quad, unsigned long mode)
{
    long cell_level = mode / 4;
    unsigned long upper, lower, slot = 0, result;
    unsigned sub[16];   /* 4x4 grid of sub quadrants */
    unsigned cell;

    if (cell_level > 0) {
        for (int i = 0; i < 4; i++) {
            int k = QUAD_POS(i);
            split(quad[k], sub + 2 * k);
            slot = 917 * slot + quad[k];
        }
        if (mode & MIDDLE)
            return make_cell(sub + 5, (cell_level - 1) * 4 + NODE_ONLY);
        upper = quad[0] | (unsigned long)quad[1] << 32;
        lower = quad[4] | (unsigned long)quad[5] << 32;
    } else {
        /* join four quarter leaves: top half rows 0-3, bottom rows 4-7 */
        upper = quad[0] | quad[1] << 4;
        lower = quad[4] | quad[5] << 4;
        slot = upper + 719 * lower;
        if (mode & MIDDLE)  /* rows 2-5, columns 2-5 */
            return (upper >> 18 & 0xF0F) | (lower & 0x3C3C) << 14;
        upper |= lower << 32;
    }

    /* linear probing hash lookup (leaves only compare their 64 bits) */
    for (; (cell = hash_table[slot &= Z - 1]) != 0; slot++)
        if (upper == mem[cell] && (cell_level == 0 || mem[cell + 2] == lower))
            break;

    /* largest step the result can have (smaller if step_log is small) */
    int step = cell_level + 1;
    if (step > step_log)
        step = step_log;

    if (!cell || (mem[cell + 1] & STEP_MASK) != step) {
        if (!cell) {
            hash_table[slot] = cell = next_free;
            mem[cell] = upper;
            next_free += 2;
            mem[next_free] = lower;    /* junk for a leaf, overwritten later */
            next_free += cell_level > 0;
        }
        if (cell_level == 0) {
            result = life_leaf(upper);
        } else {
            int child = cell_level - 1;
            /* 9 overlapping sub cells advanced half way in time */
            for (int i = 0; i < 11; i++)
                if ((i + 1) & 3)
                    sub[i] = make_cell(sub + i, child * 4);
            /* the 4 quadrants of the result: advanced again, or only
             * centred if the step must be smaller than the maximum */
            for (int i = 0; i < 4; i++) {
                int k = QUAD_POS(i);
                sub[k] = make_cell(sub + k, child * 4 + (child > step_log - 2));
            }
            result = NODE_FLAG | make_cell(sub, child * 4 + NODE_ONLY) << 32;
        }
        mem[cell + 1] = step | result;
    }
    return mode & NODE_ONLY ? cell : mem[cell + 1] >> 32;
}

/*
 * Draw a cell whose top left corner is (x, y) in Life coordinates.
 * path holds 3 bits per level (x + 2*y quadrant choice) and is used to
 * make the checkered background.
 */
void draw(unsigned cell, int cell_level, long x, long y, unsigned long path)
{
    long half = 4l << cell_level;          /* size of a quadrant */
    long view_h = Y >> 5 << scale;         /* half the view height */
    long view_w = X >> 5 << scale;         /* half the view width */

    if ((cell < FIRST_NON_EMPTY && scale > 6)
        || x >= view_x + view_w || x + 2 * half <= view_x - view_w
        || y >= view_y + view_h || y + 2 * half <= view_y - view_h)
        return;

    unsigned *pixel = pixels + (y - view_y + view_h << 4 >> scale) * STRIDE
                             + (x - view_x + view_w << 4 >> scale);

    if (cell_level <= scale - 7) {   /* smaller than one pixel */
        *pixel = WHITE;
        return;
    }
    if (cell_level > 0) {
        unsigned quad[6];
        split(cell, quad);
        for (int i = 0; i < 4; i++)
            draw(quad[QUAD_POS(i)], cell_level - 1, x + i % 2 * half,
                 y + i / 2 * half, path * 8 + i);
    } else {
        long shrink = 0;          /* log2 of Life cells per pixel */
        long grow = 4 - scale;    /* log2 of pixels per Life cell */
        if (grow < 0) {
            shrink = -grow;
            grow = 0;
        }
        path ^= path >> 1;
        unsigned background = path & BACKGROUND_BITS;
        for (long row = 0; row < 8 << grow; row++)
            for (long col = 0; col < 8 << grow; col++) {
                unsigned color = BORDER_COLOR;
                if ((row != 0 && col != 0) || grow < 2)
                    color = mem[cell] >> ((row >> grow) * 8 + (col >> grow)) & 1
                          ? WHITE : background;
                pixel[(row >> shrink) * STRIDE + (col >> shrink)] |= color;
            }
    }
}

/*
 * Advance the universe by 2^step_log generations.  Returns 1 if the new
 * state was pushed on the history stack and should be printed.
 */
int advance(void)
{
    unsigned q[16];
    long target = level + 2;
    int growing;

    generation += 1l << step_log;

    /* surround root with empty space until it is big enough */
    do {
        for (int i = 0; i < 16; i++)
            q[i] = EMPTY(level - 1);
        level++;
        growing = level < target || level < step_log;
        split(root, q + 5);
        for (int i = 0; i < 4; i++) {
            int k = QUAD_POS(i);
            q[k] = make_cell(q + 2 * k, (level - 1) * 4 + NODE_ONLY);
        }
        /* the last one returns the result: the step in time */
        root = make_cell(q, level * 4 + (growing ? NODE_ONLY : 0));
    } while (growing);

    /* shrink while the outer ring of sub quadrants is empty */
    while (--level != 1) {
        split(root, q + 10);
        for (int i = 0; i < 4; i++) {
            int k = QUAD_POS(i);
            split(q[10 + k], q + 2 * k);
        }
        for (int i = 0; i < 16; i++) {
            int border = i / 4 == 0 || i / 4 == 3 || i % 4 == 0 || i % 4 == 3;
            if (border && q[i] != EMPTY(level - 2)) {
                *++history = root;
                *++history = level;
                *++history = generation;
                return 1;
            }
        }
        root = make_cell(q + 5, (level - 1) * 4 + NODE_ONLY);
    }
    return 0;
}

int main()
{
    hash_table = calloc(Z, 8);
    pixels = hash_table + Z + PIXELS_OFFSET;
    char *line = (char *)pixels - 99;
    /* the cells read from the input are numbered from 1 in the pixels */
    unsigned *input_cells = pixels;
    unsigned *history_start;
    history = pixels + HISTORY_OFFSET;
    mem = (unsigned long *)(history + CELLS_OFFSET);

    unsigned q[16];
    long line_count = 0;
    unsigned num_cells = 0;

    /* create the empty cell of every level */
    for (int lv = 0; lv < MAX_LEVEL; lv++) {
        for (int i = 0; i < 4; i++)
            q[QUAD_POS(i)] = root;
        level = lv;
        root = make_cell(q, level * 4 + NODE_ONLY);
        next_free += !lv;
    }

    Display *display = XOpenDisplay(0);

    /* read the Golly macrocell file: skip the header and # lines */
    while (gets(line)) {
        if (line_count++ && line[0] != '#') {
            int lv;
            if (sscanf(line, "%d%d%d%d%d", &lv, q, q + 1, q + 2, q + 3) == 5) {
                /* node: "level nw ne sw se", level 3 is an 8x8 leaf */
                level = lv - 3;
                for (int i = 0; i < 4; i++)
                    q[QUAD_POS(i)] = q[i] ? input_cells[q[i]] : EMPTY(level - 1);
            } else {
                /* leaf: '.' dead, '*' alive, '$' end of row */
                char c;
                long row = 0, col = 0, pos = 0;
                level = mem[0] = 0;
                while ((c = line[pos++])) {
                    if (c == '*')
                        mem[0] |= 1l << row * 8 + col;
                    col++;
                    if (c == '$')
                        row++, col = 0;
                }
                split(0, q);
            }
            root = input_cells[++num_cells] = make_cell(q, level * 4 + NODE_ONLY);
        }
    }

    history[0] = root;
    history[1] = level;
    Window window = XCreateSimpleWindow(display, RootWindow(display, 0),
                                        0, 0, X, Y, 1, 0, 0);
    history_start = history += 2;
    XImage *image = XCreateImage(display, DefaultVisual(display, 0), 24,
                                 ZPixmap, 0, (char *)pixels, X, Y, 32,
                                 STRIDE * 4);
    XEvent event;
    XSelectInput(display, window, KeyPressMask);
    XMapWindow(display, window);

    int print = 1;
    for (;;) {
        if (print)
            printf("G:%ld M:%d L%d S%d\n", generation, (int)next_free,
                   (int)level, (int)scale);
        print = 0;

        if (run_mode == (RUNNING | SINGLE_STEP))
            run_mode = SINGLE_STEP;   /* single step: stop after one step */

        draw(root, level, -(4l << level), -(4l << level), 0);
        XPutImage(display, window, DefaultGC(display, 0), image,
                  0, 0, 0, 0, X, Y);
        for (int i = 0; i < Y * (X + 128); i++)
            pixels[i] = 0;

        if (XCheckWindowEvent(display, window, KeyPressMask, &event)) {
            /* only the low 6 bits of the keysym are looked at */
            long key = XLookupKeysym(&event.xkey, 0) & 63;
            switch (key) {
            case 8:  /* BackSpace */
                backward ^= 1;
                break;
            case 29: /* ] */
                step_log++;
                break;
            case 27: /* [ (and Escape) */
                if (step_log)
                    step_log--;
                break;
            case 45: /* - */
                scale++;
                break;
            case 61: /* = */
                if (scale)
                    scale--;
                break;
            case 32: /* space */
                run_mode ^= RUNNING;
                break;
            case 48: /* 0 */
                run_mode ^= SINGLE_STEP;
                break;
            default:
                /* arrows: Left 17, Up 18, Right 19, Down 20 */
                if ((key - 1 & 15) < 4) {
                    long move = 1l << scale + 2;
                    if (!(key - 1 & 2))
                        move = -move;
                    if (key & 1)
                        view_x += move;
                    else
                        view_y += move;
                }
            }
        }

        if (run_mode & RUNNING) {
            if (!backward) {
                print = advance();
            } else {
                if (history > history_start) {
                    history -= 3;
                    generation = *history;
                    level = history[-1];
                    root = history[-2];
                }
                print = 1;
            }
        }
    }
}
