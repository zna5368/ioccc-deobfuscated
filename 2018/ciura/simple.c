#include<unistd.h>

/*
 * Prints only the sentences of the standard input that use the passive
 * voice, with each passive construction wrapped in highlight marks.
 *
 * Two small state machines do the work:
 *   - the sentence machine reads one character at a time and reports when a
 *     sentence starts, a word (token) ends and a sentence ends;
 *   - the passive machine reads one word class at a time and reports where
 *     a passive construction starts and ends.
 * Word classes are found by matching the end of each word against the word
 * lists further below.
 */

#define MAX_INPUT (9 << 20)

/* The whole input, with two zero bytes in front and at least one behind. */
static unsigned char text[2 + MAX_INPUT + 1];

/* Character classes read by the sentence machine. */
enum char_class {
    NUL_CHAR,       /* '\0': end of the input */
    BLANK,          /* space, tab and other control characters */
    LINE_BREAK,     /* '\n' */
    UPPER,          /* 'A'-'Z' and bytes 128-255 */
    LOWER,          /* 'a'-'z', '0'-'9' and the apostrophe */
    DOT,            /* '.' */
    BANG,           /* '!' and '?' */
    OTHER_PUNCT     /* any other printable character */
};

/* What the sentence machine reports after each character. */
enum sentence_event {
    END_OF_TEXT,
    SENTENCE_START,
    SENTENCE_END,
    TOKEN_END,
    NO_EVENT
};

enum sentence_state {
    OUTSIDE_SENTENCE,
    IN_PUNCTUATION,
    AT_END_OF_INPUT,
    FINISHED,
    AFTER_BANG,
    AFTER_PERIOD,
    IN_CAPITALIZED_WORD,
    IN_WORD,
    AFTER_SPACE,
    AFTER_NEWLINE,
    SPACE_AFTER_STOP,
    NEWLINE_AFTER_STOP
};

struct sentence_transition {
    unsigned char next_state;
    unsigned char event;
};

static const struct sentence_transition sentence_machine[][8] = {
    /* OUTSIDE_SENTENCE */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, NO_EVENT},
        /* BLANK       */ {OUTSIDE_SENTENCE, NO_EVENT},
        /* LINE_BREAK  */ {OUTSIDE_SENTENCE, NO_EVENT},
        /* UPPER       */ {IN_CAPITALIZED_WORD, SENTENCE_START},
        /* LOWER       */ {IN_WORD, SENTENCE_START},
        /* DOT         */ {AFTER_PERIOD, SENTENCE_START},
        /* BANG        */ {AFTER_BANG, SENTENCE_START},
        /* OTHER_PUNCT */ {IN_PUNCTUATION, SENTENCE_START},
    },
    /* IN_PUNCTUATION */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, TOKEN_END},
        /* BLANK       */ {AFTER_SPACE, TOKEN_END},
        /* LINE_BREAK  */ {AFTER_NEWLINE, TOKEN_END},
        /* UPPER       */ {IN_CAPITALIZED_WORD, TOKEN_END},
        /* LOWER       */ {IN_WORD, TOKEN_END},
        /* DOT         */ {AFTER_PERIOD, NO_EVENT},
        /* BANG        */ {AFTER_BANG, NO_EVENT},
        /* OTHER_PUNCT */ {IN_PUNCTUATION, NO_EVENT},
    },
    /* AT_END_OF_INPUT */ {
        /* NUL_CHAR    */ {FINISHED, SENTENCE_END},
        /* BLANK       */ {FINISHED, SENTENCE_END},
        /* LINE_BREAK  */ {FINISHED, SENTENCE_END},
        /* UPPER       */ {FINISHED, SENTENCE_END},
        /* LOWER       */ {FINISHED, SENTENCE_END},
        /* DOT         */ {FINISHED, SENTENCE_END},
        /* BANG        */ {FINISHED, SENTENCE_END},
        /* OTHER_PUNCT */ {FINISHED, SENTENCE_END},
    },
    /* FINISHED */ {
        /* NUL_CHAR    */ {FINISHED, END_OF_TEXT},
        /* BLANK       */ {FINISHED, END_OF_TEXT},
        /* LINE_BREAK  */ {FINISHED, END_OF_TEXT},
        /* UPPER       */ {FINISHED, END_OF_TEXT},
        /* LOWER       */ {FINISHED, END_OF_TEXT},
        /* DOT         */ {FINISHED, END_OF_TEXT},
        /* BANG        */ {FINISHED, END_OF_TEXT},
        /* OTHER_PUNCT */ {FINISHED, END_OF_TEXT},
    },
    /* AFTER_BANG */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, TOKEN_END},
        /* BLANK       */ {SPACE_AFTER_STOP, TOKEN_END},
        /* LINE_BREAK  */ {NEWLINE_AFTER_STOP, TOKEN_END},
        /* UPPER       */ {IN_WORD, TOKEN_END},
        /* LOWER       */ {IN_WORD, TOKEN_END},
        /* DOT         */ {AFTER_PERIOD, NO_EVENT},
        /* BANG        */ {AFTER_BANG, NO_EVENT},
        /* OTHER_PUNCT */ {AFTER_BANG, NO_EVENT},
    },
    /* AFTER_PERIOD */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, TOKEN_END},
        /* BLANK       */ {SPACE_AFTER_STOP, TOKEN_END},
        /* LINE_BREAK  */ {NEWLINE_AFTER_STOP, TOKEN_END},
        /* UPPER       */ {IN_WORD, TOKEN_END},
        /* LOWER       */ {IN_WORD, TOKEN_END},
        /* DOT         */ {AFTER_PERIOD, NO_EVENT},
        /* BANG        */ {AFTER_BANG, NO_EVENT},
        /* OTHER_PUNCT */ {AFTER_PERIOD, NO_EVENT},
    },
    /* IN_CAPITALIZED_WORD */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, TOKEN_END},
        /* BLANK       */ {AFTER_SPACE, TOKEN_END},
        /* LINE_BREAK  */ {AFTER_NEWLINE, TOKEN_END},
        /* UPPER       */ {IN_WORD, NO_EVENT},
        /* LOWER       */ {IN_WORD, NO_EVENT},
        /* DOT         */ {IN_PUNCTUATION, TOKEN_END},
        /* BANG        */ {AFTER_BANG, TOKEN_END},
        /* OTHER_PUNCT */ {IN_PUNCTUATION, TOKEN_END},
    },
    /* IN_WORD */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, TOKEN_END},
        /* BLANK       */ {AFTER_SPACE, TOKEN_END},
        /* LINE_BREAK  */ {AFTER_NEWLINE, TOKEN_END},
        /* UPPER       */ {IN_WORD, NO_EVENT},
        /* LOWER       */ {IN_WORD, NO_EVENT},
        /* DOT         */ {AFTER_PERIOD, TOKEN_END},
        /* BANG        */ {AFTER_BANG, TOKEN_END},
        /* OTHER_PUNCT */ {IN_PUNCTUATION, TOKEN_END},
    },
    /* AFTER_SPACE */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, NO_EVENT},
        /* BLANK       */ {AFTER_SPACE, NO_EVENT},
        /* LINE_BREAK  */ {AFTER_NEWLINE, NO_EVENT},
        /* UPPER       */ {IN_CAPITALIZED_WORD, NO_EVENT},
        /* LOWER       */ {IN_WORD, NO_EVENT},
        /* DOT         */ {AFTER_PERIOD, NO_EVENT},
        /* BANG        */ {AFTER_BANG, NO_EVENT},
        /* OTHER_PUNCT */ {IN_PUNCTUATION, NO_EVENT},
    },
    /* AFTER_NEWLINE */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, NO_EVENT},
        /* BLANK       */ {AFTER_NEWLINE, NO_EVENT},
        /* LINE_BREAK  */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* UPPER       */ {IN_CAPITALIZED_WORD, NO_EVENT},
        /* LOWER       */ {IN_WORD, NO_EVENT},
        /* DOT         */ {AFTER_PERIOD, NO_EVENT},
        /* BANG        */ {AFTER_BANG, NO_EVENT},
        /* OTHER_PUNCT */ {IN_PUNCTUATION, NO_EVENT},
    },
    /* SPACE_AFTER_STOP */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, NO_EVENT},
        /* BLANK       */ {SPACE_AFTER_STOP, NO_EVENT},
        /* LINE_BREAK  */ {NEWLINE_AFTER_STOP, NO_EVENT},
        /* UPPER       */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* LOWER       */ {IN_WORD, NO_EVENT},
        /* DOT         */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* BANG        */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* OTHER_PUNCT */ {OUTSIDE_SENTENCE, SENTENCE_END},
    },
    /* NEWLINE_AFTER_STOP */ {
        /* NUL_CHAR    */ {AT_END_OF_INPUT, NO_EVENT},
        /* BLANK       */ {NEWLINE_AFTER_STOP, NO_EVENT},
        /* LINE_BREAK  */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* UPPER       */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* LOWER       */ {IN_WORD, NO_EVENT},
        /* DOT         */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* BANG        */ {OUTSIDE_SENTENCE, SENTENCE_END},
        /* OTHER_PUNCT */ {OUTSIDE_SENTENCE, SENTENCE_END},
    },
};

/* Word classes read by the passive machine. */
enum word_class {
    ADVERB,         /* "not", "also", "quickly", ... */
    OTHER_WORD,     /* any word that is in none of the lists */
    PARTICIPLE,     /* "bazzed", "known", "taken", ... */
    BE_OR_GET,      /* "be", "been", "being", "get", "got", "'m", "'re", ... */
    IS_OR_WAS,      /* "am", "is", "are", "was", "were", "wasn't", ... */
    WH_WORD         /* "who", "which", "by", "of", and empty words */
};

enum passive_action {
    START_HIGHLIGHT,
    NO_ACTION,
    END_HIGHLIGHT
};

enum passive_state {
    NO_PASSIVE,
    SENTENCE_BEGIN,
    PARTICIPLE_FIRST,   /* "Also bazzed is bar." */
    QUESTION,           /* "Was foo indeed bazzed?" */
    AFTER_BE            /* "Foo was bazzed." */
};

struct passive_transition {
    unsigned char next_state;
    unsigned char action;
};

static const struct passive_transition passive_machine[][6] = {
    /* NO_PASSIVE */ {
        /* ADVERB     */ {NO_PASSIVE, NO_ACTION},
        /* OTHER_WORD */ {NO_PASSIVE, NO_ACTION},
        /* PARTICIPLE */ {NO_PASSIVE, NO_ACTION},
        /* BE_OR_GET  */ {AFTER_BE, START_HIGHLIGHT},
        /* IS_OR_WAS  */ {AFTER_BE, START_HIGHLIGHT},
        /* WH_WORD    */ {NO_PASSIVE, NO_ACTION},
    },
    /* SENTENCE_BEGIN */ {
        /* ADVERB     */ {SENTENCE_BEGIN, NO_ACTION},
        /* OTHER_WORD */ {NO_PASSIVE, NO_ACTION},
        /* PARTICIPLE */ {PARTICIPLE_FIRST, START_HIGHLIGHT},
        /* BE_OR_GET  */ {AFTER_BE, START_HIGHLIGHT},
        /* IS_OR_WAS  */ {QUESTION, START_HIGHLIGHT},
        /* WH_WORD    */ {SENTENCE_BEGIN, NO_ACTION},
    },
    /* PARTICIPLE_FIRST */ {
        /* ADVERB     */ {PARTICIPLE_FIRST, NO_ACTION},
        /* OTHER_WORD */ {NO_PASSIVE, NO_ACTION},
        /* PARTICIPLE */ {NO_PASSIVE, NO_ACTION},
        /* BE_OR_GET  */ {NO_PASSIVE, END_HIGHLIGHT},
        /* IS_OR_WAS  */ {NO_PASSIVE, END_HIGHLIGHT},
        /* WH_WORD    */ {NO_PASSIVE, NO_ACTION},
    },
    /* QUESTION */ {
        /* ADVERB     */ {QUESTION, NO_ACTION},
        /* OTHER_WORD */ {QUESTION, NO_ACTION},
        /* PARTICIPLE */ {NO_PASSIVE, END_HIGHLIGHT},
        /* BE_OR_GET  */ {NO_PASSIVE, NO_ACTION},
        /* IS_OR_WAS  */ {NO_PASSIVE, NO_ACTION},
        /* WH_WORD    */ {NO_PASSIVE, NO_ACTION},
    },
    /* AFTER_BE */ {
        /* ADVERB     */ {AFTER_BE, NO_ACTION},
        /* OTHER_WORD */ {NO_PASSIVE, NO_ACTION},
        /* PARTICIPLE */ {NO_PASSIVE, END_HIGHLIGHT},
        /* BE_OR_GET  */ {AFTER_BE, START_HIGHLIGHT},
        /* IS_OR_WAS  */ {AFTER_BE, START_HIGHLIGHT},
        /* WH_WORD    */ {NO_PASSIVE, NO_ACTION},
    },
};

/*
 * Word lists.  Each entry is the ending of a word.  A leading space means
 * the entry must be a whole word, and a leading apostrophe means the entry
 * must follow an apostrophe or a period (as in "I'm" or "wasn't").  Case is
 * ignored, an apostrophe and a period are treated as the same character, and
 * any character that is not a letter, an apostrophe or a period counts as a
 * space.  When several entries match a word, the longest one wins.
 */
static const char *const abbreviations[] = {
    " cf.", ".cf.", " dept.", ".dept.", " dr.", ".dr.", "..e.", " e.g.",
    ".e.g.", ".ef.", " eq.", ".eq.", ".f.", " fig.", ".fig.", "..g.", " i.e.",
    ".i.e.", ".ig.", " mr.", ".mr.", " mrs.", ".mrs.", " ms.", ".ms.", ".of.",
    " prof.", ".prof.", ".q.", " ref.", ".ref.", ".rof.", " vs.", ".vs.",
    " yr.", ".yr.",
    0
};

static const char *const adverbs[] = {
    " again", " ago", " also", " always", " better", "day", "etimes", " even",
    " ever", "ewhat", "ewhere", " far", " fast", " first", "gether", " half",
    " here", "iefly", " indeed", " instead", " just", " last", " later",
    " least", " little", " long", "ly", " maybe", "most", " much", " never",
    " next", " not", " now", " often", "owhere", " quite", " rather", "ready",
    "rhaps", "rther", " so", " soon", " then", " thus", " tomorrow", " too",
    " very", " well", " yet", "ywhere",
    0
};

static const char *const other_words[] = {
    " ally", "aly", "'amily", "bbly", " bed", "belly", "berlain", "billy",
    " biped", "'biped", "bread", "bsled", " bully", "'bully", "choly",
    "clown", "condone", "dally", " dilly", "'dilly", " doily", "'doily",
    "dread", "dred", "eed", "eloved", " embed", "'embed", "embly", "epiped",
    "'eply", "ersted", "espread", " family", "'family", " filly", "'filly",
    "flit", "fly", "frared", "fred", " ged", "'ged", "gfried", " gully",
    "'gully", "hmed", " homily", "'homily", " hurly", "'hurly", "ilately",
    " imply", "'imply", " jared", "'jared", " jelly", "'jelly", " lily",
    "'lily", "liven", "lred", "'mbed", "morbid", "moyed", "ncred", "neset",
    "ohamed", "ohammed", "olly", "omply", "onset", "orset", "orsted", "oset",
    " ought", "'ought", " ply", "poly", "pply", "rabid", " rally", "'rally",
    " red", " rely", "'rely", " reply", "'reply", " sacred", "'sacred",
    " sally", "'sally", " shed", "'shed", " shred", "'shred", " sled", "sset",
    " sully", "'sully", " tally", "'tally", " ted", " telly", "'telly",
    "thread", "threaten", "tiply", "token", "tread", "tred", "trunk", "umely",
    "urbid", "usset", " wed", "'wed", " welt", "'welt", "wheaten",
    0
};

static const char *const participles[] = {
    " agreed", " bent", " bet", "bid", "bitten", " born", " borne", "bound",
    "built", "burst", "bust", "cast", "caught", "clad", "clung", "cut", "'d",
    "done", "drawn", "dug", "ealt", "eamt", "eapt", "eaten", "ed", "egun",
    "fallen", "felt", "flung", " found", "gone", "got", "grown", " had",
    "haken", "heard", "held", "hewn", " hit", "hosen", "hown", "hrown",
    "hrust", "hung", "hurt", "idden", "isen", "iven", "kept", "knit", "known",
    "laden", "laid", " lain", "leant", "left", " lent", "lept", " let", "lit",
    "lost", "lown", "made", "meant", " met", "mown", "nelt", "nuck", "oilt",
    "oken", "otten", "ought", "paid", "pent", "pilt", "put", "rcome", "read",
    "reft", "rewn", "ricken", " rid", "ritten", "rlain", "rnt", "rodden",
    "roven", "rozen", "run", "rung", "runk", "said", "saken", "sat", "sawn",
    "seen", " sent", "set", "sewn", "shat", "shaven", "shone", "shorn",
    "shot", "shut", "slain", "slid", "slung", "sold", "sown", "spelt",
    "spent", "spun", "struck", "stuck", "stung", "sung", "sunk", "swum",
    "taken", "taught", "told", "tolen", "tood", "torn", "uit", "welt", "wept",
    "wollen", "won", "worn", "wound", "woven", "wung",
    0
};

static const char *const be_and_get_forms[] = {
    " be", "'be", " been", "'been", " bein'", "'bein'", " being", "'being",
    " get", "'get", " gettin'", "'gettin'", " getting", "'getting", " got",
    "'got", " gotten", "'gotten", "hat's", " he's", "'he's", "here's",
    " how's", "'how's", " it's", "'it's", "'m", "'re", " she's", "'she's",
    " when's", "'when's", " which's", "'which's", " who's", "'who's",
    " why's", "'why's",
    0
};

static const char *const is_and_was_forms[] = {
    " ain't", "'ain't", " am", "'am", " are", "'are", " is", "'is", " isn't",
    "'isn't", "ren't", " was", "'was", " wasn't", "'wasn't", " were", "'were",
    0
};

static const char *const wh_words[] = {
    " ", "'", " by", "'by", " for", "'for", " from", "'from", " have",
    "'have", " how", "'how", " of", "'of", " that", "'that", " to", "'to",
    " what", "'what", " when", "'when", " where", "'where", " which",
    "'which", " who", "'who", " whose", "'whose", " why", "'why",
    0
};


static const char *const *const word_lists[] = {
    adverbs, other_words, participles, be_and_get_forms, is_and_was_forms,
    wh_words
};

/* Highlight marks: inverse video on a terminal, brackets otherwise. */
static const char *highlight_open;
static const char *highlight_close;
static int highlight_length;

static unsigned char *cursor = text + 1;   /* character just read */
static unsigned char *token_start;         /* end of the previous token */
static unsigned char *sentence_start;      /* first character not yet printed */
static unsigned char *highlight_start;     /* start of a passive construction */
static int sentence_state = OUTSIDE_SENTENCE;
static int passive_state = SENTENCE_BEGIN;
static int sentence_is_passive;            /* part of the sentence was printed */
static int found_passive;                  /* exit status */

/* Reduce a character to the alphabet that the word lists are written in. */
static int fold(int ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 'a';
    if (ch >= 'a' && ch <= 'z')
        return ch;
    if (ch == '\'' || ch == '.')
        return '\'';
    return ' ';
}

/*
 * Return the length of the longest entry in list that matches the text that
 * ends just before end, or 0 if none matches.  Characters are compared from
 * the end backwards.
 */
static int longest_match(const char *const *list, const unsigned char *end)
{
    int best = 0;

    for (; *list != 0; list++) {
        const char *entry = *list;
        int length = 0;
        int i;

        while (entry[length] != '\0')
            length++;
        for (i = 1; i <= length; i++)
            if (fold(end[-i]) != fold(entry[length - i]))
                break;
        if (i > length && length > best)
            best = length;
    }
    return best;
}

static int is_abbreviation(const unsigned char *end)
{
    return longest_match(abbreviations, end) > 0;
}

static int classify_word(const unsigned char *end)
{
    int best = 0;
    int word_class = OTHER_WORD;
    int i;

    for (i = 0; i < (int)(sizeof word_lists / sizeof word_lists[0]); i++) {
        int length = longest_match(word_lists[i], end);
        if (length > best) {
            best = length;
            word_class = i;
        }
    }
    return word_class;
}

static int char_class(int ch)
{
    if (ch == '\0')
        return NUL_CHAR;
    if (ch == '\n')
        return LINE_BREAK;
    if (ch <= ' ')
        return BLANK;
    if (ch == '.')
        return DOT;
    if (ch == '!' || ch == '?')
        return BANG;
    if (ch == '\'' || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z'))
        return LOWER;
    if ((ch >= 'A' && ch <= 'Z') || ch >= 128)
        return UPPER;
    return OTHER_PUNCT;
}

/* Read the next character and return the event it causes. */
static int next_sentence_event(void)
{
    const struct sentence_transition *t;

    cursor++;
    t = &sentence_machine[sentence_state][char_class(*cursor)];
    sentence_state = t->next_state;
    return t->event;
}

/* Feed the word that ends at the cursor into the passive machine. */
static void next_word(void)
{
    const struct passive_transition *t;

    t = &passive_machine[passive_state][classify_word(cursor)];
    passive_state = t->next_state;

    if (t->action == START_HIGHLIGHT) {
        while (*token_start <= ' ')
            token_start++;
        highlight_start = token_start;
    } else if (t->action == END_HIGHLIGHT) {
        found_passive = sentence_is_passive = 1;
        write(1, sentence_start, highlight_start - sentence_start);
        write(1, highlight_open, highlight_length);
        write(1, highlight_start, cursor - highlight_start);
        write(1, highlight_close, highlight_length);
        sentence_start = cursor;
    }
}

static void handle_event(int event)
{
    switch (event) {
    case END_OF_TEXT:
    case SENTENCE_START:
        sentence_start = token_start = cursor;
        passive_state = SENTENCE_BEGIN;
        break;

    case SENTENCE_END:
        /*
         * Step back to just after the last visible character.  The skipped
         * characters are read again, so the character that ended the
         * sentence can also start the next one.
         */
        do
            cursor--;
        while (*cursor <= ' ');
        cursor++;
        if (sentence_is_passive) {
            write(1, sentence_start, cursor - sentence_start);
            write(1, "\n", 1);
            sentence_is_passive = 0;
        }
        break;

    case TOKEN_END:
        /* A period after an abbreviation does not end the sentence. */
        if (is_abbreviation(cursor))
            sentence_state = IN_PUNCTUATION;
        next_word();
        token_start = cursor;
        break;
    }
}

int main()
{
    int event;

    read(0, text + 2, MAX_INPUT);
    if (isatty(1)) {
        highlight_open = "\33[7m[";
        highlight_close = "]\33[0m";
        highlight_length = 5;
    } else {
        highlight_open = "[";
        highlight_close = "]";
        highlight_length = 1;
    }

    for (event = NO_EVENT; event != END_OF_TEXT; event = next_sentence_event())
        handle_event(event);

    return found_passive;
}
