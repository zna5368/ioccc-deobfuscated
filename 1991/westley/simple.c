/*
 * Self-reproducing, self-learning tic-tac-toe.
 *
 * The human plays 'X' and the computer plays '0' (zero).  The program reads
 * the board, adds the human's move from the command line, picks a reply and
 * prints the source of the next generation of the original (obfuscated)
 * program with both moves drawn on its board.
 *
 * Board cells are numbered
 *
 *     1 2 3
 *     4 5 6
 *     7 8 9
 *
 * and kept in two 9-bit masks (one for X, one for 0).  Cell 1 is bit 8 and
 * cell 9 is bit 0, so cell n is the bit 1 << (9 - n).
 */

int putchar(int c);

#define CELLS 9
#define WIN_LINES 8

/* the computer's starting board: the original stores it inside its own code */
static const char board[3][3 + 1] = {
    "   ",
    "   ",
    "   ",
};

/*
 * Masks 0..7 are the eight winning lines.  Masks 8..17 are "move templates":
 * a template is a set of cells that becomes a winning line when one more cell
 * is added, so it is used to steer the computer towards a particular cell.
 */
static const int patterns[] = {
    0222, /* line: cells 2 5 8 (middle column) */
    0007, /* line: cells 7 8 9 (bottom row)    */
    0444, /* line: cells 1 4 7 (left column)   */
    0111, /* line: cells 3 6 9 (right column)  */
    0700, /* line: cells 1 2 3 (top row)       */
    0070, /* line: cells 4 5 6 (middle row)    */
    0124, /* line: cells 3 5 7 (diagonal)      */
    0421, /* line: cells 1 5 9 (diagonal)      */
    0505, /* template: cells 1 3 7 9           */
    0451, /* template: cells 1 4 6 9           */
    0154, /* template: cells 3 4 6 7           */
    0504, /* template: cells 1 3 7             */
    0026, /* template: cells 5 7 8             */
    0050, /* template: cells 4 6               */
    0060, /* template: cells 4 5               */
    0050, /* template: cells 4 6               */
    0021, /* template: cells 5 9               */
    0505, /* template: cells 1 3 7 9           */
};

/*
 * Text of the original program.  Lines 1-3 are the three source lines that
 * make up one row of the board (line 1 starts with the row function name),
 * line 4 holds the learning data and lines 5-12 are the program code.
 */
static char source[][100] = {
    "char z[][100]={",
    "a(X){/*/X=-",
    "-1;F;X=-",
    "-1;F;}/*/",
    "9999999999  :-| ",
    "int q,i,j,k,X,O=0,H;S(x)int*x;{X+=X;O+=O;*x+1?*x+2||X++:O++;*x=1;}L(n){for(*",
    "z[i=1]=n+97;i<4;i++)M(256),s(i),M(128),s(i),M(64),N;X*=8;O*=8;}s(R){char*r=z",
    "[R];for(q&&Q;*r;)P(*r++);q&&(Q,P(44));}M(m){P(9);i-2||P(X&m?88:O&m?48:32);P(",
    "9);}y(A){for(j=8;j;)~A&w[--j]||(q=0);}e(W,Z){for(i-=i*q;i<9&&q;)y(W|(1<<i++&",
    "~Z));}R(){for(k=J[*J-48]-40;k;)e(w[k--],X|O);}main(u,v)char**v;{a(q=1);b(1);",
    "c(1);*J=--u?O?*J:*v[1]:53;X|=u<<57-*v[u];y(X);K=40+q;q?e(O,X),q&&(K='|'),e(X",
    ",O),R(),O|=1<<--i:J[*J-48+(X=O=0)]--;L(q=0);for(s(i=0);q=i<12;)s(i++),i>4&&N",
    ";s(q=12);P(48);P('}');P(59);N;q=0;L(1);for(i=5;i<13;)s(i++),N;L(2);}",
    0,
};

#define LAST_LINE 12
#define FIRST_CODE_LINE 5
#define MEMORY_LINE 4

/*
 * The memory line "9999999999  :-| ":
 *   [0]     the first move of the game ('5' if the computer opened)
 *   [1..9]  for each possible first move, the template to start searching at
 *           ('9' means template 17, '8' template 16, ...)
 *   [14]    the mouth of the face: '|' normal, ')' computer won,
 *           '(' computer lost
 */
static char *const memory = source[MEMORY_LINE];
#define FIRST_MOVE memory[0]
#define FACE_MOUTH memory[14]

static int x_mask, o_mask;

static int has_winning_line(int mask)
{
    int line;

    for (line = 0; line < WIN_LINES; line++)
        if ((patterns[line] & ~mask) == 0)
            return 1;
    return 0;
}

/*
 * Find a bit that, added to mask `mine`, completes a winning line.  Bits
 * that are set in `occupied` are not added (but are still tried, so a mask
 * that already wins returns bit 0).  Returns -1 if there is no such bit.
 */
static int find_winning_bit(int mine, int occupied)
{
    int bit;

    for (bit = 0; bit < CELLS; bit++)
        if (has_winning_line(mine | ((1 << bit) & ~occupied)))
            return bit;
    return -1;
}

static void read_board(void)
{
    int row, col;

    x_mask = o_mask = 0;
    for (row = 0; row < 3; row++)
        for (col = 0; col < 3; col++) {
            x_mask <<= 1;
            o_mask <<= 1;
            if (board[row][col] == 'X')
                x_mask |= 1;
            else if (board[row][col] == '0')
                o_mask |= 1;
        }
}

static int choose_computer_move(void)
{
    int bit, template;

    /* take a win if there is one */
    bit = find_winning_bit(o_mask, x_mask);
    if (bit >= 0)
        return bit;

    FACE_MOUTH = '|';

    /* block the human's win */
    bit = find_winning_bit(x_mask, o_mask);
    if (bit >= 0)
        return bit;

    /* use the learned move template for this opening */
    for (template = memory[FIRST_MOVE - '0'] - 40; template > 0; template--) {
        bit = find_winning_bit(patterns[template], x_mask | o_mask);
        if (bit >= 0)
            return bit;
    }
    return CELLS - 1;
}

static void print_source_line(int line, int quoted)
{
    const char *p;

    if (quoted)
        putchar('"');
    for (p = source[line]; *p; p++)
        putchar(*p);
    if (quoted) {
        putchar('"');
        putchar(',');
    }
}

static void print_cell(int line, int mask)
{
    putchar('\t');
    if (line == 2)
        putchar(x_mask & mask ? 'X' : o_mask & mask ? '0' : ' ');
    putchar('\t');
}

/* print the three source lines of a board row ("a(X)...", "b(X)...", ...) */
static void print_board_row(int row)
{
    int line;

    source[1][0] = 'a' + row;
    for (line = 1; line <= 3; line++) {
        print_cell(line, 0400);
        print_source_line(line, 0);
        print_cell(line, 0200);
        print_source_line(line, 0);
        print_cell(line, 0100);
        putchar('\n');
    }
    x_mask <<= 3;
    o_mask <<= 3;
}

int main(int argc, char **argv)
{
    int moves = argc - 1;
    int line;

    read_board();

    /* remember the opening move: the human's first move or '5' */
    if (moves)
        FIRST_MOVE = o_mask ? FIRST_MOVE : argv[1][0];
    else
        FIRST_MOVE = '5';

    if (moves)
        x_mask |= moves << ('9' - argv[moves][0]);

    if (has_winning_line(x_mask)) {
        /* the human won: be sad, clear the board and learn */
        FACE_MOUTH = '(';
        x_mask = o_mask = 0;
        memory[FIRST_MOVE - '0']--;
    } else {
        FACE_MOUTH = ')';
        o_mask |= 1 << choose_computer_move();
    }

    /* print the next generation of the program */
    print_board_row(0);

    print_source_line(0, 0);
    for (line = 0; line < LAST_LINE; line++) {
        print_source_line(line, 1);
        if (line >= MEMORY_LINE)
            putchar('\n');
    }
    print_source_line(LAST_LINE, 1);
    putchar('0');
    putchar('}');
    putchar(';');
    putchar('\n');

    print_board_row(1);
    for (line = FIRST_CODE_LINE; line <= LAST_LINE; line++) {
        print_source_line(line, 0);
        putchar('\n');
    }
    print_board_row(2);
    return 0;
}
