/*
 * Prototypes for the stdio functions used.  The original entry has no
 * #include lines and relies on implicit function declarations instead.
 */
int getchar(void);
int printf(const char *format, ...);
int puts(const char *string);

#define BYTES_PER_LINE 16
#define END_OF_FILE    (-1)

/*
 * The original declares "char o[];", a one byte array, and relies on the
 * zero filled memory after it to hold the rest of the line.  Here the
 * buffer is given its real size: 16 characters plus a terminating NUL.
 */
char column;
char ascii_line[BYTES_PER_LINE + 1];

/*
 * Bytes 32 to 127 (space through DEL) are shown as themselves in the
 * ASCII sidebar, everything else is shown as a dot.  The original tests
 * this with 4 < (4 ^ c >> 5), which is true only when c >> 5 is 1, 2 or 3.
 */
int is_shown(int c)
{
    return c >= 32 && c <= 127;
}

int main(void)
{
    int c;

    for (;;) {
        c = getchar();

        if (c == END_OF_FILE) {
            /* print what is left of an unfinished line */
            ascii_line[column] = '\0';
            if (column != 0) {
                puts(ascii_line);
            }
            break;
        }

        ascii_line[column] = is_shown(c) ? c : '.';
        printf("%02x ", c);
        column = (column + 1) % BYTES_PER_LINE;

        /* a full line of 16 bytes: print the ASCII sidebar */
        if (column == 0) {
            puts(ascii_line);
        }
    }

    return 0;
}
