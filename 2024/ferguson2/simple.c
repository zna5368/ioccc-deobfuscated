#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef DICT
#define DICT "dict"
#endif

/* one line of the dictionary: an English word or phrase and its code word */
struct entry {
    char *english;
    char *navajo;
    struct entry *next;
};

/* the Navajo code alphabet, used to spell words not found in the dictionary */
static const char *alphabet[26][2] = {
    { "A", "WOL-LA-CHEE" },       { "B", "SHUSH" },
    { "C", "MOASI" },             { "D", "CHINDI" },
    { "E", "DZEH" },              { "F", "CHUO" },
    { "G", "JEHA" },              { "H", "TSE-GAH" },
    { "I", "YEH-HES" },           { "J", "AH-YA-TSINNE" },
    { "K", "BA-AH-NE-DI-TININ" }, { "L", "NASH-DOIE-TSO" },
    { "M", "BE-TAS-TNI" },        { "N", "TSAH" },
    { "O", "A-KHA" },             { "P", "CLA-GI-AIH" },
    { "Q", "CA-YEILTH" },         { "R", "DAH-NES-TSA" },
    { "S", "KLESH" },             { "T", "D-AH" },
    { "U", "NO-DA-IH" },          { "V", "A-KEH-DI-GLINI" },
    { "W", "GLOE-IH" },           { "X", "AL-NA-AS-DZOH" },
    { "Y", "TSAH-AS-ZIH" },       { "Z", "BESH-DO-TLIZ" },
};

/* dictionary entries; each line is pushed on the front, so the list is
 * in reverse file order and later lines of the file win */
static struct entry *dictionary;

/* the last character written while translating (used to decide on spaces) */
static int last_char;

/*
 * The dictionary file is stored with an Atbash cipher on the upper case
 * letters (A <-> Z, B <-> Y, ...).  Decode a string in place.
 */
static char *atbash(char *s)
{
    int i;

    for (i = 0; s[i]; ++i)
        if (isupper(s[i]))
            s[i] = "ZYXWVUTSRQPONMLKJIHGFEDCBA"[s[i] - 'A'];
    return s;
}

/*
 * Compare a dictionary word with the input word, ignoring case and any
 * characters that are not letters.  Both must be at least two characters.
 */
static int words_match(const char *word, const char *text)
{
    if (word[0] && text[0] && word[1] && text[1]) {
        while (*word && *text) {
            /* skip non-letters; like the original this also steps one
             * past a string's terminating NUL when it reaches it */
            while (!isalpha(*word))
                if (*word++ == '\0')
                    break;
            while (!isalpha(*text))
                if (*text++ == '\0')
                    break;
            if (!*text || !*word || tolower(*word) != tolower(*text))
                break;
            ++word;
            ++text;
        }
    }
    return !*word && !*text;
}

/* read the dictionary file: tab separated English, Navajo, literal meaning */
static void load_dictionary(void)
{
    FILE *fp = fopen(DICT, "r");
    char *line = NULL;
    size_t size = 0;
    char *english, *navajo;
    struct entry *e;

    if (!fp) {
        puts("couldn't open dict");
        return;
    }
    while (getline(&line, &size, fp) != -1) {
        if ((english = strtok(line, "\t")) && (navajo = strtok(NULL, "\t"))) {
            e = calloc(1, sizeof *e);
            e->english = strdup(atbash(english));
            e->navajo = strdup(atbash(navajo));
            e->next = dictionary;
            dictionary = e;
        }
    }
    free(line);
    fclose(fp);
}

/* find the first entry (in list order) whose English text matches word */
static struct entry *lookup(struct entry *list, const char *word)
{
    for (; list; list = list->next)
        if (words_match(list->english, word))
            return list;
    return NULL;
}

/* spell a word one letter at a time using the Navajo code alphabet */
static void spell_word(const char *word)
{
    int i, j, k, lower;

    for (i = 0; word[i]; ++i) {
        if (!isalpha(word[i])) {
            last_char = putchar(word[i]);
            continue;
        }
        lower = islower(word[i]);
        for (j = 0; j < 26; ++j) {
            if (alphabet[j][0][0] == toupper(word[i])) {
                for (k = 0; alphabet[j][1][k]; ++k)
                    putchar(lower ? tolower(alphabet[j][1][k])
                                  : toupper(alphabet[j][1][k]));
                last_char = putchar(' ');
                break;
            }
        }
    }
}

/*
 * Translate one word: use its dictionary code word if there is one,
 * otherwise spell it out letter by letter.
 */
static void translate_word(const char *word, struct entry *list)
{
    struct entry *e = lookup(list, word);
    int i;

    if (e) {
        /* copy any leading punctuation */
        while (!isalpha(*word) && *word)
            last_char = putchar(*word++);
        /* keep the first letter upper case only if the input's is */
        for (i = 0; e->navajo[i]; ++i)
            last_char = putchar(i == 0 && isupper(word[0]) ? e->navajo[i]
                                                           : tolower(e->navajo[i]));
    } else {
        spell_word(word);
    }
    if (!isspace(last_char))
        putchar(' ');
}

/*
 * Translate every whitespace separated word of a file.  The dictionary is
 * never passed along here, so file words are always spelled out.
 */
static void translate_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    char *line = NULL;
    size_t size = 0;
    char *token, *word;

    while (fp && getline(&line, &size, fp) != -1) {
        for (token = strtok(line, " \t"); token; token = strtok(NULL, " \t")) {
            if ((word = strdup(token))) {
                translate_word(word, NULL);
                free(word);
                if (!isspace(last_char))
                    putchar(' ');
            }
        }
    }
    if (fp) {
        clearerr(fp);
        fclose(fp);
        free(line);
    }
}

int main(int argc, char **argv)
{
    int i;

    load_dictionary();
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '+')
            translate_file(argv[i] + 1);
        else
            translate_word(argv[i], dictionary);
    }
    puts("");
}
