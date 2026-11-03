#include <stdio.h>

/*
 * Soundex code for each letter, indexed by (letter - '@'), so index 1 is 'A'
 * and index 26 is 'Z'.  The original uses symbols instead of digits:
 *
 *   '_' = 0  A E H I O U W Y   (vowel-like, dropped)
 *   '+' = 1  B F P V
 *   'C' = 2  C G J K Q S X Z
 *   '-' = 3  D T
 *   '*' = 4  L
 *   '/' = 5  M N
 *   '&' = 6  R
 *
 * The symbols are kept because code 2 is the letter 'C'.  The first letter of
 * a word is left as a letter and compared with the code that follows it, so a
 * word starting with 'C' drops a following code 2 (e.g. "Cs" -> "C").
 */
#define VOWEL_CODE '_'
static char letter_codes[32] = "__+C-_+C__CC*//_+C&C-_+_C_C/*__";

/*
 * Write the 4 character Soundex code of word into code.  The word is converted
 * to upper case in place and its letters after the first are replaced by their
 * codes.  Unused positions of code are left as '\0' instead of '0'.
 */
void soundex(char *code, char *word)
{
    int i, out, length;

    for (i = 0; i < strlen(word); i++)
        if (word[i] >= 'a' && word[i] <= 'z')
            word[i] -= 'a' - 'A';

    code[0] = word[0];
    code[1] = 0;
    code[2] = 0;
    code[3] = 0;

    length = strlen(word);
    if (length == 1)
        return;

    for (i = 1; i < length; i++)
        word[i] = letter_codes[word[i] - '@'];

    out = 0;
    for (i = 1; out < 3 && i < length; i++)
        if (word[i] != VOWEL_CODE && word[i - 1] != word[i])
            code[++out] = word[i];
}

int main(int argc, char **argv)
{
    FILE *file;
    int pos, len;
    char wanted[5], found[6], word[26], line[256];

    if (argc < 3)
        return 1;
    if ((file = fopen(argv[1], "rt")) == NULL)
        return 2;

    soundex(wanted, argv[2]);

    while (fgets(line, 256, file) != NULL) {
        pos = -1;
        while (pos < (int)strlen(line)) {
            /*
             * Skip to the next letter.  After the last word of a line this
             * walks past the end of the line into whatever the buffer held
             * before, just as the original does.
             */
            while (!isalpha(line[++pos]))
                ;

            len = 0;
            while (isalpha(line[pos]))
                word[len++] = line[pos++];
            word[len] = '\0';

            soundex(found, word);
            if (wanted[0] == found[0] && wanted[1] == found[1] &&
                wanted[2] == found[2] && wanted[3] == found[3]) {
                printf(line);
                pos = strlen(line); /* print each line only once */
            }
        }
    }
    fclose(file);
    return 0;
}
