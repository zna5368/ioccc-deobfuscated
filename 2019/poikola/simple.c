#include    <stdio.h>
#include   <string.h>
#include    <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

/*
 * Usage: ./prog <bits> <file>
 *
 * Depending on the day this program was compiled (taken from __DATE__) it
 * prints one of three things:
 *
 *   mode 0: the Fibonacci numbers below 2^64 (in hexadecimal)
 *   mode 1: the SHA-3-<bits> digest of <file>
 *   mode 2: the odd primes up to the size of <file> in bytes
 *
 * No input is validated.
 */

#define MAX_FILE_SIZE 1073741824ul	/* 1 GiB */

typedef unsigned long long u64;

enum { MODE_FIBONACCI, MODE_SHA3, MODE_PRIMES };

/* bit set of prime candidates, one bit per number up to MAX_FILE_SIZE */
static unsigned char sieve[MAX_FILE_SIZE / 8 + 1];

#define SIEVE_SET(i)	(sieve[(i) >> 3] |=  (1u << ((i) & 7)))
#define SIEVE_CLEAR(i)	(sieve[(i) >> 3] &= ~(1u << ((i) & 7)))
#define SIEVE_TEST(i)	(sieve[(i) >> 3] &   (1u << ((i) & 7)))

/* Keccak-f[1600] round constants */
static const u64 round_constants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

/* rotation offsets of the rho step, in the order lanes are visited by pi */
static const int rotations[24] = {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
};

/* lane visiting order of the pi step */
static const int pi_lanes[24] = {
    10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
    15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
};

static u64
rotate_left(u64 value, int count)
{
    return value << count | value >> (64 - count);
}

/* the Keccak-f[1600] permutation over the 5x5 state of 64-bit lanes */
static void
keccak_f(u64 state[25])
{
    u64 column[5], t, saved;
    int round, i, x, y;

    for (round = 0; round < 24; round++) {
	/* theta */
	for (x = 0; x < 5; x++)
	    column[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^
			state[x + 15] ^ state[x + 20];
	for (x = 0; x < 5; x++) {
	    t = column[(x + 4) % 5] ^ rotate_left(column[(x + 1) % 5], 1);
	    for (y = 0; y < 25; y += 5)
		state[x + y] ^= t;
	}

	/* rho and pi */
	t = state[1];
	for (i = 0; i < 24; i++) {
	    saved = state[pi_lanes[i]];
	    state[pi_lanes[i]] = rotate_left(t, rotations[i]);
	    t = saved;
	}

	/* chi */
	for (y = 0; y < 25; y += 5) {
	    for (x = 0; x < 5; x++)
		column[x] = state[x + y];
	    for (x = 0; x < 5; x++)
		state[x + y] ^= ~column[(x + 1) % 5] & column[(x + 2) % 5];
	}

	/* iota */
	state[0] ^= round_constants[round];
    }
}

#define NEXT_BYTE	(u64)data[++index] << 8 * index

/* print the SHA-3 digest of data as lowercase hexadecimal */
static void
print_sha3(const unsigned char *data, u64 size, u64 digest_bytes)
{
    u64 state[25];
    u64 rate = 25 - digest_bytes / 4;	/* lanes absorbed per block */
    u64 lane = 0, word, words = size / 8, tail = size % 8, i, index;
    int byte;

    memset(state, 0, sizeof state);

    /* absorb whole 64-bit little endian words */
    for (i = 0; i < words; i++, data += 8) {
	/*
	 * Intended as a little endian load of 8 bytes, but index is modified
	 * and read without a sequence point (undefined behaviour), exactly as
	 * in the original.  Clang computes the correct word; GCC (at least
	 * version 14) instead ORs the 8 bytes together and shifts the result
	 * by 56, which produces a different (non-standard) digest.
	 */
	index = -1;
	word = NEXT_BYTE | NEXT_BYTE | NEXT_BYTE | NEXT_BYTE |
	       NEXT_BYTE | NEXT_BYTE | NEXT_BYTE | NEXT_BYTE;
	state[lane] ^= word;
	if (++lane == rate) {
	    keccak_f(state);
	    lane = 0;
	}
    }

    /* the remaining bytes followed by SHA-3 padding */
    word = 0;
    for (byte = 0; byte < (int)tail; byte++)
	word |= (u64)data[byte] << 8 * byte;
    state[lane] ^= word ^ (u64)0x06 << 8 * byte;
    state[rate - 1] ^= 0x8000000000000000ULL;
    keccak_f(state);

    /* squeeze */
    for (i = 0; i < digest_bytes; i++)
	printf("%02x", (unsigned)(state[i / 8] >> 8 * (i % 8) & 0xff));
}

/* print every prime from 3 up to limit (2 is deliberately left out) */
static void
print_primes(u64 limit)
{
    u64 candidate, prime, multiple;

    for (candidate = 3; candidate <= limit; candidate++)
	SIEVE_SET(candidate);

    for (prime = 2; prime <= limit / 2; ) {
	for (multiple = 2 * prime; multiple <= limit; multiple += prime)
	    SIEVE_CLEAR(multiple);
	do
	    prime++;
	while (!SIEVE_TEST(prime));
    }

    for (candidate = 2; candidate <= limit; candidate++)
	if (SIEVE_TEST(candidate))
	    printf("%llu ", candidate);
}

/* print the Fibonacci numbers F(1) .. F(93), all of which fit in 64 bits */
static void
print_fibonacci(void)
{
    u64 current = 1, next = 1, sum;
    int i;

    for (i = 1; i <= 93; i++) {
	printf("%llx ", current);
	sum = current + next;
	current = next;
	next = sum;
    }
}

/* choose a mode from the compile date: it changes every day */
static int
compile_day_mode(void)
{
    const char *date = __DATE__;	/* "Mmm dd yyyy" */
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    u64 year, month, day, sum;

    year = (date[7] - '0') * 1000 + (date[8] - '0') * 100 +
	   (date[9] - '0') * 10 + (date[10] - '0');
    for (month = 1; month < 12 && strncmp(months + 3 * (month - 1), date, 3); month++)
	;
    day = (date[4] == ' ' ? 0 : (date[4] - '0') * 10) + (date[5] - '0');

    /* Mike Keith's day of week formula (offset by one day), modulo 3 */
    if (month > 2) {
	sum = 23 * month / 9 + year - 2;
    } else {
	sum = 23 * month / 9 + year;
	year--;
    }
    sum += day + 3 + year / 4 + year / 400 - year / 100;

    return sum % 3;
}

int
main(int argc, char *argv[])
{
    u64 bits = 0, size;
    const char *digits = argv[1];
    struct stat file_info;
    const unsigned char *data;
    int fd;

    (void)argc;

    for (; *digits; digits++)
	bits = bits * 10 + *digits - '0';

    fd = open(argv[2], O_RDONLY);
    fstat(fd, &file_info);
    size = file_info.st_size;
    data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

    switch (compile_day_mode()) {
    case MODE_FIBONACCI:
	print_fibonacci();
	break;
    case MODE_SHA3:
	print_sha3(data, size, bits / 8);
	break;
    case MODE_PRIMES:
	print_primes(size);
	break;
    }

    return puts("");
}
