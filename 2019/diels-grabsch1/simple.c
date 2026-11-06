#include <stdio.h>

/*
 * LZW compressor writing the classic UNIX compress(1) ".Z" format.
 */

#define MAGIC_1     0x1F        /* .Z magic number, first byte */
#define MAGIC_2     0x9D        /* .Z magic number, second byte */
#define BLOCK_MODE  0x80        /* header flag: block mode */
#define MAX_BITS    16          /* largest code width */
#define INIT_BITS   9           /* starting code width */
#define FIRST_FREE  257         /* 0-255 are bytes, 256 is CLEAR (never used) */
#define HASH_SIZE   69001       /* hash table size used by compress(1) */
#define EMPTY       (-1)

/* hash table: each slot holds (prefix << 8 | byte) << 16 | code */
static long long table[HASH_SIZE];

static unsigned long bit_buffer = 0;   /* bits not yet written */
static int bit_count = 0;              /* number of bits in bit_buffer */

/* append a code of the given width, least significant bit first */
static void put_code(long long code, int bits)
{
    bit_buffer |= (unsigned long)code << bit_count;
    bit_count += bits;
    while (bit_count >= 8) {
        putchar(bit_buffer & 0xFF);
        bit_buffer >>= 8;
        bit_count -= 8;
    }
}

/* write out any leftover bits as a final partial byte */
static void flush_bits(void)
{
    if (bit_count > 0)
        putchar(bit_buffer & 0xFF);
}

int main()
{
    long long prefix;           /* code of the string matched so far */
    long long ch;               /* next input byte */
    long long key;              /* prefix and byte packed together */
    long long step;             /* hash value, also the probe step */
    long long slot;             /* current hash table slot */
    long long next_code = FIRST_FREE;
    int bits = INIT_BITS;       /* current code width */
    int table_full = 0;         /* no more codes can be added */
    int found;

    for (slot = 0; slot < HASH_SIZE; slot++)
        table[slot] = EMPTY;

    /* .Z header */
    putchar(MAGIC_1);
    putchar(MAGIC_2);
    putchar(BLOCK_MODE | MAX_BITS);

    if ((prefix = getchar()) < 0)
        return 0;

    while ((ch = getchar()) >= 0) {
        step = slot = ch << 8 ^ prefix;
        key = prefix << 8 | ch;

        /* only re-check the code width at the start of a new string */
        if (prefix < 256) {
            if (bits < MAX_BITS) {
                if (next_code > (1L << bits))
                    bits++;
            } else if (next_code >= (1L << MAX_BITS)) {
                table_full = 1;
            }
        }

        /* double hashing: probe until the key or an empty slot is found */
        while (table[slot] != EMPTY && table[slot] >> 16 != key)
            slot = (slot + step + 1) % HASH_SIZE;

        found = table[slot] != EMPTY;
        if (found) {
            /* string + byte is known, keep extending it */
            prefix = table[slot] & 0xFFFF;
        } else {
            /* output the longest known string, start a new one */
            put_code(prefix, bits);
            if (!table_full)
                table[slot] = key << 16 | next_code++;
            prefix = ch;
        }
    }

    put_code(prefix, bits);
    flush_bits();

    return 0;
}
