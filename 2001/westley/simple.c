#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * In the original every one of these choices was a call to t(a,b), which
 * returns its first argument. The line "#define t(a,b) t(b,a)" swaps the
 * arguments of every call that comes after it in the source, so the answer
 * of each call depends on the order of the lines. These are the answers for
 * the line order of the original westley.c.
 */
#define REVERSE_LINES 0   /* J(): t(i,0) came before the #define */
#define PRINT_AS_TEXT 1   /* M(): t(1,0) came before the #define */
#define REARRANGE_LINES 1 /* E(): t(0,1) came after the #define */
#define SCRAMBLE_LINES 1  /* F(): t(0,1) came after the #define */

#define MAX_LINES 81
#define LINE_SIZE 81
#define CARD_COLUMNS 80
#define CARD_ROWS 12

/*
 * EBCDIC punch card codes. For the character at position k of card_chars,
 * zone_punch[k], digit_punch[k] and extra_punch[k] name up to three rows that
 * are punched. Row names are listed in row_names: 'y' is row 12, 'z' is
 * row 11 and '0'..'9' are rows 0 to 9. A space means no punch.
 */
char *card_chars =
    "gh$j|lm'pq&rv<x>z#);@_winky,.%/+*!=3-284106579 (:obfuscated?";
char *zone_punch =
    "yyzzyzz zz z0y000 zz 00yzz00y00yzz Brian Westley zyy00yy0yy 0";
char *digit_punch =
    "7831734578 9547693564569528334164263z284106579 52626423135477";
char *extra_punch =
    "008y8yy8yyyyz8z8z88888z0yyz888 8888            88y00zz00z0088";

/* the rows of a card from top to bottom; index 0 means "not punched" */
char *row_names = " yz0123456789";

/* position of c in the string s or -1 if it is not there */
int find_char(int c, char *s) {
  int i;

  for (i = 0; s[i] && s[i] != c; i++)
    ;
  return s[i] ? i : -1;
}

/* is card row number row (1..12) punched for character c */
int is_punched(int c, int row) {
  int k = find_char(tolower(c), card_chars);

  /* unknown characters are punched in every row (a "lace" column) */
  if (k < 0)
    return 1;

  /* lower case letters get an extra zero punch that upper case do not */
  return row == find_char(zone_punch[k], row_names) ||
         row == find_char(digit_punch[k], row_names) ||
         row == (isupper(c) ? 0 : find_char(extra_punch[k], row_names));
}

/* draw one row of holes of a card */
void print_card_row(char *line, int row) {
  int column;

  printf("|");
  for (column = 0; line[column]; column++)
    putchar(is_punched(line[column], row) ? '[' : ' ');
  while (column++ < CARD_COLUMNS)
    printf(" ");
  puts("|");
}

/* draw the top or bottom edge of a card */
void print_card_edge(int top) {
  int column;

  printf(top ? " " : "|");
  for (column = 0; column < CARD_COLUMNS; column++)
    printf("_");
  puts(top ? "_" : "|");
}

/* draw a punched card for one line of text */
void print_card(char *line) {
  int row;

  print_card_edge(1);
  printf("/%-80s|\n", line);
  for (row = 1; row <= CARD_ROWS; row++)
    print_card_row(line, row);
  print_card_edge(0);
}

/* a random comparison: negative about half of the time */
int random_compare(void) {
  static int seeded = 0;

  if (!seeded) {
    seeded = 1;
    srand(time(NULL));
  }
  return rand() - rand();
}

int compare_lines(char *a, char *b) {
  return SCRAMBLE_LINES ? random_compare() : strcmp(a, b);
}

void swap_lines(char *a, char *b) {
  char temp[LINE_SIZE];

  if (a == b)
    return;
  strcpy(temp, a);
  strcpy(a, b);
  strcpy(b, temp);
}

/* sort the lines into descending order or scramble them */
void rearrange_lines(char lines[][LINE_SIZE], int count) {
  int j, k;

  for (j = 0; j < count; j++)
    for (k = j; k < count; k++)
      if (REARRANGE_LINES && compare_lines(lines[j], lines[k]) < 0)
        swap_lines(lines[j], lines[k]);
}

void reverse_lines(char lines[][LINE_SIZE], int count) {
  int j;

  for (j = 0; j + j < count - 1; j++)
    swap_lines(lines[j], lines[count - 1 - j]);
}

/* print the lines from last to first */
void print_lines(char lines[][LINE_SIZE], int count) {
  while (count--) {
    if (PRINT_AS_TEXT)
      puts(lines[count]);
    else
      print_card(lines[count]);
  }
}

int main(void) {
  char lines[MAX_LINES][LINE_SIZE];
  int count;

  for (count = 0; gets(lines[count]); count++)
    ;

  rearrange_lines(lines, count);
  if (REVERSE_LINES)
    reverse_lines(lines, count);
  print_lines(lines, count);
  return 0;
}
