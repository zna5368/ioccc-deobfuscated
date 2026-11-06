#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define WORD_LEN      5      /* every Wordle word has 5 letters */
#define MAX_WORDS     78557  /* capacity of the in-memory dictionary */
#define MAX_GUESSES   9      /* guesses remembered for "wrong position" hints */
#define LINE_SIZE     34     /* fgets() buffer size for dictionary lines */
#define INPUT_LEN     11     /* "guess color" = 5 + 1 + 5 characters */

#define NUM_KEYS      32     /* 26 letters plus 6 unused slots */
#define LETTER_BITS   5      /* a key is (weight << 5) | letter index */
#define LETTER_MASK   31
#define GROUP_SIZE    7      /* the letter sort works in groups of 7 */
#define SORTED_LETTERS 21    /* only the first 21 key slots are pre-sorted */
#define MERGED_LETTERS 28    /* 4 groups of 7 are merged together */
#define MAX_SCORE     96     /* words scoring this or more are never offered */
#define BUCKET_SIZE   5      /* at most 5 words are kept for each score */

/* Everything learned from the colors of the previous guesses. */
struct {
	char black[256];                      /* black[ch] != 0: ch is not in the answer */
	char misplaced[MAX_GUESSES][WORD_LEN]; /* non-green letters of each guess */
	char green[WORD_LEN];                 /* letters known at each position */
	char yellow[WORD_LEN];                /* letters known to be somewhere */
	int num_guesses;                      /* rows used in misplaced[] */
	int num_yellow;                       /* letters used in yellow[] */
} hints;

/* The dictionary: every word that could still be the answer. */
char words[MAX_WORDS][WORD_LEN];
int num_words;

/* How common each letter a..z is in English: 26 = most common (e). */
int english_rank[26] = {
	25, 10, 17, 15, 26, 9, 11, 12, 23, 2, 6, 18, 13,
	20, 22, 14, 1, 24, 19, 21, 16, 5, 7, 4, 8, 3
};

void copy_word(char *dst, char *src)
{
	for (int j = 0; j < WORD_LEN; ++j)
		dst[j] = src[j];
}

int contains(char *word, char ch)
{
	for (int j = 0; j < WORD_LEN; ++j)
		if (word[j] == ch)
			return 1;
	return 0;
}

int die(char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	exit(1);
}

/* Store a dictionary line if it is exactly 5 characters long. */
void add_word(char *line)
{
	if (num_words == MAX_WORDS)
		die("limit %d\n", num_words);

	/* A newline in the first position is not treated as the end of line */
	for (int j = 1; j < WORD_LEN; ++j)
		if (line[j] == '\n')
			return;
	if (line[WORD_LEN] != '\n')
		return;

	for (int j = 0; j < WORD_LEN; ++j)
		words[num_words][j] = line[j] | ' ';    /* fold to lower case */
	++num_words;
}

/* Is word still consistent with all of the hints? */
int is_possible(char *word, char *guess, int num_green)
{
	int j, g, matches = 0;

	for (j = 0; j < WORD_LEN; ++j)
		if (hints.green[j] && word[j] != hints.green[j])
			return 0;
	for (j = 0; j < WORD_LEN; ++j)
		if (hints.black[(int)word[j]])
			return 0;
	for (j = 0; j < WORD_LEN; ++j)
		if (hints.yellow[j] && !contains(word, hints.yellow[j]))
			return 0;
	for (g = hints.num_guesses - 1; g >= 0; --g)
		for (j = 0; j < WORD_LEN; ++j)
			if (hints.misplaced[g][j] == word[j])
				return 0;

	/* the word just guessed is gone, unless it was the answer */
	for (j = 0; j < WORD_LEN; ++j)
		matches += word[j] == guess[j];
	if (matches == WORD_LEN && num_green != WORD_LEN)
		return 0;

	return 1;
}

/* Remove impossible words by moving the last word into their place. */
void filter_words(char *guess, int num_green)
{
	int w = 0;

	while (w < num_words) {
		if (is_possible(words[w], guess, num_green))
			++w;
		else
			copy_word(words[w], words[--num_words]);
	}
}

/* Sort a short array of distinct values from largest to smallest. */
void sort_descending(int *list, int len)
{
	for (int i = 1; i < len; ++i) {
		int value = list[i], j = i;

		for (; j > 0 && list[j - 1] < value; --j)
			list[j] = list[j - 1];
		list[j] = value;
	}
}

/* Merge two runs of len values (left first on ties) into dst. */
void merge(int *dst, int *left, int *right, int len)
{
	int i = 0, j = 0, out = 0;

	while (i < len && j < len)
		dst[out++] = left[i] >= right[j] ? left[i++] : right[j++];
	while (i < len)
		dst[out++] = left[i++];
	while (j < len)
		dst[out++] = right[j++];
}

/*
 * Letters that appear the same number of times are reordered by how
 * common they are in English.  Ties are handled at most 7 at a time.
 */
void break_ties(int *keys)
{
	int *end = keys + 26, group;

	for (; keys < end && *keys > NUM_KEYS; keys += group) {
		int count = *keys >> LETTER_BITS;

		for (group = 1; group < GROUP_SIZE &&
		    keys[group] >> LETTER_BITS == count; )
			++group;
		for (int j = 0; j < group; ++j) {
			int letter = keys[j] & LETTER_MASK;
			keys[j] = english_rank[letter] << LETTER_BITS | letter;
		}
		sort_descending(keys, group);
	}
}

/* Does the word use some letter more than once? */
int has_repeated_letter(char *word)
{
	char seen[NUM_KEYS] = { 0 };

	for (int j = 0; j < WORD_LEN; ++j)
		if (++seen[word[j] - 'a'] > 1)
			return 1;
	return 0;
}

/* Pick the remaining word made of the most frequent letters. */
char *choose_guess(char *fallback)
{
	int keys[NUM_KEYS], rank[NUM_KEYS];
	char *buckets[MAX_SCORE][BUCKET_SIZE] = { 0 };
	int l, j, score;

	/* key = (number of times the letter appears) << 5 | letter */
	for (l = 0; l < NUM_KEYS; ++l)
		keys[l] = l;
	for (l = 0; l < num_words; ++l)
		for (j = 0; j < WORD_LEN; ++j)
			keys[words[l][j] - 'a'] += 1 << LETTER_BITS;

	/* most frequent letters first (v..z are not pre-sorted) */
	for (l = 0; l < SORTED_LETTERS; l += GROUP_SIZE)
		sort_descending(keys + l, GROUP_SIZE);
	merge(rank, keys, keys + GROUP_SIZE, GROUP_SIZE);
	merge(rank + 2 * GROUP_SIZE, keys + 2 * GROUP_SIZE,
	    keys + 3 * GROUP_SIZE, GROUP_SIZE);
	merge(keys, rank, rank + 2 * GROUP_SIZE, MERGED_LETTERS / 2);
	break_ties(keys);

	/* rank[letter] = 0 for the best letter, 1 for the next, ... */
	for (l = 0; l < NUM_KEYS; ++l)
		rank[keys[l] & LETTER_MASK] = l;

	/* a lower score means the word uses better letters */
	for (l = 0; l < num_words; ++l) {
		score = 0;
		for (j = 0; j < WORD_LEN; ++j)
			score += rank[words[l][j] - 'a'];
		if (score < MAX_SCORE)
			for (j = 0; j < BUCKET_SIZE; ++j)
				if (!buckets[score][j]) {
					buckets[score][j] = words[l];
					break;
				}
	}

	/* best score wins; prefer words without repeated letters */
	for (score = 0; score < MAX_SCORE; ++score)
		for (j = 0; j < BUCKET_SIZE && buckets[score][j]; ++j) {
			char *word = buckets[score][j];

			if (*fallback == 'x')
				fallback = word;
			if (num_words <= 2 || !has_repeated_letter(word))
				return word;
		}
	return fallback;
}

/* Record the colors of a "guess color" line and suggest the next guess. */
char *process_feedback(char *input)
{
	int num_green = 0, new_letter = 0;

	for (int j = 0; j < WORD_LEN; ++j) {
		signed char letter = input[j];
		signed char color = input[j + WORD_LEN + 1] - 'g';

		if (color == 0) {               /* green */
			hints.green[j] = letter;
			hints.black[letter] = 0;
			++num_green;
		} else {
			hints.misplaced[hints.num_guesses][j] = letter;
			if (!contains(hints.yellow, letter)) {
				new_letter = letter;
				if (color < 0)          /* black */
					hints.black[letter] = letter;
				else                    /* yellow */
					hints.yellow[hints.num_yellow++] = letter;
			}
		}
	}
	if (new_letter)
		++hints.num_guesses;

	filter_words(input, num_green);
	return choose_guess("xyzzy");
}

int main(int argc, char **argv)
{
	static char line[LINE_SIZE], input[INPUT_LEN];
	int ch, len;

	if (argc != 2)
		die("usage: %s dict\n", argv[0]);

	FILE *dict = fopen(argv[1], "r");
	if (!dict) {
		perror(argv[1]);
		return 1;
	}
	while (fgets(line, LINE_SIZE, dict))
		add_word(line);
	fclose(dict);

	setbuf(stdout, 0);
	while (num_words != 1) {
		printf("? ");
		len = 0;
		while ((ch = getchar()) != EOF && ch != '\n' && len < INPUT_LEN)
			input[len++] = ch;
		if (len < INPUT_LEN)
			die("input: guess color\n");
		printf("%.5s\n", process_feedback(input));
	}
	return 0;
}
