/*
 * The high bit of the first character of each word is used as a flag.
 * When it is set the word is "left out" of the current subset.  Flipping
 * the flags in Gray code order visits every subset except the full set,
 * which is the starting state and is never printed.
 */

#define LEFT_OUT_BIT 0x80
#define SEPARATOR "----------"

/* A word is printed when its first character is non-zero and not flagged. */
static int is_in_subset(const char *word)
{
  return word[0] != 0 && !(word[0] & LEFT_OUT_BIT);
}

static void toggle_word(char *word)
{
  word[0] ^= LEFT_OUT_BIT;
}

/* words[0] is the program name, so the words start at index 1. */
static void print_subset(char **words)
{
  int i;

  for (i = 1; words[i] != 0; i++)
    if (is_in_subset(words[i]))
      puts(words[i]);
  puts(SEPARATOR);
}

/*
 * Visit every combination of flags for words[1..count]: walk all
 * combinations of the first count-1 words, flip word number count,
 * print the result, then walk the first count-1 words again.
 */
static void enumerate_subsets(int count, char **words)
{
  if (count == 0)
    return;

  enumerate_subsets(count - 1, words);
  toggle_word(words[count]);
  print_subset(words);
  enumerate_subsets(count - 1, words);
}

int main(int argc, char **argv)
{
  enumerate_subsets(argc - 1, argv);
  return 0;
}
