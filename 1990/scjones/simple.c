#include <stdio.h>
#include <stdlib.h>

/*
 * A quine: the program prints its own (trigraph-encoded) source code.
 *
 * source_lines holds every line of the program's code section with the
 * trigraphs already decoded into the real characters they stand for.
 * The program prints this table twice:
 *   1. as the elements of a C string array literal
 *   2. as raw program code
 * Every character that has a trigraph spelling is re-encoded on output.
 */
static const char *source_lines[] = {
    "#include<stdio.h>",
    "#include<stdlib.h>",
    "#define o stdout",
    "#define b break;case",
    "#define s(p)fputs(p,o);",
    "#define c(p)fputc(p,o);",
    "void t(p,f)char*p;{f&&c('\"')",
    "for(;;p++){switch(*p){case 0:f&&",
    "s(\"\\\",\")c('\\n')return;case",
    "'#':s(\"?\\?=\")b'{':s(\"?\\?<\")",
    "b'}':s(\"?\\?>\")b'[':s(\"?\\?(\")b']'",
    ":s(\"?\\?)\")b'\\\\':f&&s(\"?\\?/\")",
    "s(\"?\\?/\")b'\\n':if(f)s(\"?\\?/n\")",
    "else case'\"':if(f)s(\"?\\?/\\\"\")",
    "else default:c(*p)}}}main(){char**p",
    ";t(\"char*a[]={\",0);for(p=a;*p",
    ";p++)t(*p,1);t(\"0};\",0);for(p=a",
    ";*p;p++)t(*p,0);exit(!ferror(o)&&",
    "!fclose(o)?EXIT_SUCCESS",
    ":EXIT_FAILURE);",
    "/*NOTREACHED*/",
    "}",
    NULL,
};

/* Build a trigraph: two question marks (split by \? so the compiler does
 * not translate it) followed by the given character. */
#define TRIGRAPH(ch) "?\?" ch

/*
 * Print one line followed by a newline.  When as_string is set, the line is
 * printed as a quoted string literal followed by a comma, so backslashes,
 * quotes and newlines are escaped as well.  Escape backslashes are themselves
 * written as the backslash trigraph.
 */
static void print_line(const char *line, int as_string)
{
    const char *p;

    if (as_string)
        fputc('"', stdout);

    for (p = line; *p != '\0'; p++) {
        switch (*p) {
        case '#':
            fputs(TRIGRAPH("="), stdout);
            break;
        case '{':
            fputs(TRIGRAPH("<"), stdout);
            break;
        case '}':
            fputs(TRIGRAPH(">"), stdout);
            break;
        case '[':
            fputs(TRIGRAPH("("), stdout);
            break;
        case ']':
            fputs(TRIGRAPH(")"), stdout);
            break;
        case '\\':
            if (as_string)
                fputs(TRIGRAPH("/"), stdout); /* escaping backslash */
            fputs(TRIGRAPH("/"), stdout);
            break;
        case '\n':
            if (as_string)
                fputs(TRIGRAPH("/") "n", stdout);
            else
                fputc(*p, stdout);
            break;
        case '"':
            if (as_string)
                fputs(TRIGRAPH("/") "\"", stdout);
            else
                fputc(*p, stdout);
            break;
        default:
            fputc(*p, stdout);
            break;
        }
    }

    if (as_string)
        fputs("\",", stdout);
    fputc('\n', stdout);
}

int main()
{
    const char **line;

    /* First half: the source lines as an array of string literals. */
    print_line("char*a[]={", 0);
    for (line = source_lines; *line != NULL; line++)
        print_line(*line, 1);
    print_line("0};", 0);

    /* Second half: the same lines as plain program code. */
    for (line = source_lines; *line != NULL; line++)
        print_line(*line, 0);

    exit(!ferror(stdout) && !fclose(stdout) ? EXIT_SUCCESS : EXIT_FAILURE);
    /*NOTREACHED*/
}
