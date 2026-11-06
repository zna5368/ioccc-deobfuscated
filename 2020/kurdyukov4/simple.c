#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_OUTPUT      (1 << 20)   /* at most 1 MiB of output */
#define MAX_CONTEXT     128         /* context length must be 1..128 */
#define MAX_INPUT       (1 << 29)   /* input file must be under 512 MiB */
#define EMPTY_SLOT      0xFFFFFFFFu /* unused hash table entry */
#define HASH_MULTIPLIER 79764919u   /* 0x04C11DB7, the CRC-32 polynomial */

/* Random number generator state (multiplicative congruential generator). */
static uint64_t rng_state;
static uint64_t rng_multiplier;

/* Return a pseudo-random number in the range [0, range). */
static uint32_t random_below(uint32_t range)
{
    uint32_t r;

    rng_state *= rng_multiplier;
    r = (uint32_t)(rng_state >> 31);
    return (uint32_t)(((uint64_t)r * range) >> 32);
}

/* Hash the context_len bytes starting at text. */
static uint32_t hash_context(const uint8_t *text, int context_len)
{
    uint32_t hash = 0;
    int i;

    for (i = 0; i < context_len; i++)
        hash = (hash ^ text[i]) * HASH_MULTIPLIER;
    return hash;
}

/*
 * Clean up the text in place: drop underscores, replace <markup> tags by a
 * space and squeeze every run of whitespace into a single space.
 * Returns the new length.
 */
static int filter_text(uint8_t *text, int size)
{
    int in = 0, out = 0;
    int last = 0;
    uint32_t ch;

    while (in < size) {
        ch = text[in++];
        if (ch == '_')
            continue;
        if (ch == '<') {
            /* skip up to and including the closing '>' */
            ch = ' ';
            while (in < size && text[in++] != '>')
                ;
        }
        if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
            text[out++] = last = ch;
        } else if (last != ' ') {
            text[out++] = last = ' ';
        }
    }
    return out;
}

int main(int argc, char **argv)
{
    int output_len, context_len;
    int size, text_len;
    int pos, i, k, matches;
    uint32_t mask, table_size, hash;
    uint32_t *table;
    uint32_t next_char[256][2]; /* [c][0] = count, [c][1] = last position */
    uint8_t *text;
    FILE *file;

    if (argc != 5)
        return 1;

    output_len = atoi(argv[2]);
    context_len = atoi(argv[3]);
    rng_state = atoi(argv[4]);

    if (output_len < 0 || output_len >= MAX_OUTPUT)
        return 1;
    if (context_len < 1 || context_len > MAX_CONTEXT)
        return 1;
    file = fopen(argv[1], "rb");
    if (!file)
        return 1;
    if (fseek(file, 0, SEEK_END))
        return 1;
    size = ftell(file);
    if (context_len > size)
        return 1;
    if (fseek(file, 0, SEEK_SET))
        return 1;
    text = malloc(size + context_len);
    if (!text)
        return 1;
    if (fread(text, 1, size, file) - size)
        return 1;
    if (size >= MAX_INPUT)
        return 1;
    fclose(file);

    /* multiplier = largest power of 5 that fits in 64 bits (5^27) */
    rng_multiplier = 5;
    while (rng_multiplier * 5 > rng_multiplier)
        rng_multiplier *= 5;

    text_len = filter_text(text, size);

    /* make the text circular: append its first context_len bytes */
    memcpy(text + text_len, text, context_len);

    /* hash table size: smallest power of two >= 3 * text_len */
    table_size = 1;
    while ((uint32_t)(text_len * 3) > table_size)
        table_size += table_size;

    table = malloc(4 * table_size);
    if (!table)
        return 1;
    memset(table, -1, 4 * table_size);
    mask = table_size - 1;

    /* index every context position with open addressing (linear probing) */
    for (k = 0; k < text_len; k++) {
        hash = hash_context(text + k, context_len);
        while (table[hash & mask] != EMPTY_SLOT)
            hash++;
        table[hash & mask] = k;
    }

    /* start at a random position in the text */
    pos = random_below(text_len);

    while (output_len--) {
        /* collect every character that follows an occurrence of the context */
        memset(next_char, 0, sizeof next_char);
        matches = 0;
        hash = hash_context(text + pos, context_len);
        while ((i = table[hash & mask]) != (int)EMPTY_SLOT) {
            if (!memcmp(text + pos, text + i, context_len)) {
                k = text[i + context_len];
                next_char[k][0]++;
                next_char[k][1] = i;
                matches++;
            }
            hash++;
        }

        /* pick a following character weighted by how often it occurs */
        matches = random_below(matches);
        for (k = -1; matches >= 0;)
            matches -= next_char[++k][0];
        putchar(k);

        /* continue from just after the chosen occurrence, wrapping around */
        pos = next_char[k][1] + 1;
        if (pos == text_len)
            pos = 0;
    }
    putchar('\n');

    /* the original's "return !!c" always yields 1 */
    return 1;
}
