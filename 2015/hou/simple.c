#include <stdio.h>
#include <math.h>

/*
 * Prints the MD5 digest (RFC 1321) of standard input.
 *
 * The whole input is loaded and padded first, and only then hashed, so the
 * program has a fixed input limit. Bytes are placed in words little-endian
 * regardless of the machine. The original only prints a correct digest on
 * little-endian machines; elsewhere it adds " (BE)" after a wrong one.
 */

typedef unsigned int word; /* must be exactly 32 bits */

#define BLOCK_WORDS 16
#define BLOCK_BYTES 64
#define STEPS_PER_BLOCK 64
#define STEPS_PER_ROUND 16
#define LENGTH_FIELD_OFFSET 56 /* where the bit count goes in the last block */
#define LENGTH_FIELD_BYTES 8

#define MAX_MESSAGE_BYTES 399927 /* padding included */
#define MESSAGE_WORDS ((MAX_MESSAGE_BYTES + 3) / 4)

#define TWO_POW_32 4294967296.0
#define TWO_POW_52 4503599627370496.0
#define TWO_POW_53 9007199254740992.0

static word message[MESSAGE_WORDS];
static int message_bytes = 0;

/* How far each step rotates, by round and by step number modulo 4. */
static const int rotations[4][4] = {
    { 7, 12, 17, 22 },
    { 5,  9, 14, 20 },
    { 4, 11, 16, 23 },
    { 6, 10, 15, 21 }
};

/* Adds one byte to the message. Returns 0 if there is no room for it. */
static int append_byte(int byte)
{
    if (message_bytes >= MAX_MESSAGE_BYTES)
        return 0;
    message[message_bytes / 4] |= (word)byte << (message_bytes % 4 * 8);
    message_bytes++;
    return 1;
}

/*
 * Reads standard input and pads it: a 0x80 byte, zeros up to the length field,
 * then the input size in bits. Returns 0 if the input is too large.
 */
static int read_message(void)
{
    int input_byte;
    word input_bits;

    while ((input_byte = getchar()) != EOF) {
        if (!append_byte(input_byte))
            return 0;
    }
    input_bits = (word)message_bytes * 8;

    if (!append_byte(0x80))
        return 0;
    while (message_bytes % BLOCK_BYTES != LENGTH_FIELD_OFFSET) {
        if (!append_byte(0))
            return 0;
    }

    /* Inputs are too small for the upper half of the length to be non-zero. */
    message[message_bytes / 4] = input_bits;
    message_bytes += LENGTH_FIELD_BYTES;
    return 1;
}

/*
 * Checks that doubles are rounded once, to 53 bits. x87 floating point rounds
 * twice, which turns this sum into 0 rather than 1. The original does all of
 * its arithmetic through this kind of rounding, so it gives an error there.
 */
static int has_iso_rounding(void)
{
    volatile double sum = 0.5 + 1 / TWO_POW_53;

    sum += TWO_POW_52;
    sum -= TWO_POW_52;
    return sum == 1;
}

static word rotate_left(word value, int count)
{
    return value << count | value >> (32 - count);
}

/* The constant for each step is the fraction of |sin(step + 1)| in 32 bits. */
static word sine_constant(int step)
{
    return (word)(fabs(sin(step + 1.0)) * TWO_POW_32);
}

static void process_block(word state[4], const word block[BLOCK_WORDS])
{
    word a = state[0];
    word b = state[1];
    word c = state[2];
    word d = state[3];
    int step;

    for (step = 0; step < STEPS_PER_BLOCK; step++) {
        int round = step / STEPS_PER_ROUND;
        int block_index = 0;
        word mixed = 0;
        word sum;
        word rotated;

        switch (round) {
        case 0:
            mixed = (b & c) | (~b & d);
            block_index = step;
            break;
        case 1:
            mixed = (d & b) | (~d & c);
            block_index = 5 * step + 1;
            break;
        case 2:
            mixed = b ^ c ^ d;
            block_index = 3 * step + 5;
            break;
        case 3:
            mixed = c ^ (b | ~d);
            block_index = 7 * step;
            break;
        }

        sum = a + mixed + sine_constant(step) + block[block_index % BLOCK_WORDS];
        rotated = rotate_left(sum, rotations[round][step % 4]);

        a = d;
        d = c;
        c = b;
        b += rotated;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

/* The digest is the four state words with each word's lowest byte first. */
static void print_digest(const word state[4])
{
    int word_index;
    int byte_index;

    for (word_index = 0; word_index < 4; word_index++) {
        for (byte_index = 0; byte_index < 4; byte_index++)
            printf("%02x", state[word_index] >> (byte_index * 8) & 0xff);
    }
    printf("\n");
}

int main()
{
    word state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
    int offset;

    if (!read_message()) {
        printf("out of memory\n");
        return 0;
    }
    if (!has_iso_rounding()) {
        printf("non-ISO rounding");
        return 0;
    }

    for (offset = 0; offset < message_bytes; offset += BLOCK_BYTES)
        process_block(state, message + offset / 4);

    print_digest(state);
    return 0;
}
