/*
 * Braille filter.
 *
 * Plain text lines are converted into three rows of '*' dots (Braille).
 * Three rows of Braille dots are converted back into plain text.
 *
 * A Braille cell is two columns of three dots.  Each column is stored as a
 * 3 bit number (top dot = 1, middle dot = 2, bottom dot = 4).  The 6 bit cell
 * value is left column + (right column << 3).
 */

#define ROW_SIZE 120 /* each Braille row gets its own slot in the line buffer */
#define DOT '*'
#define BLANK ' '

/* Braille "number sign" cell: left column dot 3, right column dots 4,5,6 */
#define NUMBER_SIGN_LEFT 4
#define NUMBER_SIGN_RIGHT 7
#define NUMBER_SIGN (NUMBER_SIGN_LEFT + (NUMBER_SIGN_RIGHT << 3))

/* character for each 6 bit Braille cell value, '?' for unused patterns */
static char braille[] =
    " A?B?K?L?CIF?MSP?E?H?O?R?DJG?NTQ?????U?V?????X???????Z????W??Y??";

/* highest cell value whose character is ch, or 0 (blank) if there is none */
static int find_cell(char ch)
{
    int cell = 63;

    while (cell > 0 && braille[cell] != ch)
        cell--;
    return cell;
}

/* three rows of dots -> text */
static void decode_braille(char *rows, int *cols)
{
    char *top = rows, *middle = rows + ROW_SIZE, *bottom = rows + 2 * ROW_SIZE;
    int i, width, cell;

    /* combine the three rows into one 3 bit value per column */
    for (i = 0; top[i]; i++)
        cols[i] = top[i] != BLANK;
    for (; i < ROW_SIZE; i++)
        cols[i] = 0;
    for (i = 0; middle[i]; i++)
        cols[i] += (middle[i] != BLANK) * 2;
    for (i = 0; bottom[i]; i++)
        cols[i] += (bottom[i] != BLANK) * 4;

    for (i = 0; i < ROW_SIZE; i++) {
        if (!cols[i])
            continue;

        /* join the run of non-blank columns into one number, 3 bits each */
        for (width = 1; cols[i + width]; width++)
            cols[i + width] = cols[i + width - 1] + (cols[i + width] << (3 * width));
        i += width - 1;
        cell = cols[i];

        if (cell % 64 == NUMBER_SIGN && width <= 4)
            /* number sign followed by A..J means digit 1..9,0 */
            putchar('0' + (braille[cell / 64] - 4) % 10);
        else if (width > 2)
            putchar('?');
        else
            putchar(braille[cell]);

        /* two more blank columns after the separator mean a space */
        if (cols[i + 2] + cols[i + 3] == 0)
            putchar(' ');
    }
    putchar('\n');
}

/* text -> three rows of dots */
static void encode_text(char *text, int *cols)
{
    char ch;
    int n = 0, cell, bit, i;

    for (; *text; text++) {
        ch = *text;
        if (ch > '`')
            ch -= 'a' - 'A';

        if (ch >= '0' && ch <= '9') {
            /* digits are a number sign followed by a letter A..J */
            ch = "JABCDEFGHI"[ch - '0'];
            cols[n++] = NUMBER_SIGN_LEFT;
            cols[n++] = NUMBER_SIGN_RIGHT;
        }
        cell = find_cell(ch);

        cols[n++] = cell % 8; /* left column */
        if (cell / 8)
            cols[n++] = cell / 8; /* right column */
        cols[n++] = 0;            /* blank separator column */
    }
    n--; /* drop the last separator */

    if (n > 0) {
        for (bit = 1; bit < 8; bit *= 2) {
            for (i = 0; i < n; i++)
                putchar(cols[i] & bit ? DOT : BLANK);
            putchar('\n');
        }
        putchar('\n');
    }
}

main()
{
    char *p, line[3 * ROW_SIZE];
    int cols[729];

    while (gets(line)) {
        if (line[0] == '#') /* skip lines such as #define */
            continue;

        /* a line containing a dot starts a block of three Braille rows */
        for (p = line; *p && *p != DOT; p++)
            ;
        if (*p && gets(line + ROW_SIZE) && gets(line + 2 * ROW_SIZE))
            decode_braille(line, cols);
        else
            encode_text(line, cols);
    }
}
