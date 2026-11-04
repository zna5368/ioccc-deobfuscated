/*
 * Spelled-out number reader.
 *
 * Only eleven letters are significant.  Each one is an octal digit:
 *
 *   y=0 n=1 w=2 t=3 s=4 f=5 l=6 r=7
 *
 * and three more letters act like two-digit octal values:
 *
 *   a=010 ("ny")  b=011 ("nn")  g=012 ("nw")
 *
 * Every other letter is ignored, so a word becomes a short octal
 * "code": "seven" -> s,n -> 041, "thousand" -> t,s,a,n -> 03501.
 *
 * The result is accumulated as a hexadecimal number whose hex digits
 * are the decimal digits of the answer (two hundred three -> 0x203), so
 * printing it with %llx shows the decimal value.
 */

static const char significant_letters[] = "ynwtsflrabg";

/* Last two letters of a word: "tn" (-teen) and "ty" (-ty) */
#define SUFFIX_TEEN 031
#define SUFFIX_TY 030

/* Stems of the numbers zero through nine, indexed by their value */
static const long long digit_stems[10] = {
    07,  /* zero  -> r   */
    01,  /* one   -> n   (also "a", "uno") */
    032, /* two   -> tw  */
    037, /* three -> tr  */
    057, /* four  -> fr  */
    05,  /* five  -> f   */
    04,  /* six   -> s   */
    041, /* seven -> sn  */
    012, /* eight -> g   */
    011, /* nine  -> nn  */
};

/* Words that are a two digit number by themselves */
static const struct {
  long long stem;
  long long value;
} literal_words[3] = {
    {031, 0x10},  /* ten    -> tn  */
    {061, 0x11},  /* eleven -> ln  */
    {0326, 0x12}, /* twelve -> twl */
};

/* Words that multiply the number group below them */
static const struct {
  long long stem;
  long long scale;
} scale_words[6] = {
    {017, 1ll << 4 * 2},   /* hundred  -> nr  */
    {035, 1ll << 4 * 3},   /* thousand -> tf (t,s,a == t,f,y) */
    {06, 1ll << 4 * 6},    /* million  -> l   */
    {0116, 1ll << 4 * 9},  /* billion  -> bl  */
    {0376, 1ll << 4 * 12}, /* trillion -> trl */
    {0101, 1ll << 4 * 1},  /* and      -> an (no effect after a scale word) */
};

long long number;

/* Add a completed word to the number.  Returns 1 if the stem was known. */
int apply_stem(long long stem, int suffix) {
  int i;

  for (i = 0; i < 3; i++)
    if (literal_words[i].stem == stem) {
      number += literal_words[i].value;
      return 1;
    }

  for (i = 0; i < 6; i++)
    if (scale_words[i].stem == stem) {
      long long scale = scale_words[i].scale;
      long long group = number % scale;
      /* shift the low group up: e.g. 0x1005 "hundred" -> 0x1500 */
      number += group * (scale - 1);
      return 1;
    }

  for (i = 0; i < 10; i++)
    if (digit_stems[i] == stem) {
      if (suffix == SUFFIX_TY)
        number += i * 16; /* forty  -> 0x40 */
      else if (suffix == SUFFIX_TEEN)
        number += i + 16; /* fourteen -> 0x14 */
      else
        number += i;      /* four   -> 0x4 */
      return 1;
    }

  return 0;
}

/* Find the longest known prefix of the word and add its value */
void process_word(long long word) {
  int suffix = word % 64;

  for (; word; word /= 8)
    if (apply_stem(word, suffix))
      break;
}

int main() {
  long long word = 0;
  int c, i;

  /* lower case every character; EOF stays negative */
  while ((c = getchar() | 32) >= 0) {
    if (c < '/') {
      /* space, hyphen, comma, period, apostrophe, newline end a word */
      if (word)
        process_word(word);
      word = 0;
    } else {
      for (i = 0; i < 11; i++)
        if (significant_letters[i] == c)
          word = word * 8 + i;
    }
  }
  printf("%llx\n", number);
}
