#include <stdio.h>
#include <string.h>

/*
 * Each Morse code is packed into a small number.  The symbols are stored
 * one bit each, first symbol in the lowest bit (1 = '.', 0 = '-').  Above
 * the symbols sits the marker 2 (binary 10), which tells where the code ends.
 *
 * Example: 'A' is ".-" -> bits 1 then 0 -> marker 10, then 0, then 1 ->
 * binary 1001 = 9.
 */
#define MARKER 2
#define MAX_SYMBOLS 5
#define LINE_SIZE 954

static const unsigned char digit_codes[10] = {
    64, /* 0 ----- */
    65, /* 1 .---- */
    67, /* 2 ..--- */
    71, /* 3 ...-- */
    79, /* 4 ....- */
    95, /* 5 ..... */
    94, /* 6 -.... */
    92, /* 7 --... */
    88, /* 8 ---.. */
    80, /* 9 ----. */
};

static const unsigned char letter_codes[26] = {
    9,  /* A .-   */
    46, /* B -... */
    42, /* C -.-. */
    22, /* D -..  */
    5,  /* E .    */
    43, /* F ..-. */
    20, /* G --.  */
    47, /* H .... */
    11, /* I ..   */
    33, /* J .--- */
    18, /* K -.-  */
    45, /* L .-.. */
    8,  /* M --   */
    10, /* N -.   */
    16, /* O ---  */
    41, /* P .--. */
    36, /* Q --.- */
    21, /* R .-.  */
    23, /* S ...  */
    4,  /* T -    */
    19, /* U ..-  */
    39, /* V ...- */
    17, /* W .--  */
    38, /* X -..- */
    34, /* Y -.-- */
    44, /* Z --.. */
};

/* Write the Morse code of a letter or digit, then a space. */
static void encode_char(char c) {
  int code;

  if (isalnum(c)) {
    if (c >= '0' && c <= '9')
      code = digit_codes[c - '0'];
    else
      code = letter_codes[(c & 31) - 1]; /* 'A'/'a' -> 0, 'B'/'b' -> 1, ... */

    do {
      putchar(code & 1 ? '.' : '-');
      code >>= 1;
    } while (code != MARKER);
  }
  putchar(' ');
}

/* Turn a packed Morse code back into a character, or '?' if unknown. */
static char lookup_code(int code) {
  int i;

  for (i = 0; i < 10; i++)
    if (digit_codes[i] == code)
      return '0' + i;
  for (i = 0; i < 26; i++)
    if (letter_codes[i] == code)
      return 'A' + i;
  return '?';
}

/*
 * Decode one Morse sequence starting at text and print its character.
 * An empty sequence (an extra space) prints a space.  Returns a pointer
 * past the sequence and its separating space.
 */
static char *decode_symbol(char *text) {
  size_t length = strspn(text, ".-");
  int code = MARKER;
  size_t i;

  if (length == 0)
    putchar(' ');
  else if (length > MAX_SYMBOLS)
    putchar('?');
  else {
    /* Read backwards so the first symbol ends up in the lowest bit. */
    for (i = length; i > 0; i--)
      code = code * 2 + (text[i - 1] == '.');
    putchar(lookup_code(code));
  }

  text += length;
  if (*text == ' ')
    text++;
  return text;
}

main() {
  char line[LINE_SIZE];
  char *text;
  size_t length;
  int to_morse;

  while (fgets(line, LINE_SIZE, stdin)) {
    /* Remove the newline (always the last character of the line). */
    length = strlen(line);
    if (length > 0)
      line[length - 1] = '\0';

    /* A line made only of spaces, dots and dashes is Morse code. */
    to_morse = line[strspn(line, " .-")] != '\0';

    text = line;
    while (*text) {
      if (to_morse)
        encode_char(*text++);
      else
        text = decode_symbol(text);
    }
    putchar('\n');
  }
}
