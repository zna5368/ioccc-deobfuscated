#include <stdio.h>

/*
 * The table is drawn into a character canvas.  Every element gets a 4x4 cell
 * whose borders are shared with its neighbours:
 *
 *     +---+
 *     |26 |   atomic number, left justified
 *     |Fe |   chemical symbol
 *     | 56|   atomic weight, right justified
 *     +---+
 */
#define LINES 39
#define WIDTH 73 /* 18 groups * 4 characters + the closing border */
#define CELL 4

static char canvas[LINES][WIDTH];

/* chemical symbols in order of atomic number (element 0 is unused) */
static const char *symbol[] = {
    "",   "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na",
    "Mg", "Al", "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",
    "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br",
    "Kr", "Rb", "Sr", "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag",
    "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La", "Ce", "Pr",
    "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu",
    "Hf", "Ta", "W",  "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi",
    "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am",
    "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Ha"};

/* atomic weights rounded to whole numbers, in order of atomic number */
static const int weight[] = {
    0,   1,   4,   7,   9,   11,  12,  14,  16,  19,  20,  23,  24,  27,  28,
    31,  32,  35,  40,  39,  40,  45,  48,  51,  52,  55,  56,  59,  59,  64,
    65,  70,  73,  75,  79,  80,  84,  85,  88,  89,  91,  93,  96,  99,  101,
    103, 106, 108, 112, 115, 119, 122, 128, 127, 131, 133, 137, 139, 140, 141,
    144, 145, 150, 152, 157, 159, 162, 165, 167, 169, 173, 175, 178, 181, 184,
    186, 190, 192, 195, 197, 201, 204, 207, 209, 209, 210, 222, 223, 226, 227,
    232, 231, 238, 237, 244, 243, 247, 247, 251, 254, 257, 256, 254, 257, 261,
    260};

/*
 * Runs of consecutive elements that sit side by side in the table.
 * line is the canvas line of the top border, group is the cell column.
 */
static const struct run {
    int first, count, line, group;
} layout[] = {
    {1, 1, 0, 0},    {2, 1, 0, 17},                     /* period 1 */
    {3, 2, 4, 0},    {5, 6, 4, 12},                     /* period 2 */
    {11, 2, 8, 0},   {13, 6, 8, 12},                    /* period 3 */
    {19, 18, 12, 0},                                    /* period 4 */
    {37, 18, 16, 0},                                    /* period 5 */
    {55, 3, 20, 0},  {72, 15, 20, 3},                   /* period 6 */
    {87, 3, 24, 0},  {104, 2, 24, 3},                   /* period 7 */
    {58, 14, 30, 2},                                    /* lanthanides */
    {90, 14, 34, 2},                                    /* actinides */
};

static void put(int line, int column, const char *text)
{
    while (*text)
        canvas[line][column++] = *text++;
}

static void draw_element(int number, int line, int column)
{
    char text[8];
    int i;

    /* box: top and bottom edges, then the left and right sides */
    put(line, column, "+---+");
    put(line + CELL, column, "+---+");
    for (i = 1; i < CELL; i++) {
        canvas[line + i][column] = '|';
        canvas[line + i][column + CELL] = '|';
    }

    sprintf(text, "%-3d", number);
    put(line + 1, column + 1, text);
    sprintf(text, "%-2s", symbol[number]);
    put(line + 2, column + 1, text);
    sprintf(text, "%3d", weight[number]);
    put(line + 3, column + 1, text);
}

int main()
{
    int r, i, line, column;

    for (line = 0; line < LINES; line++)
        for (column = 0; column < WIDTH; column++)
            canvas[line][column] = ' ';

    for (r = 0; r < (int)(sizeof layout / sizeof layout[0]); r++)
        for (i = 0; i < layout[r].count; i++)
            draw_element(layout[r].first + i, layout[r].line,
                         (layout[r].group + i) * CELL);

    for (line = 0; line < LINES; line++)
        printf("%.*s\n", WIDTH, canvas[line]);

    return 0;
}
