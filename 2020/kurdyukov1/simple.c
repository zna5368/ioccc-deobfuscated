#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

/*
 * MD5 of standard input, printed as 32 lowercase hex digits.
 *
 * The 64 MD5 constants are floor(|sin(i + 1)| * 2^32).  Instead of using a
 * table or floating point, a point (sine, cosine) is rotated by 1 radian per
 * step using 64-bit fixed point integer arithmetic.
 */

#define BLOCK_BYTES 64

/* cos(1) * 2^41 and sin(1) * 2^41 split into a high part (scaled by 2^20)
 * and a low part (scaled by 2^41) so the products fit in 64 bits */
#define COS_HI 566548
#define COS_LO 64336
#define SIN_HI 882346
#define SIN_LO 585873

static uint32_t state[4];
static uint32_t block[16];
static int64_t bit_length = 0;
static int last_char = 0;

/* per-round shift amounts */
static const int shifts[4][4] = {
    { 7, 12, 17, 22 },
    { 5,  9, 14, 20 },
    { 4, 11, 16, 23 },
    { 6, 10, 15, 21 },
};

static uint32_t rotate_left(uint32_t value, int count)
{
    return value << count | value >> (32 - count);
}

/* next byte of the padded message: data, then 0x80, then zeros */
static uint32_t next_byte(void)
{
    if (last_char < 0)
        return 0;
    last_char = getchar();
    if (last_char >= 0) {
        bit_length += 8;
        return last_char;
    }
    return 0x80;
}

/* Fill the 64 byte block.  Returns 0 if this is the last block (it holds
 * the message length), or 1 if more blocks follow. */
static int read_block(void)
{
    int byte;
    uint32_t word = 0;

    for (byte = 0; byte < BLOCK_BYTES; byte++) {
        if (byte == 56 && last_char < 0) {
            /* end of input reached: append the length in bits */
            block[14] = (uint32_t)bit_length;
            block[15] = (uint32_t)(bit_length >> 32);
            return 0;
        }
        /* little endian: each new byte enters at the top of the word */
        word = next_byte() << 24 | word >> 8;
        block[byte / 4] = word;
    }
    return 1;
}

static void compress_block(void)
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    int64_t sine = 0, cosine = -((int64_t)1 << 40); /* point (0, -2^40) */
    int64_t new_sine;
    uint32_t i, round, mix, message_index, constant, sum;

    for (i = 0; i < 64; i++) {
        round = i / 16;

        /* rotate the point by 1 radian: sine becomes -2^40 * sin(i + 1) */
        new_sine = ((sine * COS_LO + cosine * SIN_LO) >> 21)
                 + sine * COS_HI + cosine * SIN_HI;
        cosine = (((cosine * COS_LO - sine * SIN_LO) >> 21)
                 + cosine * COS_HI - sine * SIN_HI) >> 20;
        sine = new_sine >> 20;
        /* |sine| scaled down to 2^32 (one's complement when negative) */
        constant = (uint32_t)(sine >> 40 ^ sine >> 8);

        switch (round) {
        case 0:
            mix = (b & c) | (~b & d);
            message_index = i;
            break;
        case 1:
            mix = (b & d) | (c & ~d);
            message_index = 5 * i + 1;
            break;
        case 2:
            mix = b ^ c ^ d;
            message_index = 3 * i + 5;
            break;
        default:
            mix = c ^ (b | ~d);
            message_index = 7 * i;
            break;
        }

        sum = a + mix + block[message_index % 16] + constant;
        a = d;
        d = c;
        c = b;
        b += rotate_left(sum, shifts[round][i % 4]);
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

int main()
{
    int more = 1;
    int digit, nibble;

#ifdef _WIN32
    setmode(fileno(stdin), O_BINARY);
#endif

    state[0] = 0x67452301;
    state[1] = 0xefcdab89;
    state[2] = 0x98badcfe;
    state[3] = 0x10325476;

    while (more) {
        more = read_block();
        compress_block();
    }

    /* print the digest bytes in little endian order, high nibble first */
    for (digit = 0; digit < 32; digit++) {
        nibble = state[digit / 8] >> (digit % 8 * 4 ^ 4) & 15;
        putchar(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
    }
    putchar('\n');
    return 0;
}
