/*
 * LZW compressor that writes the UNIX compress (.Z) format with a fixed
 * code size of 9 bits.  Codes 0..255 are single bytes, code 256 is the
 * (unused) CLEAR code and codes 257..511 are learned strings.
 */

#define HEADER      0x899D1F /* 0x1F 0x9D magic, then 0x89 = block mode | 9 bits */
#define CODE_BITS   9
#define FIRST_CODE  256      /* dictionary entry n gets code FIRST_CODE + n */
#define MAX_ENTRIES 256      /* entries 1..255 give codes 257..511 */
#define FINISHED    17       /* bit count that means "flush and stop" */
#define END_OF_FILE (-1)     /* what getchar() returns at the end */

/*
 * dictionary[(byte << 9) | prefix_code] holds the entry number of the
 * string "prefix + byte", or 0 when that string is not known yet.
 */
int dictionary[1 << 17];
int next_entry;

/* write the lowest whole bytes of the bit buffer to stdout */
unsigned int flush_bytes(unsigned int buffer, int nbytes)
{
	unsigned char byte;

	while (nbytes-- > 0) {
		byte = buffer & 255;
		write(1, &byte, 1);
		buffer >>= 8;
	}
	return buffer;
}

/*
 * The bit buffer is laid out from the lowest bit up as:
 *
 *   [ nbits finished output bits ][ 9-bit current prefix code ][ next byte ]
 *
 * nbits starts as argc.  With exactly 14 arguments (argc == 15) the first
 * 8 bits of HEADER are written right away, and the remaining 16 header
 * bits act as 7 pending bits plus a fake first "prefix code".  That fake
 * code is emitted (completing the header) when the first byte is read
 * and uses up dictionary entry 0, which is why 0 can mean "not found".
 */
int main(int argc)
{
	unsigned int buffer = HEADER;
	int nbits = argc;
	int key, c;

	for (;;) {
		buffer = flush_bytes(buffer, nbits / 8);
		if (nbits == FINISHED)
			break;
		nbits %= 8;

		c = getchar();
		if (c == END_OF_FILE) {
			/* emit the last prefix code, pad with 1 bits, then stop */
			buffer ^= ~0U << (nbits + CODE_BITS);
			nbits = FINISHED;
			continue;
		}

		/* place the byte just above the current prefix code (those
		   bits are zero after a nicely asked start, so ^ acts like |) */
		buffer ^= (unsigned int)c << (nbits + CODE_BITS);
		key = buffer >> nbits;	/* (c << 9) | prefix */

		if (dictionary[key]) {
			/* "prefix + c" is known: its code becomes the new prefix */
			buffer &= (1U << nbits) - 1;
			buffer |= (unsigned int)(FIRST_CODE + dictionary[key]) << nbits;
		} else {
			/* unknown: learn it, then output the prefix code and
			   let c become the new prefix */
			if (next_entry < MAX_ENTRIES)
				dictionary[key] = next_entry++;
			nbits += CODE_BITS;
		}
	}
	return 0;
}
