#include <stdio.h>

/*
 * The music font is hidden in the whitespace of the source file that is
 * passed as the first argument.  Starting at character 99, every group of
 * 8 characters is one byte of bitmap data: a space is a 1 bit, anything else
 * is a 0 bit (most significant bit first).
 */
#define FONT_START 99
#define FONT_BYTES 491

/* Offsets of the glyphs inside the decoded font. Most glyphs are stored in a
 * 7 byte wide sheet, the bass clef and the accidentals in 3 byte wide sheets */
#define BASS_CLEF 0        /* 3 bytes wide, staff rows 19..41              */
#define TREBLE_CLEF 75     /* 7 byte rows, staff rows 11..63, inverted     */
#define HEAD_FILLED 78     /* 7 byte rows, 9 rows tall, 2 bytes wide       */
#define HEAD_OPEN 80       /* 7 byte rows, 9 rows tall, 2 bytes wide       */
#define FLAG_UP_STEM 140   /* 7 byte rows, 9 rows tall                     */
#define FLAG_DOWN_STEM 143 /* 7 byte rows, 9 rows tall                     */
#define QUARTER_REST 213   /* 7 byte rows, staff rows 23..44               */
#define EIGHTH_REST 225    /* 7 byte rows, staff rows 27..41               */
#define DIGITS 377         /* 7 byte rows, 10 rows tall, byte n = digit n  */
#define ACCIDENTALS 446    /* 3 bytes wide (sharp, flat, natural), 15 rows */

#define ROWS_PER_STAFF 80
#define FULL_BYTE 255

/* durations are stored as the digit minus '1' */
#define WHOLE 0
#define HALF 1
#define QUARTER 3
#define EIGHTH 7

static unsigned char font[FONT_BYTES];

static void decode_font(const char *source) {
  int i, bit;

  for (i = 0; i < FONT_BYTES; i++) {
    font[i] = 0;
    for (bit = 0; bit < 8; bit++) {
      /* The original decodes in place, so the very first bit is read from
       * the byte that was just cleared and is therefore always 0 */
      int is_space = (i == 0 && bit == 0)
                         ? 0
                         : source[FONT_START + 8 * i + bit] == ' ';
      font[i] = font[i] * 2 + is_space;
    }
  }
}

/* Width in bytes (8 pixel columns) of a token, used to size the image */
static int layout_width(char *token) {
  if (token[0] == ':') /* bar line, wider when another note follows */
    return 2 + (token[3] && token[3] < 'x');
  if (token[0] == 's') /* manual space: convert the amount to a number */
    return token[1] -= '0';
  if (token[0] >= 'A')
    return token[0] == 'm' ? 2 : 3; /* time signature or note/rest */
  return 1;                         /* accidental */
}

/* Number of bytes drawn for a token while printing a row */
static int draw_width(const char *token, int column, int max_width) {
  if (token[0] == 0) /* end of staff: pad up to the longest staff */
    return column ? max_width - column : 0;
  if (token[0] > 'r') /* manual space */
    return token[1];
  return 1 + 2 * (token[0] >= 'A') + (2 - !token[3]) * (token[0] == ':') -
         (token[0] == 'm');
}

/* Vertical position of a note on the staff, one step per line or space.
 * C..B is the lower octave, c..b the upper one, 0 is the highest note. */
static int staff_position(int pitch, int bass_clef) {
  return 105 - pitch - 7 * (pitch < 'c') - 18 * (pitch < 'M') +
         2 * bass_clef - 7 * (pitch < 'C');
}

static int draw_bar(const char *token, int row, int x) {
  if (x != 1 || row <= 17 || row >= 51)
    return 0;
  if (token[1] == ';')
    return 0x27; /* final bar: thin and thick line */
  if (token[1] == ':')
    return 0x09; /* double bar */
  return 0x01;   /* single bar */
}

static int draw_accidental(const char *token, int dy) {
  int kind = token[0] < '-' ? 0 : token[0] < '=' ? 1 : 2;

  if (dy < 7 && dy > -9)
    return font[ACCIDENTALS + 3 * (dy + 8) + kind];
  return 0;
}

static int draw_rest(int duration, int row, int x) {
  if ((row > 26 && row < 29 && duration == WHOLE) ||
      (duration == HALF && row < 34 && row > 31))
    return x == 0 ? 0x0F : x == 1 ? 0xF0 : 0; /* filled block */
  if (duration == QUARTER)
    return x < 2 && row > 22 && row < 45
               ? font[QUARTER_REST + 7 * (row - 23) + x]
               : 0;
  if (duration == EIGHTH && row > 26 && row < 42 && x < 2)
    return font[EIGHTH_REST + 7 * (row - 27) + x];
  return 0;
}

static int draw_note(const char *token, int duration, int position, int row,
                     int dy, int x) {
  int pixels = 0;
  int stem_down = position < 8;

  /* note head and the dot of a dotted note */
  if (dy > -5 && dy < 5) {
    if (x < 2)
      pixels |=
          font[(duration < QUARTER ? HEAD_OPEN : HEAD_FILLED) + 7 * (dy + 4) + x];
    else if (token[2] == '.' && (dy == -2 || dy == -1 - 2 * (position % 2)))
      pixels |= 0x60;
  }

  /* stem: left side going down, or right side going up */
  if (duration != WHOLE) {
    if (x == 0) {
      if (stem_down && dy > 1 && dy < 24)
        pixels |= 0x20;
    } else if (x < 2 && !stem_down && dy < 1 && dy > -24)
      pixels |= 0x08;
  }

  /* ledger lines below and above the staff */
  if (x < 2 && ((row == 66 && position > 14) || (position > 12 && row == 58) ||
                (position < 2 && row == 10)))
    pixels |= FULL_BYTE;

  /* flag of an eighth note at the end of the stem */
  if (duration == EIGHTH) {
    if (stem_down) {
      if (dy > 13 && dy < 23 && x < 2)
        pixels |= font[FLAG_DOWN_STEM + 7 * (dy - 14) + x];
    } else if (x && dy < -14 && dy > -24)
      pixels |= font[FLAG_UP_STEM + 7 * (dy + 23) + x];
  }
  return pixels;
}

static int draw_time_signature(const char *token, int row, int x) {
  if (x != 0)
    return 0;
  if (row > 21 && row < 32) /* numerator */
    return font[DIGITS + 7 * (row - 22) + token[1] - '0'];
  if (row > 37 && row < 48) /* denominator */
    return font[DIGITS + 7 * (row - 38) + token[2] - '0'];
  return 0;
}

/* One byte (8 pixels) of the symbol for a token, x is the byte inside it */
static int draw_token(const char *token, int duration, int position, int row,
                      int dy, int x) {
  if (!token[0] || token[0] >= 's') /* end of staff, space */
    return 0;
  if (token[0] == ':')
    return draw_bar(token, row, x);
  if (token[0] < '?') /* + - = */
    return draw_accidental(token, dy);
  if (token[0] > 'o') /* r */
    return draw_rest(duration, row, x);
  if (token[0] < 'k') /* A-G a-g */
    return draw_note(token, duration, position, row, dy, x);
  return draw_time_signature(token, row, x); /* m */
}

int main(int argc, char *argv[]) {
  char *music = argv[2];
  char *token;
  int bass_clef = 0;
  int staves = 1, width = 0, max_width = 0;
  int row, k, x, column, start, count = 0;

  (void)argc;
  printf("P4");
  decode_font(argv[1]);

  if (music[0] == 'K') { /* clef selection, KF means bass clef */
    music += 3;
    bass_clef = music[-2] == 'F';
  }

  /* first pass: count staves and find the widest one */
  for (token = music; token[0] && token[1] && token[2]; token += 3) {
    if (token[0] > 'w') { /* x: new staff */
      width = 0;
      staves++;
      token[0] = 0;
    } else
      width += layout_width(token);
    if (width > max_width)
      max_width = width;
  }
  printf("  %d          %d\n", 56 + 8 * max_width, ROWS_PER_STAFF * staves);
  token[0] = 0;
  token = music;

  /* second pass: print every staff row by row */
  while (staves--) {
    for (row = 0; row < ROWS_PER_STAFF; row++) {
      int staff_line = row >= 51 || row < 18 || row % 8 != 2 ? 0 : FULL_BYTE;

      token -= 3 * count; /* back to the first token of this staff */
      count = 0;

      putchar(0); /* left margin */
      putchar(staff_line);
      for (k = 0; k < 3; k++) {
        int clef = 0;
        if (bass_clef) {
          if (row > 18 && row < 42)
            clef = font[BASS_CLEF + 3 * (row - 19) + k];
        } else if (row >= 11 && row < 64)
          clef = ~font[TREBLE_CLEF + 7 * (row - 11) + k];
        putchar(staff_line | clef);
      }
      putchar(staff_line);

      for (column = 0; (start = column) < max_width;) {
        int duration = token[1] - '1';
        int position = staff_position(token[token[0] <= '?'], bass_clef);
        int dy = row - 4 * position - 6;
        int columns = draw_width(token, start, max_width);

        for (x = 0; x < columns; x++, column++)
          putchar(staff_line |
                  draw_token(token, duration, position, row, dy, x));
        count++;
        token += 3;
      }
      putchar(0); /* right margin */
    }
    count = 0;
  }
}
