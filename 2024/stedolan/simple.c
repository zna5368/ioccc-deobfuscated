/*
 * 2024/stedolan - decode a 40 bit icon into an 80x80 PBM image
 *
 * The input is 10 hex digits (40 bits).  The icon is a 10x10 grid of
 * cells, each cell drawn as an 8x8 block of pixels.  The outer ring of
 * cells is always blank, leaving 8 rows of 8 cells for the picture.
 *
 * Each picture row uses 5 bits of the input (lowest bits = top row).
 * The 8 cells of a row are taken from those 5 bits in the order
 *
 *     bit 0, 1, 2, 3, 4, 2, 1, 0
 *
 * so the left five cells are stored directly and the right three cells
 * mirror cells 2, 1 and 0.  This is how 40 bits produce 64 cells.
 *
 * stdio.h is supplied by the Makefile with -include stdio.h
 */

#define IMAGE_SIZE      80      /* pixels per side */
#define CELL_SIZE       8       /* pixels per cell side */
#define GRID_SIZE       (IMAGE_SIZE / CELL_SIZE)   /* cells per side (10) */
#define BITS_PER_ROW    5       /* input bits used by one picture row */

/* which input bit (within a row) is shown in each grid column, -1 = blank */
static const int column_bit[GRID_SIZE] = { -1, 0, 1, 2, 3, 4, 2, 1, 0, -1 };

int main(void)
{
    unsigned long icon = 0;
    int y, x;

    puts("P1\n80 80");                 /* plain PBM header: width height */
    scanf("%10lx", &icon);             /* read up to 10 hex digits */

    for (y = 0; y < IMAGE_SIZE; y++) {
        int grid_row = y / CELL_SIZE;

        for (x = 0; x < IMAGE_SIZE; x++) {
            int grid_col = x / CELL_SIZE;
            int bit = column_bit[grid_col];
            int black = 0;

            /* the first and last grid rows are a blank border */
            if (bit >= 0 && grid_row >= 1 && grid_row <= GRID_SIZE - 2) {
                unsigned long row_bits = icon >> ((grid_row - 1) * BITS_PER_ROW);
                black = (row_bits >> bit) & 1;
            }
            putchar(black ? '1' : '0');
        }
        putchar('\n');
    }
    return 0;
}
