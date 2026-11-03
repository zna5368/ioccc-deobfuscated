#include <stdio.h>
#include <string.h>

/*
 * The real program is hidden as whitespace and braces inside the big
 * comment of spinellis.c.  Run it as:
 *
 *     ./simple < spinellis.c
 *
 * Each character of the comment is one "digit" whose value is its position
 * in the alphabet below.  The '*' that opens the comment marks the start of
 * the encoded text.
 */
static const char *alphabet = "{ }\t;\n*";

#define START_MARK 6 /* index of '*' in the alphabet */
#define END_MARK '$' /* decoded character that ends the text */

/* Read one input character and return its position in the alphabet. */
int read_digit(void) { return strchr(alphabet, getchar()) - alphabet; }

/* Read a base-3 digit: the six alphabet symbols are paired up, 0..5 -> 0..2 */
int read_trit(void) { return read_digit() / 2; }

/*
 * Decode one output character.  A code 0..53 is built from one base-6
 * digit followed by two base-3 digits:  code = 9*d6 + 3*t1 + t2.
 *
 *   0       space
 *   1       newline
 *   2       tab
 *   3..32   'a'..'~'   (lowercase letters and { | } ~)
 *   33..53  one more base-3 digit selects one of three consecutive
 *           characters starting at (code-32)*3 + ' ', covering
 *           '#'..'`' (digits, uppercase letters and punctuation)
 */
int decode_char(void)
{
	int code;

	code = read_digit() * 3;
	code += read_trit();
	code = code * 3 + read_trit();

	if (code == 0)
		return ' ';
	if (code == 1)
		return '\n';
	if (code == 2)
		return '\t';
	if (code < 33)
		return code - 3 + 'a';
	return (code - 32) * 3 + ' ' + read_trit();
}

int main(void)
{
	int ch;

	/* skip everything up to the '*' that opens the comment */
	while (read_digit() != START_MARK)
		;

	while ((ch = decode_char()) != END_MARK)
		putchar(ch);

	return 0;
}
