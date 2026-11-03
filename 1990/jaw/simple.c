#include <stdio.h>

/*
 * One program, two tools.  The name it is run under picks the tool:
 *
 *   atob  - turn btoa's base-85 text back into binary, checking the
 *           length and the three checksums on the "xbtoa End" line
 *   zcat  - undo the LZW compression done by compress(1) (.Z files)
 *
 * Both read standard input and write standard output.
 */

#define CLEAR_CODE 256      /* block-mode code that resets the dictionary */
#define FIRST_CODE 256      /* first dictionary entry after the 256 bytes */
#define MAX_BITS_LIMIT 16   /* largest code width this decoder supports */
#define TABLE_SIZE 69001    /* compress's hash table size, used for prefixes */
#define BUFFER_SIZE (256 * 256)
#define LOW32(x) ((x) & 4294967295UL)

static int fail(void)
{
    fprintf(stderr, "oops\n");
    return 1;
}

/* ------------------------------------------------------------------ atob */

static unsigned long xor_sum;   /* "E" check: XOR of all bytes */
static unsigned long add_sum;   /* "S" check: sum of (byte + 1) */
static unsigned long rot_sum;   /* "R" check: rotate left 1 bit, then add */

static void checksum_byte(int byte)
{
    xor_sum ^= byte;
    add_sum += byte + 1;
    rot_sum = rot_sum * 2 + byte + (rot_sum >> 31 & 1);
}

/* Write bytes 3 down to `last` of a 4-byte group, high byte first. */
static void put_group(unsigned long group, int last)
{
    int k;

    for (k = 3; k >= last; k--)
        checksum_byte(putchar(group >> k * 8 & 255));
}

static int atob(void)
{
    unsigned long group = 0;    /* value of the base-85 digits read so far */
    int digits = 0;             /* how many digits are in `group` */
    int ch, padding;
    long length;
    unsigned long length_hex, want_xor, want_add, want_rot;

    /* skip the "xbtoa Begin" line */
    while (getchar() != '\n')
        ;

    /* decode until EOF or the 'x' of the "xbtoa End" line */
    while ((ch = getchar()) != EOF && ch != 'x') {
        if (ch == '\n')
            continue;
        if (digits > 4) {
            put_group(group, 0);
            group = 0;
            digits = 0;
        }
        if (ch > ' ' && ch < 'v') {
            group = group * 85 + (ch - '!');
            digits++;
        } else if (ch == 'z' && digits == 0) {
            group = 0;          /* 'z' is shorthand for four zero bytes */
            digits = 5;
        } else {
            return fail();
        }
    }

    /* xbtoa End N <length> <length in hex> E <xor> S <sum> R <rot> */
    while (getchar() != 'N')
        ;
    if (scanf("%ld%lx E%lx S%lx R%lx ", &length, &length_hex,
              &want_xor, &want_add, &want_rot) != 5)
        return fail();

    /* The last group was padded with zeros: only write the real bytes,
     * but the padding still counts toward the checksums. */
    if (length) {
        padding = 3 - ((length - 1) & 3);
        put_group(group, padding);
        while (padding--)
            checksum_byte(0);
    }

    if (LOW32(length) != LOW32(length_hex) || LOW32(want_xor) != LOW32(xor_sum) ||
        LOW32(want_add) != LOW32(add_sum) || LOW32(want_rot) != LOW32(rot_sum))
        return fail();
    return 0;
}

/* ------------------------------------------------------------------ zcat */

static int prefix[TABLE_SIZE];              /* code this entry extends */
static unsigned char suffix[BUFFER_SIZE];   /* byte this entry adds */
/* bytes 0..15 hold the input chunk; from 256 up is the output stack */
static unsigned char buffer[BUFFER_SIZE];

static int max_bits;        /* widest code allowed, from the header */
static int code_bits;       /* current code width */
static long max_code;       /* largest code that fits in code_bits */
static long free_entry;     /* next unused dictionary slot */
static int bit_offset;      /* position of the next code in buffer */
static int bit_limit;       /* codes must start before this bit */

/*
 * compress writes codes in chunks of 8 codes (= code_bits bytes).  When the
 * code width changes, the rest of the chunk is padding, so throw it away and
 * read a fresh chunk at the new width.
 */
static long read_code(void)
{
    unsigned char *p;
    int shift, refill = bit_offset >= bit_limit;

    if (free_entry > max_code && code_bits < max_bits) {
        code_bits++;
        max_code = (1L << code_bits) - 1;
        refill = 1;
    }
    if (refill) {
        bit_offset = 0;
        bit_limit = fread(buffer, 1, code_bits, stdin) * 8;
        if (bit_limit < 8)
            return -1;
        bit_limit -= code_bits - 1;
    }

    /* codes are packed least significant bit first */
    p = buffer + bit_offset / 8;
    shift = bit_offset & 7;
    bit_offset += code_bits;
    return p[0] >> shift | (max_code & (p[1] | p[2] << 8) << (8 - shift));
}

static int zcat(void)
{
    unsigned char *stack = buffer + 256;
    long code, old_code, in_code;
    int first_byte, block_mode, flags;

    getchar();          /* magic number 0x1f 0x9d, not checked */
    getchar();
    flags = getchar();
    block_mode = flags >> 7 & 1;
    max_bits = flags & 31;
    if (max_bits > MAX_BITS_LIMIT)
        return fail();

    for (code = 255; code >= 0; code--) {
        prefix[code] = 0;
        suffix[code] = code;
    }
    code_bits = 8;
    free_entry = FIRST_CODE + block_mode;   /* block mode reserves 256 */

    first_byte = old_code = read_code();
    if (old_code < 0)
        return 1;
    putchar(first_byte);

    while ((code = read_code()) != -1) {
        if (code == CLEAR_CODE && block_mode) {
            for (code = 255; code >= 0; code--)
                prefix[code] = 0;
            /* max_code below free_entry makes read_code restart at 9 bits */
            max_code = code_bits = 8;
            free_entry = FIRST_CODE;
            if ((code = read_code()) < 0)
                return 0;
        }
        in_code = code;

        /* code not in the table yet: it is old string + its own first byte */
        if (code >= free_entry) {
            *stack++ = first_byte;
            code = old_code;
        }
        /* walk the chain back to a single byte, stacking the bytes */
        while (code >= 256) {
            *stack++ = suffix[code];
            code = prefix[code];
        }
        first_byte = suffix[code];
        putchar(first_byte);
        while (stack > buffer + 256)
            putchar(*--stack);

        /* new entry: previous string + first byte of this one */
        if (free_entry < 1L << max_bits) {
            prefix[free_entry] = old_code;
            suffix[free_entry] = first_byte;
            free_entry++;
        }
        old_code = in_code;
    }
    return 0;
}

int main(int argc, char **argv)
{
    (void)argc;
    if (argv[0][0] != 'a')
        return zcat();
    return atob();
}
