#include <stdio.h>

/*
 * A "letter vector" holds one small counter per distinct letter of the input
 * sentence.  The counters are stored bit-sliced across four longs: bit n of
 * plane 0 is the 1s bit of letter n's count, bit n of plane 1 the 2s bit, etc.
 */
#define PLANES 4
#define HASH_SIZE 4096
#define HASH_MASK (HASH_SIZE - 1)

long zero[PLANES];            /* constant all-zero vector */
long negated[PLANES];         /* scratch: 0 - letter */
long single_letter[PLANES];   /* scratch: vector holding one letter */
long letter_bit[256];         /* character -> its bit, 0 if not in sentence */
long next_letter_bit = 1;

typedef struct word {
  long signature;             /* OR of the planes: which letters appear */
  long hash;                  /* low 12 bits of the signature */
  long count[PLANES];         /* letter counts */
  long offset;                /* file offset of the word in the dictionary */
  struct word *next;          /* dictionary: next word in the hash bucket;
                                 search: the previous (parent) partial anagram */
} word;

word sentence;                /* letters given on the command line */
word *hash_table[HASH_SIZE];  /* dictionary candidates, chained per bucket */
char line[256];
char max_words = 3;

/*
 * difference = minuend - subtrahend, done for all letters in parallel with a
 * software full subtractor over the four bit planes.  Returns the final
 * borrow bits: a 1 marks a letter whose count went negative.
 */
long subtract(long *minuend, long *subtrahend, long *difference) {
  long borrow = 0, bit;
  int plane;

  for (plane = 0; plane < PLANES; plane++) {
    bit = borrow ^ minuend[plane] ^ subtrahend[plane];
    borrow = (borrow & subtrahend[plane]) |
             ((borrow | subtrahend[plane]) & ~minuend[plane]);
    difference[plane] = bit;
  }
  return borrow;
}

/* vector += letter, computed as vector - (0 - letter) */
void add_letter(int ch, long *vector) {
  single_letter[0] = letter_bit[ch];
  subtract(zero, single_letter, negated);
  subtract(vector, negated, vector);
}

void set_signature(word *w) {
  w->signature = w->count[0] | w->count[1] | w->count[2] | w->count[3];
  w->hash = w->signature & HASH_MASK;
}

/* print the chain of words that made up an anagram, oldest first */
void print_words(word *w, int separator) {
  int ch;

  if (w->next->next)
    print_words(w->next, ' ');
  fseek(stdin, w->offset, SEEK_SET);
  do {
    ch = getchar();
    putchar(ch != '\n' ? ch : separator);
  } while (ch != '\n');
}

/*
 * Find dictionary words that use up the letters left in `remaining`.
 * Buckets are walked downwards from `bucket`, resuming at `candidate`, so
 * each set of words is produced in only one order.  Only buckets whose index
 * is a subset of the remaining letters (and contains `top_bit`) are visited.
 */
void find_anagrams(word *remaining, int bucket, word *candidate, int depth,
                   long top_bit) {
  word rest;
  int stray;
  int letters = remaining->hash;

  rest.next = remaining;

  if (bucket > letters) {
    bucket = letters;
    candidate = hash_table[bucket];
  } else if (bucket < letters && (stray = bucket & ~letters)) {
    /* bucket uses letters we no longer have: skip to the next lower subset */
    stray |= stray >> 1;
    stray |= stray >> 2;
    stray |= stray >> 4;
    stray |= stray >> 8;
    bucket = ((((bucket & letters) | stray) & ~(stray >> 1)) - 1) & letters;
    candidate = hash_table[bucket];
  }

  --depth;

  if (depth == 0) {
    /* last word: it must contain exactly the remaining letters */
    for (; candidate; candidate = candidate->next) {
      subtract(remaining->count, candidate->count, rest.count);
      set_signature(&rest);
      if (rest.signature == 0) {
        rest.offset = candidate->offset;
        print_words(&rest, '\n');
      }
    }
    return;
  }

  while (!(top_bit & letters))
    top_bit >>= 1;

  for (;;) {
    while (!candidate) {
      /* next lower bucket index whose bits are a subset of `letters` */
      bucket = ((bucket & letters) - 1) & letters;
      if (bucket == letters || !(bucket & top_bit))
        return;
      candidate = hash_table[bucket];
    }

    if (!(candidate->signature & ~remaining->signature) &&
        !subtract(remaining->count, candidate->count, rest.count)) {
      set_signature(&rest);
      rest.offset = candidate->offset;
      if (rest.signature)
        find_anagrams(&rest, bucket, candidate->next, depth, top_bit);
      else
        print_words(&rest, '\n');
    }
    candidate = candidate->next;
  }
}

/*
 * Read the dictionary.  Every usable word lives in its own stack frame of
 * this recursive function; at end of file the search runs from the deepest
 * frame while all of the words are still alive.
 */
void read_dictionary(void) {
  word entry;
  char *ch;
  int usable;

  for (;;) {
    entry.offset = ftell(stdin);
    if (gets(line) == NULL) {
      find_anagrams(&sentence, HASH_MASK, hash_table[HASH_MASK], max_words,
                    HASH_SIZE);
      return;
    }

    usable = 1;
    for (ch = line; *ch; ch++)
      if (!letter_bit[*ch]) {
        usable = 0;
        break;
      }
    if (!usable)
      continue;

    entry.count[0] = entry.count[1] = entry.count[2] = entry.count[3] = 0;
    for (ch = line; *ch; ch++)
      add_letter(*ch, entry.count);

    if (!subtract(sentence.count, entry.count, negated)) {
      set_signature(&entry);
      entry.next = hash_table[entry.hash];
      hash_table[entry.hash] = &entry;
      read_dictionary();
      return;
    }
  }
}

int main(int argc, char **argv) {
  char *arg;

  while ((arg = *++argv)) {
    for (; *arg; arg++) {
      if (*arg == '-') {
        max_words = -atoi(arg);
        break;
      }
      if (!letter_bit[*arg]) {
        letter_bit[*arg] = next_letter_bit;
        next_letter_bit <<= 1;
      }
      add_letter(*arg, sentence.count);
    }
  }
  set_signature(&sentence);
  read_dictionary();
  return 0;
}
