#include <stdio.h>

#define MAX_WORD 8 /* longest word kept in the buffer */

static int is_upper(int c) { return c >= 'A' && c <= 'Z'; }
static int is_lower(int c) { return c >= 'a' && c <= 'z'; }
static int is_letter(int c) { return is_upper(c) || is_lower(c); }

/*
 * Return the replacement letter with the same case as the letter it
 * replaces: upper case if the original was upper case, else lower case.
 */
static int match_case(int original, int replacement) {
  if (is_upper(original))
    return is_lower(replacement) ? replacement - ('a' - 'A') : replacement;
  else
    return is_upper(replacement) ? replacement + ('a' - 'A') : replacement;
}

/* Print the first len letters of replacement, cased like word. */
static void print_poot(const char *word, const char *replacement, int len) {
  int i;

  for (i = 0; i < len; i++)
    printf("%c", match_case(word[i], replacement[i]));
}

int main() {
  char *poot = "poot", *pootpoot = "pootpoot", word[MAX_WORD + 1];
  int c;
  int len = 0;       /* number of letters in word */
  int tag_depth = 0; /* > 0 while inside an HTML <...> tag */

  word[0] = '\0';
  while ((c = getc(stdin)) != EOF) {
    if (tag_depth == 0 && is_letter(c)) {
      /* collect letters of a word; letters past MAX_WORD are dropped */
      if (len != MAX_WORD) {
        word[len++] = c;
        word[len] = '\0';
      }
      continue;
    }

    /* a non-letter (or anything inside a tag) ends the current word */
    if (len >= 8)
      print_poot(word, pootpoot, 8);
    else if (len >= 4)
      print_poot(word, poot, 4);
    else
      printf("%s", word);
    printf("%c", c);

    len = 0;
    word[0] = '\0';

    /* skip over HTML tags so markup is not pootified */
    if (c == '<')
      ++tag_depth;
    if (c == '>' && tag_depth != 0)
      --tag_depth;
  }
  /* a word still in the buffer at end of input is never printed */
  return 0;
}
