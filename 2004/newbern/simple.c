#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * A glyph is a linked list of rows.  Each row is a string of '#' and ' '
 * pixels that is printed on its own line.
 */
struct row {
  struct row *next;
  char *pixels;
};

#define FONT_SIZE 256
#define MAX_COLUMNS 200 /* encoder wraps lines that are this wide */
#define BITS_PER_CHAR 6 /* pixels stored in one encoded character */
#define ENCODE_BASE '+' /* encoded character for six blank pixels */

static struct row **font; /* font[c] is the glyph for character c */

/* Built-in Morse code "font": one row per character, dot '#', dash '###' */
static const char *morse_letters[26] = {
    "# ###",           /* A */
    "### # # #",       /* B */
    "### # ### #",     /* C */
    "### # #",         /* D */
    "#",               /* E */
    "# # ### #",       /* F */
    "### ### #",       /* G */
    "# # # #",         /* H */
    "# #",             /* I */
    "# ### ### ###",   /* J */
    "### # ###",       /* K */
    "# ### # #",       /* L */
    "### ###",         /* M */
    "### #",           /* N */
    "### ### ###",     /* O */
    "# ### ### #",     /* P */
    "### ### # ###",   /* Q */
    "# ### #",         /* R */
    "# # #",           /* S */
    "###",             /* T */
    "# # ###",         /* U */
    "# # # ###",       /* V */
    "# ### ###",       /* W */
    "### # # ###",     /* X */
    "### # ### ##",    /* Y */
    "### ### # #",     /* Z */
};

static const char *morse_digits[10] = {
    "### ### ### ### ###", /* 0 */
    "# ### ### ### ###",   /* 1 */
    "# # ### ### ###",     /* 2 */
    "# # # ### ###",       /* 3 */
    "# # # # ###",         /* 4 */
    "# # # # #",           /* 5 */
    "### # # # #",         /* 6 */
    "### ### # #",         /* 7 */
    "### ### ### # #",     /* 8 */
    "### ### ### ### #",   /* 9 */
};

static void *xmalloc(size_t size) {
  void *p = malloc(size);
  if (!p)
    exit(1);
  return p;
}

static struct row **new_font(void) {
  struct row **f = calloc(FONT_SIZE, sizeof(struct row *));
  if (!f)
    exit(1);
  return f;
}

/* append a row of pixels to the end of the glyph for character c */
static void add_row(unsigned char c, char *pixels) {
  struct row **tail = &font[c];
  struct row *r = xmalloc(sizeof(struct row));
  r->next = 0;
  r->pixels = pixels;
  while (*tail)
    tail = &(*tail)->next;
  *tail = r;
}

/*
 * Decode an encoded row: each character minus '+' holds six pixels, the
 * lowest bit being the leftmost pixel.  Blank pixels at the end of the
 * last character are not printed.
 */
static char *decode_row(const char *code, size_t len) {
  char *pixels = xmalloc(len * BITS_PER_CHAR + 1);
  size_t i, out = 0;
  for (i = 0; i < len && code[i] >= ENCODE_BASE; i++) {
    int bits = code[i] - ENCODE_BASE;
    int last = (i + 1 == len) || code[i + 1] < ENCODE_BASE;
    int col;
    for (col = 0; col < BITS_PER_CHAR; col++, bits >>= 1) {
      if (last && bits == 0)
        break;
      pixels[out++] = (bits & 1) ? '#' : ' ';
    }
  }
  pixels[out] = '\0';
  return pixels;
}

/*
 * A font line is a key character followed by whitespace separated encoded
 * rows.  The first row is added to the key character, the next to the
 * character after it, and so on.
 */
static void load_font(const char *filename) {
  char line[256];
  FILE *file;

  font = new_font();
  if (!(file = fopen(filename, "r")))
    exit(1);
  while (fgets(line, sizeof(line), file)) {
    unsigned char key = line[0];
    char *word = line + 1;
    while (*word) {
      size_t len = 0;
      while (word[len] && !isspace(word[len]))
        len++;
      add_row(key++, decode_row(word, len));
      word += len;
      while (*word && isspace(*word))
        word++;
    }
  }
  fclose(file);
}

static void load_morse_font(void) {
  int i;
  font = new_font();
  for (i = 0; i < 26; i++) {
    add_row('a' + i, (char *)morse_letters[i]);
    add_row('A' + i, (char *)morse_letters[i]);
  }
  for (i = 0; i < 10; i++)
    add_row('0' + i, (char *)morse_digits[i]);
  add_row('.', "# ### # ### # ###");
  add_row('?', "# # ### ### # #");
  add_row(',', "### ### # # ### ###");
  add_row(' ', "");
}

static void print_glyph(unsigned char c) {
  struct row *r;
  for (r = font[c]; r; r = r->next) {
    fputs(r->pixels, stdout);
    putchar('\n');
  }
}

/* "encryption": every glyph moves down one character, wrapping around */
static void rotate_font(void) {
  struct row *first = font[0];
  int i;
  for (i = 0; i < FONT_SIZE - 1; i++)
    font[i] = font[i + 1];
  font[FONT_SIZE - 1] = first;
}

/* read standard input and print each character as a line of Morse code */
static void morse(void) {
  int c, first = 1;
  load_morse_font();
  while ((c = getchar()) != EOF) {
    if (!first)
      putchar('\n');
    first = 0;
    print_glyph(c);
  }
}

/* convert a picture on standard input into lines for a font file */
static void encode_glyph(char key) {
  char bits[MAX_COLUMNS / BITS_PER_CHAR + 1] = {0};
  int column = 0, c, i;

  for (;;) {
    c = (column == MAX_COLUMNS) ? '\n' : getchar();
    if (c == '\n' || c == EOF) {
      if (column > 0) {
        putchar(key);
        for (i = 0; i < column; i += BITS_PER_CHAR) {
          putchar(bits[i / BITS_PER_CHAR] + ENCODE_BASE);
          bits[i / BITS_PER_CHAR] = 0;
        }
        putchar('\n');
      }
      if (c == EOF)
        exit(0);
      column = 0;
      continue;
    }
    if (c != ' ')
      bits[column / BITS_PER_CHAR] |= 1 << (column % BITS_PER_CHAR);
    column++;
  }
}

/* print each character of text using the font, optionally "encrypted" */
static void banner(const char *text, int encrypt) {
  while (*text) {
    if (encrypt)
      rotate_font();
    print_glyph(*text++);
    if (*text)
      putchar('\n');
  }
}

int main(int argc, char **argv) {
  switch (argc) {
  case 1:
    morse();
    break;
  case 2:
    encode_glyph(argv[1][0]);
    break;
  case 3:
    load_font(argv[1]);
    banner(argv[2], 0);
    break;
  case 4:
    load_font(argv[1]);
    banner(argv[2], 1);
    break;
  default:
    banner(argv[2], 0); /* no font was loaded */
    break;
  }
  exit(0);
}
