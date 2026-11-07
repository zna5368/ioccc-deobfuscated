#include <stdio.h>

/*
 * Message encoder: writes a C file whose compiler warnings carry a message.
 *
 * Each nibble of the message becomes the text "O/0-" in the generated file.
 * The "/" makes the compiler print a "division by zero" warning, and the
 * column number in that warning (mod 16) is the nibble value.
 *
 * GCC (11+) counts a tab as moving to the next tab stop (every 8 columns),
 * while Clang counts a tab as a single column.  By mixing tabs and spaces
 * before "O/0-", the GCC column and the Clang column can be made different,
 * so one file can hold two different messages.
 */

/* Start of the generated program (also the start of a Ruby/Perl string) */
static const char header[] =
    "#include<stdio.h>/*\n"
    "$t=\"*/\n"
    "static int l=1,I,i,o;int main(int O,char**s){O=O?O:\n";

/* What is placed in front of each nibble.  "/0" triggers the warning. */
static const char div_by_zero[] = "O/0-";

/* Decoder in C: parses "file:line:col: warning: division by zero" lines
 * from stdin and rebuilds bytes from the column numbers (mod 16). */
static const char c_decoder[] =
    "**s;"
    "while((O=getchar())-EOF){"
    "l=l?l-100?l-105?O-58?O>47&&O<58?(i=i*10+O-48),l:10-O?100-O||l-5?l:O:1"
    ":2-l?l+1:(i=0)+3:O-118?6:o?(I|=i&15)?(o=!putchar(I))+8:0"
    ":11+(I=(i&15)<<(o=4)):O-105?6:O:0;"
    "}"
    "return 0;";

/* Ruby and Perl code that prints GCC-style warnings for every "/" that
 * follows an "O", so the message can be decoded without a C compiler. */
static const char script_decoder[] =
    "}/*\";"
    "$w=$x=$y=$z=1;"
    "$v=\"#{$w=8;$t.each_byte{|i|$u=i;%q[\";"
    "if($v){foreach$u(unpack\"C*\",$t){#];\n"
    "$u==47&&$z==79&&print($0,\":\",$y+1,\":\",$x,\":: div\\n\");"
    "$x+=$u==9?$w-~-$x%$w:1;"
    "$y+=$u==10?$x=1:0;"
    "$z=$u;"
    "}}#\"#*/\n";

/* Current 1-based column as GCC (tab stops of 8) and Clang (tab = 1) see it */
int gcc_column, clang_column;

/* Value of a hexadecimal digit, or -1 if it is not one */
int hex_value(int ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

/*
 * Print a string the way the original output routine did: whitespace other
 * than tab is dropped, upper case T..Z become "$" plus lower case, and the
 * letters J K L M N P Q R S are replaced by other symbols.  Only used for
 * the program name in the usage message.
 */
void print_filtered(const char *s)
{
    int ch;

    for (; (ch = *s); s++) {
        if (ch <= ' ' && ch != '\t')
            continue;
        if (ch >= 'T' && ch <= 'Z') {
            putchar('$');
            ch += 'a' - 'A';
        }
        switch (ch) {
        case 'J': ch = '\\'; break;
        case 'K': ch = ',';  break;
        case 'L': ch = ' ';  break;
        case 'M': ch = '$';  break;
        case 'N': ch = '#';  break;
        case 'P': ch = 6;    break;
        case 'Q': ch = '\n'; break;
        case 'R': ch = '3';  break;
        case 'S': ch = '"';  break;
        }
        putchar(ch);
    }
}

/* Translate C escape sequences (\n \t \x1b ...) in place */
void unescape(char *s)
{
    char *out = s;

    for (; *s; s++) {
        if (*s != '\\') {
            *out++ = *s;
            continue;
        }
        switch (*++s) {
        case 'r': *out++ = '\r'; break;
        case 'v': *out++ = '\v'; break;
        case 'n': *out++ = '\n'; break;
        case 't': *out++ = '\t'; break;
        case 'b': *out++ = '\b'; break;
        case 'a': *out++ = '\a'; break;
        case 'x':
            if (hex_value(s[1]) >= 0) {
                if (hex_value(s[2]) >= 0) {
                    s += 2;
                    *out++ = 16 * hex_value(s[-1]) + hex_value(s[0]);
                } else {
                    *out++ = hex_value(*++s);
                }
            } else {
                *out++ = 'x';
            }
            break;
        default:
            *out++ = *s;
            break;
        }
    }
    *out = 0;
}

/* GCC column reached after printing 'length' characters from 'gcc_col',
 * where bit n of 'tabs' set means character n is a tab, else a space */
int column_after(int gcc_col, int tabs, int length)
{
    int n;

    for (n = 0; n < length; n++)
        gcc_col += tabs & (1 << n) ? 8 - (gcc_col - 1) % 8 : 1;
    return gcc_col;
}

/*
 * Find the shortest run of tabs and spaces that makes
 * (GCC column - Clang column) mod 16 equal to 'offset'.
 * Returns it as a bit mask (bit set = tab), or 0 if no run is needed.
 */
int find_tab_pattern(int offset)
{
    int length, tabs, best = 0;

    for (length = 0; !best && offset != ((gcc_column - clang_column) & 15);
         length++) {
        for (tabs = (1 << length) - 1; tabs >= 0; tabs--) {
            int gcc_col = column_after(gcc_column, tabs, length);

            if (((gcc_col - clang_column - length) & 15) == offset)
                best = tabs;
        }
    }
    return best;
}

/* Emit one "O/0-" line segment that encodes a GCC nibble and a Clang nibble */
void encode_nibble(int gcc_nibble, int clang_nibble)
{
    int tabs = find_tab_pattern((gcc_nibble - clang_nibble) & 15);
    int target = (gcc_nibble - 1) & 15;

    /* Tabs and spaces that shift the GCC column relative to Clang's */
    for (; tabs; tabs /= 2) {
        if (tabs & 1) {
            fputs("\t", stdout);
            gcc_column += 8 - (gcc_column - 1) % 8;
        } else {
            fputs(" ", stdout);
            gcc_column += 1;
        }
        clang_column++;
    }

    /* Spaces until the "/" lands on a GCC column equal to the nibble */
    while (target != gcc_column % 16) {
        gcc_column++;
        clang_column++;
        fputs(" ", stdout);
    }

    fputs(div_by_zero, stdout);
    gcc_column += 4;
    clang_column += 4;
}

/* Emit one line of the generated program for a GCC byte and a Clang byte */
void encode_byte(char gcc_byte, char clang_byte)
{
    int shift;

    gcc_column = clang_column = 1;
    for (shift = 4; shift >= 0; shift -= 4)
        encode_nibble((gcc_byte >> shift) & 15, (clang_byte >> shift) & 15);
    fputs("\n", stdout);
}

int main(int argc, char **argv)
{
    char *gcc_msg, *clang_msg;

    if (argc < 2 || argc > 3) {
        print_filtered(argv[0]);
        fputs(" {msg1} [msg2]\n", stdout);
        return 1;
    }

    unescape(gcc_msg = clang_msg = argv[1]);
    if (argc > 2)
        unescape(clang_msg = argv[2]);

    fputs(header, stdout);
    for (; *gcc_msg || *clang_msg; gcc_msg++, clang_msg++) {
        encode_byte(*gcc_msg, *clang_msg);
        /* When one message ends, follow the other one */
        if (!*gcc_msg)
            gcc_msg = clang_msg;
        if (!*clang_msg)
            clang_msg = gcc_msg;
    }
    fputs(c_decoder, stdout);
    fputs(script_decoder, stdout);
    return 0;
}
