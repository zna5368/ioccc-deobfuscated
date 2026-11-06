#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

/*
 * tac - tokenize and count C source read from stdin.
 *
 * The usage message.  The original takes it from the U macro which the
 * Makefile passes on the command line.
 */
static const char usage_text[] =
    "prog [-tcksri] < file.c\n"
    "tokenize and count stdin\n"
    "\t-t\ttokens only\n"
    "\t-c\tcompatible counts\n"
    "\t-k\tkeep comments\n"
    "\t-s\tsuppress output\n"
    "\t-r\treserved words count as 1\n"
    "\t-i\tioccc count only\n";

/*
 * The reserved word list (the original #includes it from the file named by
 * the W macro).  Words are grouped by length; each space starts the group
 * for the next length, so the first group (length 1) is empty, the second
 * holds "if", "do" and "or", the third "int", "for", "#if", ...
 */
static const char keyword_list[] =
    "  ifdoor intfor#ifandnotxor voidcharelsegotocaselongenumautobooltrue "
    "whilebreakconstfloatshortunion#else#elif#line_Boolfalsecomplbitoror_eq "
    "returnsizeofstructdoubleswitchstaticexternsigned#ifdef#endifinline"
    "#errorbitandand_eqnot_eqxor_eq typedefdefault#ifndef#pragma_Atomic"
    "_Pragmaalignasalignof #includeunsignedcontinuevolatileregisterrestrict"
    "#warning_Complex_Generic_Alignas_Alignofnoreturn _Noreturn _Imaginary  "
    "thread_local _Thread_localstatic_assert _Static_assert";

static char keyword_chars[sizeof keyword_list];
static int keyword_groups;          /* number of entries in group_start[] */
static int group_start[64];         /* group_start[n] = first word of length n */

/* option flags */
enum {
    COMPAT   = 0x01,   /* -c  count like iocccsize */
    KEEP     = 0x02,   /* -k  keep comments */
    RESERVED = 0x04,   /* -r  reserved words count as 1 */
    SUPPRESS = 0x08,   /* -s  suppress the token listing */
    IOCCC    = 0x10,   /* -i  print only the IOCCC size */
    TOKENS   = 0x20    /* -t  print only the tokens */
};

/* token codes; single character tokens are their own character */
enum {
    COMMENT      = 256,
    KEYWORD      = 257,
    IDENTIFIER   = 258,
    PREPROCESSOR = 258,   /* whole # line, only with -t */
    INTEGER      = 260,
    FLOAT        = 261,
    ARROW        = 262,   /* -> (and also <>) */
    WITH_EQUALS  = 128,   /* c|128: +=, ==, <= ... */
    DOUBLED      = 256    /* c|256: ++, &&, << ... (c|384: <<=, >>=) */
};

/* character classes */
enum {
    BRACE  = 0x01,   /* ; { }  (free when followed by whitespace) */
    HEX    = 0x02,   /* hexadecimal digit */
    DIGIT  = 0x04,   /* decimal digit */
    IDENT  = 0x08,   /* identifier character (including #) */
    SPACE  = 0x10,   /* whitespace (including NUL) */
    COMBO  = 0x20,   /* - < >  may form ->, <<=, >>= */
    ASSIGN = 0x40,   /* may be followed by = */
    DOUBLE = 0x80,   /* may be doubled: && ++ -- << >> || */
    DH  = DIGIT | HEX,
    IH  = IDENT | HEX,
    AD  = ASSIGN | DOUBLE,
    ADC = ASSIGN | DOUBLE | COMBO
};

static const unsigned char char_class[256] = {
 /*      0      1      2      3      4      5      6      7      8      9      a      b      c      d      e      f */
 /*0*/ SPACE,     0,     0,     0,     0,     0,     0,     0,     0, SPACE, SPACE, SPACE, SPACE, SPACE,     0,     0,
 /*1*/     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
 /*2*/ SPACE,ASSIGN,     0, IDENT,     0,ASSIGN,    AD,     0,     0,     0,ASSIGN,    AD,     0,   ADC,     0,ASSIGN,
 /*3*/    DH,    DH,    DH,    DH,    DH,    DH,    DH,    DH,    DH,    DH,     0, BRACE,   ADC,ASSIGN,   ADC,     0,
 /*4*/     0,    IH,    IH,    IH,    IH,    IH,    IH, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
 /*5*/ IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,     0,     0,     0,ASSIGN, IDENT,
 /*6*/     0,    IH,    IH,    IH,    IH,    IH,    IH, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
 /*7*/ IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, BRACE,    AD, BRACE,     0,     0,
};

/* ??x trigraph replacements */
static const unsigned char trigraph[256] = {
    ['='] = '#', ['('] = '[', ['/'] = '\\', [')'] = ']', ['\''] = '^',
    ['<'] = '{', ['!'] = '|', ['>'] = '}',  ['-'] = '~',
};

static unsigned char *buf;     /* source text */
static unsigned char *end;     /* end of source text (a NUL) */
static unsigned char *cur;     /* scanning position */
static unsigned char *white;   /* whitespace before the current token */
static unsigned char *start;   /* start of the current token */

static int flags;
static int lines, words, bytes, ioccc_size, saved, extra;
static int keyword_count, keyword_saved;
static int after_block_comment;

/* print at most len characters, stopping at a NUL */
static void print_text(unsigned char *text, int len)
{
    for (; len-- && *text; putchar(*text++))
        ;
}

static void init_keywords(void)
{
    const char *k;
    int n = 0;

    for (k = keyword_list; *k; ++k) {
        if (*k == ' ')
            group_start[++keyword_groups] = n;
        else
            keyword_chars[n++] = *k;
    }
    group_start[++keyword_groups] = n;
}

/* compare word against the reserved words of the same length */
static int is_keyword(unsigned char *word, int len)
{
    char *kw, *kw_end;
    int n;

    if (len <= 0 || len >= keyword_groups)
        return 0;
    kw_end = keyword_chars + group_start[len + 1];
    for (kw = keyword_chars + group_start[len]; kw < kw_end; kw += len) {
        for (n = 0; n < len && word[n] == kw[n]; ++n)
            ;
        if (n == len) {
            keyword_saved += len - 1;
            return ++keyword_count;
        }
    }
    return 0;
}

/* skip past the character stop, honouring backslash escapes */
static void skip_until(int stop)
{
    unsigned a = 0;
    int ch;

    while (a != stop && cur < end) {
        ch = a = *cur++;
        if (a == '\\' && (*cur == '\\' || *cur == '\'' || *cur == '"' || *cur == '\n'))
            ch = *cur++;
        if (ch == '\n')
            ++lines;
    }
}

/* return the next token code; start..cur is the token text */
static int lex(void)
{
    int c = 0, cls, next;

    white = cur;
    while (cur <= end && char_class[c = *cur++] & SPACE) {
        ++bytes;
        if (c == '\n')
            ++lines;
    }
    start = cur - 1;
    if (cur >= end) {
        if (c == 0)
            --bytes;   /* the terminating NUL was counted as whitespace */
        return c;
    }

    cls = char_class[c];
    if ((flags & TOKENS) && c == '#') {
        skip_until('\n');
        return PREPROCESSOR;
    }
    if (cls & IDENT) {
        while (char_class[c] & (IDENT | DIGIT))
            c = *cur++;
        --cur;
        return is_keyword(start, cur - start) ? KEYWORD : IDENTIFIER;
    }
    if (cls & DIGIT) {
        if (c == '0' && (*cur | 32) == 'x')
            c = *++cur;
        while (char_class[c] & (DIGIT | HEX))
            c = *cur++;
        if (c == '.') {
            strtod((char *)start, (char **)&cur);
            return FLOAT;
        }
        --cur;
        return INTEGER;
    }
    if (c == '/' && (*cur == '/' || *cur == '*')) {
        if (*cur++ == '/') {
            skip_until('\n');
            return COMMENT;
        }
        skip_until('*');
        while (cur < end && *cur++ != '/') {
            --cur;
            skip_until('*');
        }
        return COMMENT;
    }
    if (c == '\'' || c == '"') {
        skip_until(c);
        return c;
    }

    next = *cur++;
    if (next == '=' && (cls & ASSIGN))
        return c | WITH_EQUALS;
    if ((cls & DOUBLE) && next == c) {
        if (*cur == '=' && (cls & COMBO) && c != '-') {
            ++cur;
            return c | DOUBLED | WITH_EQUALS;
        }
        return c | DOUBLED;
    }
    if ((cls & COMBO) && next == '>')
        return ARROW;
    --cur;
    return *cur ? c : ' ';   /* an operator just before a NUL is reported as ' ' */
}

/* iocccsize compatibility: whitespace inside the token is not counted */
static void compat_words(int len, int inword)
{
    int c, is_space;

    for (c = 0; c < len; ++c) {
        is_space = char_class[start[c]] & SPACE;
        if (is_space || (char_class[start[c]] & BRACE && char_class[start[c + 1]] & SPACE))
            --ioccc_size;
        if (inword)
            inword = !is_space;
        else if (!is_space)
            inword = ++words;
    }
}

/* iocccsize compatibility: reserved words inside // comments count as 1 */
static void compat_keywords(int len, int inword)
{
    int c, word = 2;

    for (c = 2; c < len; ++c) {
        if (inword) {
            inword = char_class[start[c]] & IDENT;
            if (!inword && is_keyword(start + word, c - word))
                ioccc_size -= c - word - 1;
        } else if (char_class[start[c]] & IDENT) {
            inword = 1;
            word = c;
        }
    }
}

/* iocccsize compatibility: adjust the counts and print the source */
static void compat(int token, int len)
{
    int keep = flags & KEEP;
    int inword, c;
    unsigned char *p;

    if (after_block_comment) {
        /* spaces up to the end of line after a block comment are not counted */
        after_block_comment = 0;
        for (p = white; p < start && *p++ != '\n'; )
            ;
        if (!keep && p[-1] == '\n')
            --lines;
        bytes -= p - white;
    }

    if (white == start && white[-1] == '\n')
        --white;
    if (token == '"' || token == '\'')
        compat_words(len, 1);

    inword = white == start && start > buf;
    if (token != COMMENT && inword && char_class[white[-1]] != SPACE)
        --words;

    if (!(flags & SUPPRESS) && (token != COMMENT || keep)) {
        print_text(white, start - white);
        print_text(start, len);
    }

    if (token != COMMENT)
        return;

    if (keep) {
        ioccc_size += len;
        compat_words(len, inword);
        if (start[1] == '/') {
            /* starts "in a word" when the comment ends with whitespace */
            compat_keywords(len, char_class[start[len - 1]] & SPACE);
        }
    } else {
        for (c = 0; c < len; ++c)
            if (start[c] == '\n')
                --lines;
        if (!inword && char_class[start[len]] != SPACE && start[1] != '/')
            ++words;
    }

    /* spaces before a comment that begins a line are not counted */
    for (p = start; p > white && *--p != '\n'; )
        ;
    if (*p == '\n' || p == buf || white[-1] == '\n')
        bytes -= start - p - (*p == '\n');

    after_block_comment = start[1] == '*';
}

static void read_source(void)
{
    unsigned size = 0, o = 2;
    int c;

    for (;;) {
        c = getchar();
        if (o >= size)
            buf = realloc(buf, size += 256);
        if (c == EOF)
            break;
        if (trigraph[c] && (buf[o - 1] & buf[o - 2]) == '?') {
            o -= 2;
            extra += 2;
            c = trigraph[c];
        }
        buf[o] = c;
        if (c == '\r') {
            if (flags & COMPAT)
                ++extra;
        } else if (c == '\n' && buf[o - 1] == '\\') {
            /* backslash newline is removed */
            --o;
            ++lines;
            if (flags & COMPAT) {
                extra += 2;
                ++words;
            }
        } else {
            ++o;
        }
    }
    end = buf + o;
    *end = 0;
    cur = buf += 2;
}

int main(int argc, char **argv)
{
    int opt, token, len, trailing;

    init_keywords();

    while ((opt = getopt(argc, argv, "?cikrst")) != -1) {
        switch (opt) {
        case 'c': flags |= COMPAT;   break;
        case 'k': flags |= KEEP;     break;
        case 'r': flags |= RESERVED; break;
        case 's': flags |= SUPPRESS; break;
        case 't': flags |= TOKENS;   break;
        case 'i': flags |= IOCCC | SUPPRESS | RESERVED | COMPAT; break;
        default:
            print_text((unsigned char *)usage_text, -1);
            return 0;
        }
    }

    read_source();

    while ((token = lex())) {
        len = cur - start;
        if (token != COMMENT) {
            bytes += len;
            ++words;
            if ((flags & RESERVED) && token == KEYWORD)
                ioccc_size += 1;
            else if (token < 256 && char_class[token] == BRACE && char_class[*cur] == SPACE)
                ;   /* ; { } followed by whitespace are free */
            else
                ioccc_size += len;
        }
        if (flags & COMPAT) {
            compat(token, len);
        } else if (!(flags & SUPPRESS) && (token != COMMENT || (flags & KEEP))) {
            if (!(flags & TOKENS))
                printf("%d ", token);
            print_text(start, len);
            if (cur[-1] != '\n')
                putchar('\n');
        }
    }

    trailing = end - white;
    if (trailing && (flags & COMPAT))
        compat(0, trailing);

    if (flags & KEEP)
        bytes = end - buf;
    if (!(flags & RESERVED))
        keyword_saved = 0;
    saved = bytes - ioccc_size - keyword_saved;
    bytes += extra;

    if (flags & TOKENS)
        return 0;
    if (flags & IOCCC)
        printf("%d\n", ioccc_size);
    else
        fprintf(stderr, "%d %d %d %d %d %d %d\n", lines, words, bytes,
                ioccc_size, saved, keyword_count, keyword_saved);
    return 0;
}
