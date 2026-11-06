/*
 * Backtrace quine.
 *
 * The program pushes the full text of prog.c onto a stack of characters and
 * then pops it one character at a time.  Every character c has a handler
 * function named x<hex of c> whose call statement sits on source line c
 * (forced with #line).  Each handler pops the next character and calls that
 * character's handler, so the call chain grows one frame per character.
 * The very first character is preceded by a 0, whose handler is NULL, so the
 * program crashes there.  Reading the line numbers of the debugger's
 * backtrace as ASCII codes spells out prog.c.
 */

/*
 * Compressed description of prog.c.  It is also printed verbatim, as the
 * string literal of prog.c.  Inside a segment '!' stands for a newline and
 * "!!" marks the end of the segment.
 */
char *source_data =
    "o x" "!!"                                  /* handler name prefix */
    "(){f[*r--]();}" "!!"                       /* handler body */
    "#define o void/*!r!bt!q!*/!char*d=" "!!"   /* lines 1-6 of prog.c */
    "o(*f[])()={b" "!!"                         /* start of the table */
    /* the C code that follows the string literal in prog.c */
    ",*q;o(*f[128])();int*r,t[9999],*h=t+127,n;"
    "o x0a(){f[*r--]();}"
    "int main(){f[0]();f[59]();}"
    "o v(int x!){for(q=d+x;*q+q[1]-66;q++)*++h=*q-33?*q:10;}"
    "o a(int x){*++r=*++h=x+48+x/10*39;}"
    "o b(){*h=1;r=2096+t;v(21);"
    "for(*++h=34,q=!d;*q;t[*++h=*q++]=1){(q-d+8)%119?0:(t[*++h=92]=1,t[*++h=10]=1);}"
    "for(v(t[*++h=34]=71);++n/126-1;n/13?*++h=10:0)"
    "{*++r=44,*!++r=48,t[n]?*r+=72,v(0),a(n/16),a(n%16),v(5),h-=n-10?0:19:h;}"
    "v(57);*++r=125;}"
    "!!";

/* Offsets of the segments inside source_data */
#define HANDLER_NAME   0    /* "o x" */
#define HANDLER_BODY   5    /* "(){f[*r--]();}" */
#define HEADER        21    /* "#define o void/" "*\nr\nbt\nq\n*" "/\nchar*d=" */
#define TABLE_START   57    /* "o(*f[])()={b" */
#define CODE          71    /* ",*q;o(*f[128])();..." */

/* The string literal in prog.c starts after the 8 characters of: char*d=" */
#define LITERAL_COLUMN   8
/* and is wrapped with a backslash-newline every 119 characters */
#define LINE_WIDTH     119

/* Highest line of prog.c that holds handler definitions */
#define LAST_HANDLER_LINE 125
/* Lines 1 to 13 of prog.c hold the header, the string and the code */
#define FIRST_HANDLER_LINE 13

/* char_handler[c] is the function for character c (NULL if c is unused) */
void (*char_handler[128])(void);

/* Stack of characters of prog.c, first character at the bottom */
int char_stack[4096];
int *char_stack_top;

/* Characters that appear in prog.c and therefore need a handler */
int char_is_used[128];

void push(int c)
{
    *++char_stack_top = c;
}

/* Push one "!!"-terminated segment of source_data, turning '!' into '\n' */
void push_segment(int offset)
{
    char *p;

    for (p = source_data + offset; !(p[0] == '!' && p[1] == '!'); p++) {
        if (*p == '!')
            push('\n');
        else
            push(*p);
    }
}

void push_hex_digit(int digit)
{
    push("0123456789abcdef"[digit]);
}

/* Push the whole text of prog.c except for its final ";\n" */
void push_source_text(void)
{
    int i, c;

    /* below the first character: 0 has no handler, which causes the crash */
    char_stack_top = char_stack;
    *char_stack_top = 0;

    /* lines 1-6: the #define, the gdb commands, and "char*d=" */
    push_segment(HEADER);

    /* the string literal, wrapped every LINE_WIDTH columns */
    push('"');
    char_is_used['"'] = 1;
    for (i = 0; source_data[i] != '\0'; i++) {
        if ((i + LITERAL_COLUMN) % LINE_WIDTH == 0) {
            push('\\');
            push('\n');
            char_is_used['\\'] = 1;
            char_is_used['\n'] = 1;
        }
        push(source_data[i]);
        char_is_used[(int)source_data[i]] = 1;
    }
    push('"');

    /* the C code of lines 10-13 */
    push_segment(CODE);

    /*
     * lines 14-125: line c defines the handler for character c if it is
     * used (the handler for '\n' is already defined in the code on line 10)
     */
    for (c = 1; c <= LAST_HANDLER_LINE; c++) {
        if (char_is_used[c] && c != '\n') {
            push_segment(HANDLER_NAME);
            push_hex_digit(c / 16);
            push_hex_digit(c % 16);
            push_segment(HANDLER_BODY);
        }
        if (c >= FIRST_HANDLER_LINE)
            push('\n');
    }

    /* line 126: the handler table "o(*f[])()={b,0,...,x0a,...,x7d}" */
    push_segment(TABLE_START);
    for (c = 1; c <= LAST_HANDLER_LINE; c++) {
        push(',');
        if (char_is_used[c]) {
            push('x');
            push_hex_digit(c / 16);
            push_hex_digit(c % 16);
        } else {
            push('0');
        }
    }
    push('}');
}

/*
 * Character handlers.  Each one pops the next character off the stack and
 * calls its handler.  The #line before each one makes the debugger report
 * the line number equal to the character code.
 */
#line 10
void x0a(void) { char_handler[*char_stack_top--](); }  /* '\n' */
#line 32
void x20(void) { char_handler[*char_stack_top--](); }  /* ' ' */
#line 33
void x21(void) { char_handler[*char_stack_top--](); }  /* '!' */
#line 34
void x22(void) { char_handler[*char_stack_top--](); }  /* '"' */
#line 35
void x23(void) { char_handler[*char_stack_top--](); }  /* '#' */
#line 37
void x25(void) { char_handler[*char_stack_top--](); }  /* '%' */
#line 40
void x28(void) { char_handler[*char_stack_top--](); }  /* '(' */
#line 41
void x29(void) { char_handler[*char_stack_top--](); }  /* ')' */
#line 42
void x2a(void) { char_handler[*char_stack_top--](); }  /* '*' */
#line 43
void x2b(void) { char_handler[*char_stack_top--](); }  /* '+' */
#line 44
void x2c(void) { char_handler[*char_stack_top--](); }  /* ',' */
#line 45
void x2d(void) { char_handler[*char_stack_top--](); }  /* '-' */
#line 47
void x2f(void) { char_handler[*char_stack_top--](); }  /* '/' */
#line 48
void x30(void) { char_handler[*char_stack_top--](); }  /* '0' */
#line 49
void x31(void) { char_handler[*char_stack_top--](); }  /* '1' */
#line 50
void x32(void) { char_handler[*char_stack_top--](); }  /* '2' */
#line 51
void x33(void) { char_handler[*char_stack_top--](); }  /* '3' */
#line 52
void x34(void) { char_handler[*char_stack_top--](); }  /* '4' */
#line 53
void x35(void) { char_handler[*char_stack_top--](); }  /* '5' */
#line 54
void x36(void) { char_handler[*char_stack_top--](); }  /* '6' */
#line 55
void x37(void) { char_handler[*char_stack_top--](); }  /* '7' */
#line 56
void x38(void) { char_handler[*char_stack_top--](); }  /* '8' */
#line 57
void x39(void) { char_handler[*char_stack_top--](); }  /* '9' */
#line 58
void x3a(void) { char_handler[*char_stack_top--](); }  /* ':' */
#line 59
void x3b(void) { char_handler[*char_stack_top--](); }  /* ';' */
#line 61
void x3d(void) { char_handler[*char_stack_top--](); }  /* '=' */
#line 63
void x3f(void) { char_handler[*char_stack_top--](); }  /* '?' */
#line 91
void x5b(void) { char_handler[*char_stack_top--](); }  /* '[' */
#line 92
void x5c(void) { char_handler[*char_stack_top--](); }  /* '\\' */
#line 93
void x5d(void) { char_handler[*char_stack_top--](); }  /* ']' */
#line 97
void x61(void) { char_handler[*char_stack_top--](); }  /* 'a' */
#line 98
void x62(void) { char_handler[*char_stack_top--](); }  /* 'b' */
#line 99
void x63(void) { char_handler[*char_stack_top--](); }  /* 'c' */
#line 100
void x64(void) { char_handler[*char_stack_top--](); }  /* 'd' */
#line 101
void x65(void) { char_handler[*char_stack_top--](); }  /* 'e' */
#line 102
void x66(void) { char_handler[*char_stack_top--](); }  /* 'f' */
#line 104
void x68(void) { char_handler[*char_stack_top--](); }  /* 'h' */
#line 105
void x69(void) { char_handler[*char_stack_top--](); }  /* 'i' */
#line 109
void x6d(void) { char_handler[*char_stack_top--](); }  /* 'm' */
#line 110
void x6e(void) { char_handler[*char_stack_top--](); }  /* 'n' */
#line 111
void x6f(void) { char_handler[*char_stack_top--](); }  /* 'o' */
#line 113
void x71(void) { char_handler[*char_stack_top--](); }  /* 'q' */
#line 114
void x72(void) { char_handler[*char_stack_top--](); }  /* 'r' */
#line 116
void x74(void) { char_handler[*char_stack_top--](); }  /* 't' */
#line 118
void x76(void) { char_handler[*char_stack_top--](); }  /* 'v' */
#line 120
void x78(void) { char_handler[*char_stack_top--](); }  /* 'x' */
#line 123
void x7b(void) { char_handler[*char_stack_top--](); }  /* '{' */
#line 125
void x7d(void) { char_handler[*char_stack_top--](); }  /* '}' */

void (*char_handler[128])(void) = {
    ['\n'] = x0a, [' '] = x20, ['!'] = x21, ['"'] = x22, ['#'] = x23,
    ['%'] = x25, ['('] = x28, [')'] = x29, ['*'] = x2a, ['+'] = x2b,
    [','] = x2c, ['-'] = x2d, ['/'] = x2f, ['0'] = x30, ['1'] = x31,
    ['2'] = x32, ['3'] = x33, ['4'] = x34, ['5'] = x35, ['6'] = x36,
    ['7'] = x37, ['8'] = x38, ['9'] = x39, [':'] = x3a, [';'] = x3b,
    ['='] = x3d, ['?'] = x3f, ['['] = x5b, ['\\'] = x5c, [']'] = x5d,
    ['a'] = x61, ['b'] = x62, ['c'] = x63, ['d'] = x64, ['e'] = x65,
    ['f'] = x66, ['h'] = x68, ['i'] = x69, ['m'] = x6d, ['n'] = x6e,
    ['o'] = x6f, ['q'] = x71, ['r'] = x72, ['t'] = x74, ['v'] = x76,
    ['x'] = x78, ['{'] = x7b, ['}'] = x7d
};

/* main is the last two characters: line 10 is '\n' and the ';' comes next */
int main(void)
{
    push_source_text();
#line 10
    char_handler[';']();
}
