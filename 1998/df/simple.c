#include <stdio.h>
#include <string.h>
#include <time.h>

#define WORD_LEN 8
#define NUM_WORDS 24
#define HIDDEN '_'
#define TO_UPPER_MASK 0xDF /* clears bit 5: 'a' -> 'A' */

/*
 * The word list.  The original program decodes it at run time from
 * compressed text hidden in its own source file.
 */
static const char *words[NUM_WORDS] = {
    "COMPUTER", "BEVERAGE", "OBFSCATD", "BORDEAUX", "BOUNDARY", "ESCAPADE",
    "EVERYONE", "SOMEBODY", "GUARDIAN", "IMPERIAL", "INFERIOR", "MERIDIAN",
    "NEGATIVE", "POSTPONE", "PRECLUDE", "RULEBOOK", "SCAFFOLD", "SKYWARDS",
    "SOFTWARE", "HARDWARE", "TOUCHING", "UNTITLED", "YEARBOOK", "WITHHOLD"};

static void play(void) {
  const char *word = words[time(NULL) % NUM_WORDS];
  char shown[WORD_LEN + 1];
  char typed = 0, guess;
  int i, remaining = 1;

  for (i = 0; i < WORD_LEN; i++)
    shown[i] = HIDDEN;
  shown[WORD_LEN] = '\0';

  for (;;) {
    printf("\t[%s]\n", shown);
    if (!remaining)
      break;

    /* read one guess, ignoring newlines (on end of input the last
       character read is guessed again forever) */
    do {
      scanf("%c", &typed);
      guess = typed & TO_UPPER_MASK;
    } while (guess == '\n');

    /* reveal matching letters and count how many are still hidden */
    remaining = 0;
    for (i = 0; i < WORD_LEN; i++) {
      if (word[i] == guess)
        shown[i] = guess;
      if (shown[i] == HIDDEN)
        remaining++;
    }
  }
}

int main(void) {
  play();
  return 0;
}
