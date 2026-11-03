#include <ctype.h>
#include <stdio.h>

/*
 * A C-aware line folder: the source is split into tokens, needless white
 * space and comments are dropped, and the remaining tokens are written back
 * out with lines broken so they fit within the requested width.  Preprocessor
 * lines (starting with '#') are never folded.
 */

#define BUFFER_SIZE 300000
#define DEFAULT_WIDTH 79

/* token types returned by next_token() */
#define TOKEN_WORD 0    /* identifier, keyword or number */
#define TOKEN_PUNCT 1   /* operator, punctuation or string/char literal */
#define TOKEN_EOF 2     /* end of input */
#define TOKEN_SPACE 3   /* run of blanks, tabs and newlines */
#define TOKEN_COMMENT 4 /* a comment */

char input[BUFFER_SIZE + 1];
char *input_end;

/* operators made of two characters, stored back to back */
char *two_char_operators = ">=" "<=" "!=" "==" "||" "&&" "->" "++" "--" ">>" "<<";

/*
 * Find the token starting at start, store a pointer just past it in *end
 * and return its type.
 *
 * Note: strchr() also matches the terminating '\0', so a NUL byte counts as
 * white space and as an exponent sign; this mirrors the original program.
 */
int next_token(char *start, char **end)
{
    char *p;
    char *op;
    int is_number;
    int type;

    if (start >= input_end) {
        *end = start;
        return TOKEN_EOF;
    }

    /* identifiers and numbers (including things like 1.5e+10) */
    is_number = isdigit(*start) || (*start == '.' && isdigit(start[1]));
    for (p = start; p < input_end; p++) {
        if (isalnum(*p) || *p == '_')
            continue;
        if (is_number && *p == '.')
            continue;
        if (is_number && (p[-1] == 'e' || p[-1] == 'E') && strchr("+-", *p))
            continue;
        break;
    }
    if (p > start) {
        *end = p;
        return TOKEN_WORD;
    }

    /* a run of white space; p is left on its last character */
    type = TOKEN_PUNCT;
    for (p = start; p < input_end && strchr(" \t\n", *p); p++)
        ;
    if (p > start) {
        p--;
        type = TOKEN_SPACE;
    }

    /* string or character literal; p is left on the closing quote */
    if (*start == '"' || *start == '\'') {
        for (p = start + 1; p < input_end && *p != *start; p++)
            if (*p == '\\')
                p++; /* skip the escaped character */
    }

    /* two character operators */
    for (op = two_char_operators; *op && strncmp(op, start, 2); op += 2)
        ;
    if (*op)
        p = start + 1;

    /*
     * Comment; p is left on the closing '/'.  The character right after
     * the opening and the one after every '*' are never examined, so this
     * search is a little sloppy, just like the original.
     */
    if (!strncmp("/*", start, 2)) {
        p = start + 2;
        for (;;) {
            p++;
            if (*p != '*')
                continue;
            p++;
            if (*p == '/')
                break;
        }
        type = TOKEN_COMMENT;
    }

    *end = p + 1;
    return type;
}

int main(int argc, char **argv)
{
    char *prev_start, *prev_end; /* last token written out */
    char *cur_start, *cur_end;   /* token being handled */
    char *next_start, *next_end; /* token that follows it */
    char *p;
    int prev_type = TOKEN_PUNCT;
    int cur_type, next_type;
    int width;
    int column = 0;  /* characters already on the output line */
    int folding = 1; /* 0 while inside a preprocessor line */
    int dropped;

    width = argc > 1 ? atoi(argv[1]) : DEFAULT_WIDTH;
    prev_start = prev_end = cur_start = input;
    input_end = input + read(0, input, BUFFER_SIZE);

    cur_type = next_token(cur_start, &cur_end);
    while (cur_type != TOKEN_EOF) {
        next_start = cur_end;
        next_type = next_token(next_start, &next_end);
        dropped = 0;

        /* a '#' starts a preprocessor line: begin it on a fresh line */
        if (cur_type == TOKEN_PUNCT && cur_end - cur_start == 1 &&
            *cur_start == '#') {
            if (column)
                putchar('\n');
            column = 0;
            folding = 0;
        }

        if (!folding && cur_type == TOKEN_SPACE &&
            (char *)strchr(cur_start, '\n') < cur_end) {
            /* the newline that ends a preprocessor line */
            putchar('\n');
            column = 0;
            folding = 1;
        } else if (cur_type >= TOKEN_SPACE &&
                   (prev_type != TOKEN_WORD || next_type != TOKEN_WORD) &&
                   (folding || prev_type != TOKEN_WORD) &&
                   !(prev_end - prev_start <= 1 && *prev_start == '=' &&
                     next_end - next_start <= 1 && strchr("-*&", *next_start))) {
            /*
             * White space or a comment that does not separate two words
             * is not needed, except between '=' and a following '-', '*'
             * or '&' (to avoid creating the old "=-" style operators).
             */
            dropped = 1;
        } else if (cur_type == TOKEN_SPACE) {
            /* needed white space becomes one blank or a line break */
            if (folding && column && column + 1 + (next_end - next_start) > width) {
                putchar('\n');
                column = 0;
            } else {
                putchar(' ');
                column++;
            }
        } else {
            /* any other token is copied, on a new line if it won't fit */
            if (folding && column && column + (cur_end - cur_start) > width) {
                putchar('\n');
                column = 0;
            }
            column += cur_end - cur_start;
            for (p = cur_start; p < cur_end; p++)
                putchar(*p);
        }

        if (!dropped) {
            prev_start = cur_start;
            prev_end = cur_end;
            prev_type = cur_type;
        }
        cur_start = cur_end;
        cur_end = next_end;
        cur_type = next_type;
    }
    if (column)
        putchar('\n');
    return 0;
}
