/*
 * Print a table of the characters for each byte value, 16 per row, with
 * hexadecimal digits labelling the columns and rows.
 *
 * Rows start at 0x20 (space). If the program name (argv[0]) starts with a
 * character whose code is even, the table stops after 0x7F (rows 2-7). If it
 * is odd, the table goes on through 0xFF (rows 2-15).
 */

/* Return the hexadecimal digit character for a value 0-15. */
char hex_digit(int value)
{
    if (value < 10)
        return '0' + value;
    return 'A' + (value - 10);
}

/* Print the column header: the 16 hex digits 0-F. */
void print_header(void)
{
    int column;

    printf("   ");
    for (column = 0; column < 16; column++)
        printf("%c", hex_digit(column));
    printf("\n\n");
}

/* Print one row: its hex digit, then the 16 characters row*16 .. row*16+15. */
void print_row(int row)
{
    int column;

    printf("%c  ", hex_digit(row));
    for (column = 0; column < 16; column++)
        printf("%c", row * 16 + column);
    printf("\n");
}

int main(int argc, char **argv)
{
    int row;
    int end_row;

    /*
     * Odd first character of the program name: 1 << 4 = 16 rows (all bytes).
     * Even first character: 1 << 3 = 8 rows (ASCII only).
     */
    end_row = 1 << (3 + argv[0][0] % 2);

    print_header();
    for (row = 2; row < end_row; row++)
        print_row(row);

    return 0;
}
