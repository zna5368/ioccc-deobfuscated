/*
 * Anthony's Grep - deobfuscated
 *
 * A pattern is compiled by a recursive descent parser into an NFA stored
 * in a flat array ("railroad").  Lines are then matched with a DFA that is
 * built lazily from the NFA one transition at a time.
 */
#include <stdio.h>

#define MAX_ELEMENTS 512

#define EXIT_MATCH 0
#define EXIT_NOMATCH 1
#define EXIT_USAGE 2
#define EXIT_ERROR 3

/* Element labels stored in nfa[] */
#define BOL '\n'      /* beginning of line anchor */
#define EOL '\n'      /* end of line anchor */
#define STOP '\n'     /* end of a character class list */
#define WILD 0        /* '.' matches any character */
#define NCLASS (-1)   /* [^...] negated character class */
#define CLASS (-2)    /* [...] character class */
#define PASS (-3)     /* empty transition, falls through to next element */
#define JUMP(n) (-4 - (n))   /* empty transition to element n (or next) */
#define JUMP_TARGET(x) (-4 - (x))
#define IS_JUMP(x) ((x) <= PASS)

/*
 * Every DFA state set starts with this member: the WILD element of the
 * leading ".*" that main() places in front of the pattern.  It is used
 * to delimit consecutive sets inside set[].
 */
#define SET_DELIMITER 2

static int current_char;   /* character for the transition being computed */
static int reached_final;  /* true when the final NFA state is reachable */
static int nesting;        /* depth of open parentheses */
static int nfa[MAX_ELEMENTS], nfa_length;

static int dfa[MAX_ELEMENTS][MAX_ELEMENTS];
static int set[MAX_ELEMENTS], new_set, next_set;

static int new_element(int label);
static int match(char *line);
static int in_class(int *element);
static int can_reach_final(int element);
static void expr(char **pattern);
static void term(char **pattern);
static void factor(char **pattern);
static void literal(char **pattern);
static void add_reachable(int element);

int main(int argc, char **argv)
{
	int status;
	FILE *file;
	char *name, line[BUFSIZ + 1];

	if (--argc < 1) {
		fprintf(stderr, "usage:  ag <pattern> [js...]\n");
		return EXIT_USAGE;
	}

	/* Prefix pattern with an unanchored ".*": [pass][jump 3][wild][jump 2] */
	new_element(PASS);
	new_element(JUMP(nfa_length + 2));
	new_element(WILD);
	new_element(JUMP(nfa_length - 1));

	expr(&(*++argv));

	*line = BOL;
	next_set = 1;
	status = EXIT_NOMATCH;
	do {
		name = "-";
		file = stdin;
		if (1 < argc && **++argv != '-') {
			if (!(file = fopen(name = *argv, "r"))) {
				fprintf(stderr, "ag: Failed to open '%s'.\n", name);
				status = EXIT_ERROR;
				continue;
			}
		}
		while (fgets(line + 1, BUFSIZ, file)) {
			if (match(line)) {
				printf("%s:%s", name, line + 1);
				status = EXIT_MATCH;
			}
		}
		fclose(file);
	} while (1 < --argc);
	return status;
}

/* expr: term { '|' term } */
static void expr(char **pattern)
{
	int branch, skip;

	branch = new_element(PASS);
	term(pattern);
	while (**pattern == '|') {
		++*pattern;
		skip = new_element(PASS);
		new_element(STOP);
		nfa[branch] = JUMP(nfa_length);
		branch = new_element(PASS);
		term(pattern);
		nfa[skip] = JUMP(nfa_length);
	}
}

/* term: factor { factor } */
static void term(char **pattern)
{
	while (**pattern != '|' && (!nesting || **pattern != ')') && **pattern)
		factor(pattern);
}

/* factor: ( '^' | '$' | '(' expr ')' | literal ) [ '*' | '?' ] */
static void factor(char **pattern)
{
	int start = new_element(PASS);

	if (**pattern == '^') {
		++*pattern;
		new_element(BOL);
	} else if (**pattern == '$') {
		++*pattern;
		new_element(EOL);
	} else if (**pattern == '(') {
		++*pattern;
		++nesting;
		expr(pattern);
		if (**pattern != ')') {
			fprintf(stderr, "ag: Missing ')'.\n");
			exit(EXIT_ERROR);
		}
		--nesting;
		++*pattern;
	} else {
		literal(pattern);
	}

	if (**pattern == '*') {
		++*pattern;
		nfa[start] = JUMP(nfa_length);
		new_element(JUMP(start + 1));
	} else if (**pattern == '?') {
		++*pattern;
		nfa[start] = JUMP(nfa_length);
	}
}

/* literal: '.' | '[' class ']' | '\' char | char */
static void literal(char **pattern)
{
	int c, hi;

	if (**pattern == '.') {
		new_element(WILD);
	} else if (**pattern == '[') {
		++*pattern;
		c = new_element(CLASS);
		if (**pattern == '^') {
			++*pattern;
			nfa[c] = NCLASS;
		}
		if (**pattern == ']') {
			++*pattern;
			new_element(']');
		}
		while ((c = **pattern) != ']') {
			if ((*pattern)[1] == '-' && c < (hi = (*pattern)[2])) {
				while (c <= hi)
					new_element(c++);
				*pattern += 3;
			} else {
				new_element(c);
				++*pattern;
			}
		}
		new_element(STOP);
	} else {
		if (**pattern == '\\')
			++*pattern;
		new_element(**pattern);
	}
	++*pattern;
}

/* Append an element to the NFA and return its index. */
static int new_element(int label)
{
	if (MAX_ELEMENTS <= nfa_length) {
		fprintf(stderr, "ag: Pattern too long.\n");
		exit(EXIT_ERROR);
	}
	nfa[nfa_length] = label;
	return nfa_length++;
}

/*
 * Follow empty transitions from 'element' and add every labelled element
 * that matches current_char to the set being built.
 */
static void add_reachable(int element)
{
	int i = new_set;

	if (nfa_length <= element)
		return;
	if (IS_JUMP(nfa[element])) {
		if (nfa[element] != PASS)
			add_reachable(JUMP_TARGET(nfa[element]));
		add_reachable(element + 1);
	} else if (nfa[element] == WILD || nfa[element] == current_char ||
		   in_class(&element)) {
		while (i < next_set)
			if (set[i++] == element)
				return;	/* already a member */

		reached_final = can_reach_final(element + 1);

		if (MAX_ELEMENTS <= next_set) {
			fprintf(stderr, "ag: Out of space.\n");
			exit(EXIT_ERROR);
		}
		set[next_set++] = element;
	}
}

/*
 * For a CLASS element return true if current_char is listed, for an
 * NCLASS element return true if it is not.  Advances 'element' to the
 * STOP at the end of the class.
 */
static int in_class(int *element)
{
	int found = 0, negated = 0;

	if (nfa[*element] == CLASS || (negated = nfa[*element] == NCLASS)) {
		while (nfa[++*element] != STOP)
			if (nfa[*element] == current_char)
				found = 1;
	}
	return found ^ negated;
}

/* True if the end of the NFA is reachable using only empty transitions. */
static int can_reach_final(int element)
{
	return reached_final || element == nfa_length ||
	       (IS_JUMP(nfa[element]) &&
		((nfa[element] != PASS &&
		  can_reach_final(JUMP_TARGET(nfa[element]))) ||
		 can_reach_final(element + 1)));
}

/*
 * Return true if the line matches.  The line has the form "\ntext\n"
 * so that the anchors can match the surrounding newlines.
 */
static int match(char *line)
{
	int state, i, j, k;

	reached_final = state = 0;
	while (*line && 0 <= state) {
		if (0 < (i = dfa[state][*line])) {
			/* Known DFA transition */
			state = i;
		} else {
			/* Compute the next set of NFA elements on *line. */
			current_char = *line;
			i = dfa[state][0];
			j = new_set = next_set;
			do
				add_reachable(set[i++] + 1);
			while (set[i] != SET_DELIMITER);

			/* Search earlier sets for one equal to the new set. */
			i = k = 0;
			while (i < new_set) {
				if (set[i] == SET_DELIMITER) {
					j = new_set;
					++k;
				}
				if (set[i++] == set[j]) {
					if (next_set <= ++j) {
						j = new_set;
						if (set[i] == SET_DELIMITER) {
							/* existing state: discard copy */
							next_set = new_set;
							break;
						}
					}
				} else {
					j = new_set;
				}
			}
			if (new_set < next_set)
				dfa[++k][0] = new_set;	/* new DFA state */
			state = dfa[state][*line] = reached_final ? -1 : k;
		}
		++line;
	}
	return state < 0;
}
