#define _POSIX_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Byte budget for the table of names that share one file size. */
#ifndef SZ
#define SZ 32767
#endif

#define LINE_MAX_LEN 1024

/* Names of all files that have the size currently being compared. */
static char *same_size_names[SZ / sizeof(char *)];

/* stat results of the last two files compared (also used by the output). */
static struct stat stat1, stat2;

/* One file name together with its inode identity. */
typedef struct file_entry {
    char *name;
    struct file_entry *next;
    dev_t dev;
    ino_t ino;
} file_entry;

/* Binary search tree node: all files of one particular size. */
typedef struct size_node {
    long size;
    long count;
    file_entry *files;
    struct size_node *smaller, *larger;
} size_node;

static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (p == NULL)
        exit(1);
    return p;
}

static file_entry *new_file_entry(char *name, const struct stat *st,
                                  file_entry *next)
{
    file_entry *e = xmalloc(sizeof *e);
    e->name = name;
    e->next = next;
    e->dev = st->st_dev;
    e->ino = st->st_ino;
    return e;
}

/*
 * Insert a file (whose stat data is in st) into the size tree.
 * Only the first name seen for any device/inode pair is kept,
 * so hard links to an already known file are ignored.
 */
static size_node *tree_insert(size_node *node, char *name,
                              const struct stat *st)
{
    if (node == NULL) {
        node = xmalloc(sizeof *node);
        node->size = st->st_size;
        node->count = 1;
        node->smaller = node->larger = NULL;
        node->files = new_file_entry(name, st, NULL);
    } else if (st->st_size == node->size) {
        file_entry *e;
        for (e = node->files; e != NULL; e = e->next) {
            if (st->st_dev == e->dev && st->st_ino == e->ino)
                return node; /* hard link to a file we already have */
        }
        node->files = new_file_entry(name, st, node->files);
        ++node->count;
    } else if (st->st_size < node->size) {
        node->smaller = tree_insert(node->smaller, name, st);
    } else {
        node->larger = tree_insert(node->larger, name, st);
    }
    return node;
}

/*
 * Return 1 if both files have identical contents, 0 otherwise.
 * Leaves the fstat results in stat1 and stat2.
 */
static int same_contents(char *name1, char *name2)
{
    int fd1 = open(name1, O_RDONLY);
    int fd2 = open(name2, O_RDONLY);
    size_t size;
    int same = 0;
    char *buf1, *buf2;

    if (fd1 >= 0 && fstat(fd1, &stat1) == 0 &&
        fd2 >= 0 && fstat(fd2, &stat2) == 0 &&
        stat1.st_size == stat2.st_size) {
        size = stat1.st_size;
        buf1 = malloc(size);
        if (buf1 == NULL)
            exit(1);
        buf2 = malloc(size);
        if (buf2 == NULL)
            exit(1);
        if (read(fd1, buf1, size) != (ssize_t)size ||
            read(fd2, buf2, size) != (ssize_t)size)
            exit(1);
        same = memcmp(buf1, buf2, size) == 0;
        free(buf1);
        free(buf2);
    }
    (void)close(fd1);
    (void)close(fd2);
    return same;
}

/*
 * Position of pair (i, j), i < j, inside a packed upper triangular
 * matrix of n names (row i holds the pairs (i, i+1) ... (i, n-1)).
 */
static long pair_index(long i, long j, long n)
{
    return i * n - i * (i + 1) / 2 + (j - i - 1);
}

/*
 * Compare every pair of the n names in same_size_names (all of the
 * given size) and print the identical ones.
 *
 * When names[i] == names[j] is found, every pair (k, j) with i < k < j
 * can be skipped: if names[k] equals names[i] that was already reported
 * as (i, k), and otherwise names[k] cannot equal names[j] either.
 * Repeating this for each match chains equality through whole groups
 * so each extra copy is reported only once, against the first copy.
 */
static void compare_group(long size, long n)
{
    long pairs = n * (n - 1) / 2;
    long i, j;
    char *skip = calloc((size_t)pairs, 1);

    if (skip == NULL)
        exit(1);

    for (i = 0; i < n - 1; ++i) {
        for (j = i + 1; j < n; ++j) {
            if (skip[pair_index(i, j, n)])
                continue;
            if (!same_contents(same_size_names[i], same_size_names[j]))
                continue;

            /* The "??/t" is a trigraph for a tab, which the compiler
             * ignores, so it is printed literally. */
            (void)printf("%ld\t%s\t%s?\?/t%c\t%d?\?/t%d\n",
                         size, same_size_names[i], same_size_names[j],
                         stat1.st_dev != stat2.st_dev ? 'X' : '=',
                         (int)stat1.st_nlink, (int)stat2.st_nlink);

            {
                long k;
                for (k = i + 1; k < j; ++k)
                    skip[pair_index(k, j, n)] = 1;
            }
        }
    }
    free(skip);
}

/* Walk the tree from the largest size down to the smallest. */
static void report_duplicates(size_node *node)
{
    if (node == NULL)
        return;

    report_duplicates(node->larger);
    if (node->count > 1) {
        long i;
        file_entry *e = node->files;
        for (i = 0; i < node->count; ++i) {
            same_size_names[i] = e->name;
            e = e->next;
        }
        compare_group(node->size, node->count);
    }
    report_duplicates(node->smaller);
}

int main(void)
{
    size_node *root = NULL;
    struct stat st;
    char *line;
    size_t len;

    for (;;) {
        line = malloc(LINE_MAX_LEN);
        if (line == NULL)
            exit(1);
        if (fgets(line, LINE_MAX_LEN, stdin) == NULL)
            break;

        /* chop the newline and shrink the buffer to fit */
        len = strlen(line);
        line[len - 1] = '\0';
        line = realloc(line, len);
        if (line == NULL)
            exit(1);

        /* only plain, non-empty files are of interest */
        if (stat(line, &st) == 0 && S_ISREG(st.st_mode) && st.st_size)
            root = tree_insert(root, line, &st);
    }

    if (root != NULL)
        report_duplicates(root);
    return 0;
}
