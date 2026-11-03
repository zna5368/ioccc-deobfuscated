/*
 * Cubic function plotter.
 *
 * Plots y = a*x^3 + b*x^2 + c*x + d into a 96x160 bitmap and then draws the
 * bitmap on a text terminal, packing each 4 row x 2 column block of pixels
 * into one character chosen from a "graphtab" table.
 */

#define BITROWS 96
#define BITCOLS 160
#define SCREENROWS 24
#define SCREENCOLS 80

/* plot area inside the bitmap */
#define MINR 10
#define MAXR 80
#define MINC 20
#define MAXC 150

/* box drawn two columns outside the plot area */
#define BOXMINC (MINC - 2)
#define BOXMAXC (MAXC + 2)

/* x range and scale factors: rows per y unit, columns per x unit */
#define MINX -20
#define MAXX 10
#define MAXY 500
#define YSCALE .07     /* (MAXR - MINR) / (MAXY - MINY) = 70 / 1000 */
#define XSCALE 4.33333 /* (MAXC - MINC) / (MAXX - MINX) = 130 / 30  */

#define SETBIT(r, c) framebuffer[r][c] = 1

char framebuffer[BITROWS][BITCOLS];

/*
 * graphtab[left][right]: left and right are the 4 bit columns of a 4x2
 * pixel block (top pixel is the high bit).
 */
char graphtab[16][17] = {
    " ..,.;:j'::;';!|",   "._:,;;;j/;;;jjjj",   ".,-,r<jq/;I;/|I|",
    ",,rx///d///d///d",   ".:\\;-=vq':<|':++",  ";;>;==gg':SS///+",
    ":\\\\\\r<wq/<64//d+", "lL+by[pg/</6///+",   "`::;`\\\\\\\"::;`\\YT",
    ";;;;`2>\\:=;;??]]",  ":;;;>2>\\F;O;7?]7",  ";;;;?2>b;;;;??]#",
    "'\\\\\\`\\\\\\''<5\"\"Vq", ";\\\\\\`=\\\\''55\"?gg",
    "ILLLD\\bfF[[[P?#M",  "|||||#+#T[FFP?PM",
};

/* the 4 pixels of one column of a character cell as a 4 bit number */
int column_bits(int row, int col)
{
    return (framebuffer[row + 0][col] << 3) + (framebuffer[row + 1][col] << 2) +
           (framebuffer[row + 2][col] << 1) + (framebuffer[row + 3][col] << 0);
}

main(argc, argv) char **argv;
{
    double x, y;
    double cube, square, linear, constant;
    int r, c, i;

    if (argc < 5)
        exit(1);

    cube = atof(argv[1]);
    square = atof(argv[2]);
    linear = atof(argv[3]);
    constant = atof(argv[4]);

    /* draw the box around the graph */
    for (i = MINR; i <= MAXR; i++) {
        SETBIT(i, BOXMINC);
        SETBIT(i, BOXMAXC);
    }
    for (i = BOXMINC; i <= BOXMAXC; i++) {
        SETBIT(MINR, i);
        SETBIT(MAXR, i);
    }

    /* plot the cubic */
    for (x = MINX; x <= MAXX; x += .05) {
        y = cube * x * x * x + square * x * x + linear * x + constant;
        r = (MAXY - y) * YSCALE + MINR;
        c = (x - MINX) * XSCALE + MINC;
        if (r >= MINR && r <= MAXR && c >= MINC && c <= MAXC)
            SETBIT(r, c);
    }

    /* show the bitmap, 4x2 pixels per character */
    for (r = 0; r < SCREENROWS; r++) {
        for (c = 0; c < SCREENCOLS - 1; c++)
            putchar(graphtab[column_bits(4 * r, 2 * c)][column_bits(4 * r, 2 * c + 1)]);
        putchar('\n');
    }
}
