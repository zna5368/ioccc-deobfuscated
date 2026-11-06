#include <stdio.h>

typedef unsigned char byte;
typedef unsigned long long uint64;

/*
 * prog.c is 1408 bytes long = 11 SHA-512 blocks of 128 bytes.  The first 10
 * blocks (the first 1280 bytes, up to and including the opening quote of the
 * string below) were hashed ahead of time.  The string holds:
 *
 *   bytes   0..103  the 8 intermediate hash words after block 10, each written
 *                   as 13 base-31 digits ('`' = 0 ... '~' = 30)
 *   bytes   0..124  the raw text of block 11 (the string's own contents)
 *   bytes 104..116  also read as 13 base-31 digits giving the last 8 bytes of
 *                   block 11, i.e. string bytes 120..124 followed by "\";\n"
 */
static const byte *tail_block = (const byte *)
"crsmyiajqhwy{unwa|hjoi`hlxhpxrzb~edko~rtr~ileqyjk`znqgsuitvgqnfdfa||wedvnmhozkpokootqzcexeld~oibqzpcsuw{ib{x`m`hsa`jmn}wcfzpb";

#define TOTAL_LENGTH_BITS (1408 * 8) /* 11264 */

static uint64 rotr(uint64 x, uint64 n)
{
    return x >> n | x << (64 - n);
}

/* SHA-512 logical functions */
static uint64 small_sigma0(uint64 x) { return rotr(x, 1) ^ rotr(x, 8) ^ (x >> 7); }
static uint64 small_sigma1(uint64 x) { return rotr(x, 19) ^ rotr(x, 61) ^ (x >> 6); }
static uint64 big_sigma0(uint64 x) { return rotr(x, 28) ^ rotr(x, 34) ^ rotr(x, 39); }
static uint64 big_sigma1(uint64 x) { return rotr(x, 14) ^ rotr(x, 18) ^ rotr(x, 41); }
static uint64 choose(uint64 x, uint64 y, uint64 z) { return (x & y) ^ (~x & z); }
static uint64 majority(uint64 x, uint64 y, uint64 z) { return ((x ^ y) & z) ^ (x & y); }

/* read 8 bytes of the string as a big-endian 64-bit word */
static uint64 read_word(uint64 index)
{
    uint64 word = 0;
    for (int i = 0; i < 8; i++)
        word = word * 256 + tail_block[index * 8 + i];
    return word;
}

/* decode 13 characters of the string as a base-31 number (wraps mod 2^64) */
static uint64 decode_base31(uint64 index)
{
    uint64 value = 0;
    for (int i = 0; i < 13; i++)
        value = value * 31 + tail_block[index * 13 + i] - 96;
    return value;
}

/*
 * Approximate cube of a fixed point number split into three 24-bit digits:
 *   x = hi + mid * 2^-24 + lo * 2^-48     (hi has 16 fractional bits)
 * Result is x^3 scaled so that the integer part sits above bit 48.
 * Low order terms are dropped or truncated; this is just accurate enough for
 * the first 80 primes.
 */
static uint64 approx_cube(uint64 hi, uint64 mid, uint64 lo)
{
    uint64 low_terms = ((3 * (hi * lo + mid * mid) >> 9) + (3 * mid * lo >> 32)) * hi >> 21;
    uint64 mid_terms = 3 * hi * hi * mid >> 6;
    uint64 mid_cubed = (mid >> 4) * (mid >> 4) * mid >> 46;
    return ((low_terms + mid_terms + mid_cubed) >> 18) + hi * hi * hi;
}

/* is (integer_part + fraction / 2^64)^3 below prime? */
static uint64 cube_is_below(uint64 prime, uint64 integer_part, uint64 fraction)
{
    uint64 hi = (integer_part << 16) | (fraction >> 48);
    uint64 mid = (fraction >> 24) % (1 << 24);
    uint64 lo = fraction % (1 << 24);
    return approx_cube(hi, mid, lo) >> 48 < prime;
}

/* integer part (0..7) of the cube root, found by bisection */
static uint64 cube_root_integer(uint64 prime)
{
    uint64 root = 0;
    for (uint64 bit = 4; bit; bit /= 2)
        if (cube_is_below(prime, root | bit, root))
            root |= bit;
    return root;
}

/* first 64 fractional bits of the cube root, found by bisection */
static uint64 cube_root_fraction(uint64 prime)
{
    uint64 integer_part = cube_root_integer(prime);
    uint64 fraction = 0;
    for (uint64 bit = 1ull << 63; bit; bit /= 2)
        if (cube_is_below(prime, integer_part, fraction | bit))
            fraction |= bit;
    return fraction;
}

static uint64 is_prime(uint64 n)
{
    if (n < 2)
        return 0;
    for (uint64 d = 2; d < n; d++)
        if (n % d == 0)
            return 0;
    return 1;
}

/* the n-th prime, counting from nth_prime(0) == 2 */
static uint64 nth_prime(uint64 n)
{
    uint64 candidate = n ? nth_prime(n - 1) + 1 : 2;
    while (!is_prime(candidate))
        candidate++;
    return candidate;
}

/* SHA-512 round constant: fractional part of the cube root of a prime */
static uint64 round_constant(uint64 round)
{
    return cube_root_fraction(nth_prime(round));
}

/* one SHA-512 round; the message schedule is kept as a rolling 16-word window */
static void sha512_round(uint64 *state, uint64 round, uint64 *schedule)
{
    uint64 t1 = state[7] + big_sigma1(state[4]) + choose(state[4], state[5], state[6])
              + round_constant(round) + schedule[round % 16];
    uint64 t2 = big_sigma0(state[0]) + majority(state[0], state[1], state[2]);

    for (int i = 7; i > 0; i--)
        state[i] = state[i - 1];
    state[4] += t1;
    state[0] = t1 + t2;

    /* after every 16 rounds compute the next 16 schedule words in place */
    if (round % 16 == 15)
        for (int i = 0; i < 16; i++)
            schedule[i] += schedule[(i + 9) % 16] + small_sigma0(schedule[(i + 1) % 16])
                         + small_sigma1(schedule[(i + 14) % 16]);
}

static void sha512_compress(uint64 *state, uint64 *saved, uint64 *schedule)
{
    for (int i = 0; i < 8; i++)
        saved[i] = state[i];
    for (uint64 round = 0; round < 80; round++)
        sha512_round(state, round, schedule);
    for (int i = 0; i < 8; i++)
        state[i] += saved[i];
}

int main()
{
    uint64 state[8], saved[8], schedule[16];

    /* resume from the precomputed state after the first 1280 bytes */
    for (int i = 0; i < 8; i++)
        state[i] = decode_base31(i);

    /* block 11: the string contents plus the hidden "\";\n" */
    for (int i = 0; i < 15; i++)
        schedule[i] = read_word(i);
    schedule[15] = decode_base31(8);
    sha512_compress(state, saved, schedule);

    /* block 12: padding bit and total message length */
    schedule[0] = 1ull << 63;
    for (int i = 1; i < 15; i++)
        schedule[i] = 0;
    schedule[15] = TOTAL_LENGTH_BITS;
    sha512_compress(state, saved, schedule);

    for (int i = 0; i < 8; i++)
        printf("%016llx", state[i]);
    printf("\n");
    return 0;
}
