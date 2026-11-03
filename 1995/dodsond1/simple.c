/*
 * Translate English text read from stdin into Pig Latin on stdout.
 *
 * Note: the letter ranges deliberately include one extra character past
 * 'Z' and 'z' ('[' and '{') to match the original program's range test.
 */

#define ALPHABET_SIZE 26

/* 'A'..'Z' plus '[' */
int is_upper(int c) { return c >= 'A' && c <= 'A' + ALPHABET_SIZE; }

/* 'a'..'z' plus '{' */
int is_lower(int c) { return c >= 'a' && c <= 'a' + ALPHABET_SIZE; }

int is_letter(int c) { return is_upper(c) || is_lower(c); }

int to_lower(int c) { return is_upper(c) ? c - 'A' + 'a' : c; }

int to_upper(int c) { return is_lower(c) ? c - 'a' + 'A' : c; }

/* letters (and '{') that are not vowels a, e, i, o, u */
int is_consonant(int c) {
  int lower = to_lower(c);
  return is_letter(c) && lower != 'a' && lower != 'e' && lower != 'i' &&
         lower != 'o' && lower != 'u';
}

main() {
  char suffix[9], *end;
  char c = getchar(); /* char, so EOF (and byte 0xff) read as -1 */
  char capitalized;

  /* copy any text before the first word */
  while (c != -1 && !is_letter(c)) {
    putchar(c);
    c = getchar();
  }

  while (c != -1) {
    /* move the leading consonants of the word into the suffix */
    end = suffix;
    capitalized = is_upper(c);
    while (is_consonant(c)) {
      *end++ = to_lower(c);
      c = getchar();
    }
    /* words starting with a vowel get "wa", otherwise "a" */
    if (end == suffix)
      *end++ = 'w';
    *end++ = 'a';
    *end = 0;

    /* print the rest of the word, carrying over the capital letter */
    if (capitalized)
      c = to_upper(c);
    while (is_letter(c)) {
      putchar(c);
      c = getchar();
    }

    /* then the moved consonants with the suffix */
    for (end = suffix; *end; end++)
      putchar(*end);

    /* copy the text up to the next word */
    while (c != -1 && !is_letter(c)) {
      putchar(c);
      c = getchar();
    }
  }
}
