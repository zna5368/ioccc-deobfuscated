#include <stdio.h> /******** SpigotQuine -- usage: ./endoh2 [pi or e] ********/

/*
 * This program prints a C program (the "child") shaped like the Greek letter
 * pi (or like the letter e).  The child is a quasi-quine: it computes some
 * digits of pi (or e) and prints its own source code drawn as ASCII art of
 * those digits, with one more digit each generation.
 *
 * The child's source is kept here as plain text.  The child stores that same
 * text in its string variable p and uses it as a printf() format string, so
 * every '%' that the child's code really contains is written here as "%%",
 * and the placeholders are:
 *   %s  -> the text itself (this is what makes the program a quine)
 *   %c  -> '"' (the closing quote of p's string literal)
 *   %c  -> ' ' (the space in "int main")
 *   %d  -> n, the number of digits the next generation computes
 */

/*
 * 3x5 bitmap font of the digits 0-9 packed into 20 characters (after printf()
 * turns "%%" into "%").  The 15-bit glyph of digit n is
 *     275 * font[n + 10] - 8 * font[n] - 8
 */
static const char FONT_TABLE[] = "G1%%xJ{;Q7wunmuGuu%%uu";

/* Start of the child program: headers, globals, and start of main(). */
static const char CHILD_HEAD[] =
    "#include<stdio.h>/*Spigot_Quine*/"
    "#include<stdlib.h>/*_IOCCC2012_*/"
    "int*e,i,j,k,n;char*q,*a,*d,*z,*p=%s%c;"
    "int%cmain(){"
    "a=calloc(1,1e4+n*2);;"
    "for(*a=";

/* Spigot loop that writes the decimal digits of pi into a[], one per byte. */
static const char CHILD_PI_DIGITS[] =
    "3,z=d=a+n+1,j=n*8-7;k=0,j-1;j-=2){"
    "for(a[1]+=2;--z-a;*z=k%%10,k/=10)k+=j/2**z;;"
    "for(;k=k%%j*10+*++z,z<d;*z=k/j);;";

/* Spigot loop that writes the decimal digits of e into a[], one per byte. */
static const char CHILD_E_DIGITS[] =
    "2,z=d=a+n*2,*z=1,j=0;++j<n;){"
    "for(;k=k%%j*10+*z,a-z;*z--=k/j)a++;"
    "for(k=0;z-d;*a--=k%%10,k/=10)k+=*++z+*a";

/*
 * End of the child program: it expands its own text with sprintf(), then
 * draws every digit in a[] (20 lines per digit) with the font table, filling
 * the "on" pixels with characters of its source code and inserting string
 * quotes as needed.  The text ends by opening a comment that hides the
 * leftover text.
 */
static const char CHILD_TAIL[] =
    ";}"
    "d+=sprintf(q=d-20,p,p,34,32,n+1)+2;;;;"
    "for(n=n*20-400;k<n;++k%%n?j=+puts(d)<0:(d++[j]=47,d[j-2]=42),"
    "k%%20<1?puts(d-1),a++:0){"
    "for(i=-1;i++<32;!*z?q[662]=0,z=q+207:*z+z[1]<65?z+=11:*z==34?p=0:0)"
    "d[i]=((k/20-1?275*q[*a+10]-8*q[*a+0]-8:128)>>(i/11+k/4%%5*3))&1"
    "?k/3*!j&&p?j=34:(j=i+1,*z++):32;"
    "k/3*j--&&p?d[z--,j]=34:0;"
    "}}"
    "int*y,n=%d;/*..~";

/* The first child computes and draws 4 glyphs: n - 20 = 4 ("3.14"/"2.71"). */
#define FIRST_CHILD_DIGITS 24

/* Each output line holds 65 characters plus a newline. */
#define LINE_WIDTH 65

/*
 * Shapes of the child program as runs of characters, read left to right and
 * top to bottom:  a positive number copies that many characters of the
 * program text, a negative number prints that many spaces.
 *
 * The first `quoted_runs` runs lie inside the big string literal p, so after
 * each of them a '"' is printed to close or reopen the literal (and when that
 * quote ends a line, the next line begins with another '"').
 */
static const int PI_SHAPE[] = {
    194, 63, 19, -26, 16, 17, -28, 16, 16, -5, 2, -2, 6, -4, 22, 16, -3,
    4, -2, 6, -4, 22, 16, -2, 5, -2, 6, -4, 22, 25, -2, 6, -4, 22, 25,
    -2, 6, -4, 22, 24, -3, 6, -4, 22, 24, -3, 6, -4, 22, 24, -2, 7, -4,
    22, 23, -3, 7, -4,
    /* the rest is outside the string literal: no quotes */
    42, -3, 1, -5, 9, -4, 42, -9, 10, -6, 40, -8, 11, -10, 37, -6, 13, -8,
    279, 0};
#define PI_QUOTED_RUNS 58

static const int E_SHAPE[] = {
    194, 63, 26, -11, 24, 24, -5, 2, -6, 22, 22, -5, 6, -6, 20, 21, -5, 8,
    -6, 19, 21, -4, 10, -5, 19, 20, -5, 10, -5, 19, 20, -22, 19, 20, -5,
    36, 20, -5, 36, 20, -5, 36,
    /* the rest is outside the string literal: no quotes */
    22, -5, 15, -1, 44, -6, 13, -2, 45, -7, 9, -3, 49, -14, 54, -9, 221,
    0};
#define E_QUOTED_RUNS 42

/* Append a newline to the output whenever the current line is full. */
static char *wrap_line(char *out, const char *start) {
  if ((out - start) % (LINE_WIDTH + 1) == 0)
    *out++ = '\n';
  return out;
}

int main(int argc, char **argv) {
  char text[2012];   /* the child program as one long line */
  char source[2012]; /* the expanded source, later the drawn shape */
  int want_e = argc > 1 && argv[1][0] == 'e';
  const int *shape = want_e ? E_SHAPE : PI_SHAPE;
  int quoted_runs = want_e ? E_QUOTED_RUNS : PI_QUOTED_RUNS;
  const char *in;
  char *out;
  int run, count;

  /* The child's string p: font table followed by the program text. */
  sprintf(text, "%s%s%s%s", FONT_TABLE, CHILD_HEAD,
          want_e ? CHILD_E_DIGITS : CHILD_PI_DIGITS, CHILD_TAIL);

  /* Expand the program text using itself as the value of p. */
  sprintf(source, text + sizeof(FONT_TABLE) - 1, text, '"', ' ',
          FIRST_CHILD_DIGITS);

  /*
   * The two #include lines (33 characters each) are padded to the full line
   * width with comments holding filler copied from the declarations, and the
   * opening quote of p's string literal is inserted after "p=".  A final
   * comment terminator closes the comment at the end of the program.
   */
  {
    const char *include_stdio = source;
    const char *include_stdlib = source + 33;
    const char *declarations = source + 66;
    const char *string_body = source + 99;
    sprintf(text, "%.33s/*%.28s*/%.33s/*%.28s*/%.33s\"%s*/", include_stdio,
            declarations, include_stdlib, declarations + 10, declarations,
            string_body);
  }

  /* Pour the text into the shape. */
  in = text;
  out = source;
  for (; (run = *shape) != 0; shape++, quoted_runs--) {
    for (count = 0; count < (run > 0 ? run : -run); count++) {
      out = wrap_line(out, source);
      *out++ = run > 0 ? *in++ : ' ';
    }
    if (quoted_runs > 0) {
      *out++ = '"';
      if ((out - source) % (LINE_WIDTH + 1) == 0) {
        *out++ = '\n';
        *out++ = '"';
      }
    }
  }
  *out = 0;

  /* Skip the newline written before the first line. */
  puts(source + 1);
}
