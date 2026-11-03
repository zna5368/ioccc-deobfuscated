#include <string.h>

#define CONTEXT_LEN 4      /* P:  length of the correlator string */
#define ALPHABET 128       /* AE: number of character counters */
#define TEMPLATE_MAX 4096  /* PE: maximum template characters read */

/*
 * The counters are plain (signed) chars just like in the original, so they
 * wrap around for large templates exactly the same way.
 */
char template[TEMPLATE_MAX + 1]; /* template text read from stdin */
char context_len;                /* chars currently in the correlator */
char *template_end;              /* end of the template text */
char context[CONTEXT_LEN + 1];   /* correlator string (last chars printed) */
char counts[ALPHABET];           /* how often each char follows context */

/*
 * Find every place in the template where the correlator string occurs,
 * count which character follows it, then pick one of those characters at
 * random weighted by its count.  Returns 0 when nothing matches.
 */
pick_next_char() {
  register char next, *pos, target, total = 0;

  for (pos = template; pos < template_end - CONTEXT_LEN; pos++)
    if (!strncmp(pos, context, context_len)) {
      counts[pos[context_len]]++;
      total++;
    }
  if (!total)
    return total;

  target = rand() % total + 1;

  /* walk the counters until the running total reaches the target */
  next = -1;
  total = 0;
  do {
    next++;
    total += counts[next];
  } while (total < target);
  return next;
}

main() {
  char next, *pos = template;

  srand((unsigned)time((long)0));

  /* read the template, stopping at EOF or when the buffer is full */
  while (((*(pos++) = getchar()) != -1) && ((pos - template) < TEMPLATE_MAX))
    ;
  template_end = --pos;
  *template_end = '\0';

  for (context_len = 0; context_len <= CONTEXT_LEN;) {
    /* reset the character statistics */
    {
      register unsigned i = ALPHABET;
      while (i--)
        counts[i] = 0;
    }

    next = pick_next_char();
    if (!next)
      break;
    putchar(next);

    if (context_len < CONTEXT_LEN) {
      /* correlator not full yet: append the char */
      context[context_len] = next;
      context_len++;
    } else {
      /* correlator full: drop its first char and append the new one */
      for (pos = context + 1; *pos; pos++)
        *(pos - 1) = *pos;
      *(pos - 1) = next;
      ++context_len;
    }
  }
}
