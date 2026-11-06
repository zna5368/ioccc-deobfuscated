/*
 * Print the low 8 bits of a number as a string of binary digits, without
 * any loops or branches.
 *
 * The eight output characters are built inside a single 64-bit integer (one
 * character per byte) and then printed by treating that integer's memory as
 * a char array.  Which byte is printed first depends on the machine's byte
 * order, so there are two sets of constants:
 *
 *   default            - for little-endian machines (prog / prog_le)
 *   -DBIG_ENDIAN_BUILD - for big-endian machines    (prog_be)
 *
 * The constants are only correct when exactly one argument is given
 * (argc == 2), because argc is used as a divisor below.  Other argument
 * counts produce the "strange output" mentioned in the README.
 */

/* "11111111" in ASCII: the byte '1' (0x31) repeated eight times */
#define ASCII_ONES 0x3131313131313131LL

#ifndef BIG_ENDIAN_BUILD
/* (MAGIC / 2) ^ ASCII_ONES == 0x0100804020100804, the bit-spreading multiplier */
#define MAGIC          0x606362e22242726aLL
#define VALUE_SHIFT    0	/* bits 6..0 are spread by the multiply */
#define TOP_BIT_SHIFT  7	/* bit 7 goes straight into the first byte */
#else
/* (MAGIC / 2) ^ ASCII_ONES == 0x0004081020408102, the bit-spreading multiplier */
#define MAGIC          0x626a724222e36066LL
#define VALUE_SHIFT    1	/* bits 7..1 are spread by the multiply */
#define TOP_BIT_SHIFT  0	/* bit 0 goes straight into the last byte */
#endif

int main(int argc, char **argv)
{
    long long magic = MAGIC;
    long long ones = ASCII_ONES;

    /*
     * With argc == 2: 0x3131... / 2 = 0x1898..., & 0x3131... = 0x1010...,
     * >> 4 = 0x0101010101010101, i.e. the lowest bit of every byte.
     */
    long long low_bit_mask = ((ones / argc) & ones) >> 4;

    long long value = atoi(*++argv);

    /*
     * Multiplying by a constant with one bit set per byte adds together
     * shifted copies of the value, moving each input bit into the lowest
     * bit of its own byte.  The bit not reachable by the multiply is
     * OR'ed in with a plain shift.
     */
    long long multiplier = ones ^ (magic / argc);
    long long spread_bits = (multiplier * (value >> VALUE_SHIFT)) | (value >> TOP_BIT_SHIFT);

    /* "00000000" in ASCII (0x30 per byte) when argc == 2 */
    long long ascii_zeros = ones ^ low_bit_mask;

    /* each byte becomes '0' or '1' */
    long long digits = (spread_bits & low_bit_mask) | ascii_zeros;

    printf("%.8s\n", (char *)&digits);
}
