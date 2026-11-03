#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORDS 111
#define MAX_WORD_LEN 21
#define LINE_LEN 111

/* The first 3 characters end a sentence; all 6 are stripped from words. */
static const char punctuation[] = "!?.:;,";
#define SENTENCE_ENDERS 3
#define ALL_PUNCTUATION 6

/* Does the last character of word match one of the first count punctuation marks? */
int ends_with_punct(char *word, int count)
{
  int i, found = 0;
  for (i = 0; i < count; i++)
    found |= punctuation[i] == word[strlen(word) - 1];
  return found;
}

/* Remove one trailing punctuation mark from word, if there is one. */
void strip_punct(char *word)
{
  if (ends_with_punct(word, ALL_PUNCTUATION))
    word[strlen(word) - 1] = '\0';
}

/*
 * Usage: prog file           -- print the file, then ask for a question
 *        prog file words...  -- answer the question made of words, then ask again
 *
 * Each question is answered by re-running this program through system()
 * with the question words as extra arguments.
 */
int main(int argc, char *argv[])
{
  char words[MAX_WORDS][MAX_WORD_LEN];      /* unmatched words of current sentence */
  char best_words[MAX_WORDS][MAX_WORD_LEN]; /* unmatched words of best sentence */
  char line[LINE_LEN], command[LINE_LEN];
  int nwords, best_nwords, score, best_score;
  int matched, end_of_sentence, i, k;
  FILE *file;

  if (argc != 2) {
    /* Find the sentence sharing the most words with the question. */
    file = fopen(argv[1], "r");
    nwords = best_nwords = best_score = score = 0;
    while (fscanf(file, "%s", words[nwords]) != EOF) {
      end_of_sentence = ends_with_punct(words[nwords], SENTENCE_ENDERS);
      strip_punct(words[nwords]);

      /* Compare against every argument (program name and file name too). */
      matched = 0;
      for (k = 0; k < argc; k++) {
        strip_punct(argv[k]);
        if (strcasecmp(argv[k], words[nwords]) == 0)
          matched = ++score;
      }
      /* Keep only the words that were not in the question. */
      if (!matched)
        nwords++;

      if (end_of_sentence) {
        if (score > best_score) {
          best_score = score;
          best_nwords = nwords;
          for (i = 0; i < nwords; i++)
            strcpy(best_words[i], words[i]);
        }
        score = nwords = 0;
      }
    }

    if (!best_score)
      puts(" Beats me.");
    else if (!best_nwords)
      puts(" Yes.");
    else {
      /* Answer with the leftover words of the best sentence. */
      best_words[0][0] = toupper(best_words[0][0]);
      for (i = 0; i < best_nwords; i++)
        printf(" %s", best_words[i]);
      puts(".");
    }
  } else {
    /* First run: show the text. */
    sprintf(line, "cat %s;echo", argv[1]);
    system(line);
  }

  /* Read a question and run ourselves again with it as arguments. */
  printf("\n? ");
  fgets(line, LINE_LEN, stdin);
  sprintf(command, "%s %s %s", argv[0], argv[1], line);
  system(command);
}
