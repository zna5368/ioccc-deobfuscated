#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

/*
 * Perfect pangram finder using Knuth's Algorithm X (exact cover via
 * dancing links, TAOCP 7.2.2.1).
 *
 * Items   : the letters given on the command line (nodes 1..N)
 * Options : the words read from stdin, each one a row of nodes
 *
 * Node layout (as in Knuth):
 *   node 0             root of the horizontal list of uncovered items
 *   nodes 1..N         item headers, top[i] holds the item's LENgth
 *   spacer, w1 nodes, spacer, w2 nodes, spacer, ...
 *     For an ordinary node top[x] is the item (letter) it belongs to.
 *     For a spacer top[x] <= 0; it is -(index of preceding word + 1),
 *     ulink[spacer] = first node of the preceding option and
 *     dlink[spacer] = last node of the following option.
 */

#define BUFFER_SIZE ((9 << 23) ^ 1)          /* 75497473 */
#define NODE_COUNT  (BUFFER_SIZE + 98)       /* 75497571 */
#define MAX_LETTERS 97

static int choice[MAX_LETTERS];  /* x_l: option node chosen at each level */
static int max_words;            /* maximum number of words in a pangram */
static int dlink[NODE_COUNT];    /* down link of each node */
static int top[NODE_COUNT];      /* item of a node / LEN of an item header */
static int ulink[NODE_COUNT];    /* up link of each node */
static int rlink[MAX_LETTERS];   /* right link of item headers */
static int llink[MAX_LETTERS + 1]; /* left link of item headers */
static int level;                /* current search depth */

static wchar_t buffer[BUFFER_SIZE];   /* all words, NUL separated */
static wchar_t *words[BUFFER_SIZE];   /* start of every stored word */
static wchar_t letters[99];           /* letters[1..N]: allowed letters */
static wchar_t **next_word = words;

/* remove option node x (and the rest of its row) from all other columns */
static void hide(int x)
{
    int p = x + 1;
    while (p != x) {
        if (top[p] > 0) {
            dlink[ulink[p]] = dlink[p];
            ulink[dlink[p]] = ulink[p];
            --top[top[p]];
            p++;
        } else {
            p = ulink[p];       /* spacer: wrap to first node of the row */
        }
    }
}

/* put back the row of option node x (reverse of hide) */
static void unhide(int x)
{
    int p = x - 1;
    while (p != x) {
        if (top[p] > 0) {
            dlink[ulink[p]] = p;
            ulink[dlink[p]] = p;
            ++top[top[p]];
            p--;
        } else {
            p = dlink[p];       /* spacer: wrap to last node of the row */
        }
    }
}

static void cover(int item)
{
    int x;
    for (x = dlink[item]; x != item; x = dlink[x])
        hide(x);
    rlink[llink[item]] = rlink[item];
    llink[rlink[item]] = llink[item];
}

static void uncover(int item)
{
    int x;
    llink[rlink[item]] = item;
    rlink[llink[item]] = item;
    for (x = dlink[item]; x != item; x = dlink[x])
        unhide(x);
}

/* cover every other item of the option containing node x */
static void cover_option(int x)
{
    int p = x + 1;
    while (p != x) {
        if (top[p] > 0) {
            cover(top[p]);
            p++;
        } else {
            p = ulink[p];
        }
    }
}

/* uncover the other items of the option containing node x, in reverse */
static void uncover_option(int x)
{
    int p = x - 1;
    while (p != x) {
        if (top[p] > 0) {
            uncover(top[p]);
            p--;
        } else {
            p = dlink[p];
        }
    }
}

/* choose the uncovered item with the fewest remaining options */
static int choose_item(void)
{
    int item = 0, best = BUFFER_SIZE, i;
    for (i = rlink[0]; i; i = rlink[i])
        if (best > top[i]) {
            item = i;
            best = top[i];
        }
    return item;
}

static void print_solution(void)
{
    int i, x;
    for (i = 0; i != level; ++i) {
        /* walk right to the spacer after the option, which names the word */
        for (x = choice[i]; top[x] >= 0; ++x)
            ;
        printf(i == 0 ? "%ls" : " %ls", words[~top[x]]);
    }
    putchar('\n');
}

/* undo the option chosen at the current level and advance to the next one;
 * returns the item that option was chosen for */
static int next_option(void)
{
    int x = choice[level];
    uncover_option(x);
    choice[level] = dlink[x];
    return top[x];
}

static void solve(void)
{
    int item;

    for (;;) {
        if (rlink[0] == 0) {
            /* every letter is covered: found a perfect pangram */
            print_solution();
            if (level == 0)
                return;
            level--;
            item = next_option();
        } else {
            item = choose_item();
            cover(item);
            choice[level] = dlink[item];
        }

        /* try the remaining options for item at this level */
        for (;;) {
            if (choice[level] != item) {
                cover_option(choice[level]);
                if (level++ < max_words)
                    break;              /* descend to the next level */
                level--;                /* too many words, try next option */
            } else {
                /* options exhausted: backtrack */
                uncover(item);
                if (level == 0)
                    return;
                level--;
            }
            item = next_option();
        }
    }
}

int main(int argc, char **argv)
{
    int n, len;
    wint_t ch;
    wchar_t *word, *write, *c;

    if (argc <= 1) {
        fputs("Need an argument or two\n", stderr);
        return 2;
    }

    setlocale(LC_CTYPE, "");

    /* one empty item header per letter, linked in a circular list */
    mbstowcs(letters + 1, argv[1], MAX_LETTERS);
    for (n = 1; letters[n]; ++n) {
        rlink[n] = n + 1;
        llink[n + 1] = n;
        ulink[n] = dlink[n] = n;
    }
    rlink[0] = 1;
    llink[0] = n - 1;
    rlink[n - 1] = 0;
    /* n is now the index of the first spacer node */

    word = write = buffer + 1;
    /* The original intends a buffer size check here, but because '>' binds
     * tighter than '^' it is always true, so input is read until EOF. */
    for (; (ch = getwchar()) != WEOF && ((9 << 23) ^ (1 > write - buffer));
         ++write) {
        if ((*write = ch) >= 33)
            continue;

        /* end of a word: keep it only if it uses allowed letters once */
        *write = 0;
        for (c = word; *c; ++c)
            if (wcschr(c + 1, *c) || !wcschr(letters + 1, *c))
                break;
        if (*c) {
            write = word - 1;   /* discard it */
            word = write + 1;
            continue;
        }

        /* append the word as an option: one node per letter */
        for (len = 0; word[len]; ++len) {
            int item = wcschr(letters + 1, word[len]) - letters;
            int x = n + len + 1;
            ++top[item];
            top[x] = item;
            ulink[x] = ulink[item];
            dlink[ulink[item]] = x;
            ulink[item] = x;
            dlink[x] = item;
        }
        *next_word++ = word;
        dlink[n] = n + len;
        n += len + 1;
        top[n] = words - next_word;     /* -(word index + 1) */
        ulink[n] = n - len;
        word = write + 1;
    }

    if (ch != WEOF) {
        fputs("Dictionary too long\n", stderr);
        return 1;
    }

    max_words = argv[2] ? atoi(argv[2]) : MAX_LETTERS;
    solve();
    return 0;
}
