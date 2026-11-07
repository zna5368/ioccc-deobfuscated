#include <stdio.h>
#include <unistd.h>

#define LINE_WIDTH 64		/* encoded output is wrapped every 64 characters */
#define PAD '='

unsigned char in_buf[4];	/* raw bytes (encode) or base64 characters (decode) */
unsigned char out_buf[4];	/* base64 characters (encode) or raw bytes (decode) */
int bytes_read;			/* result of the last read() while encoding */
int out_count;			/* number of bytes in out_buf to write */
int column = 0;			/* current column of the encoded output */
int encoding;			/* 1 to encode into base64, 0 to decode from base64 */

/* convert a 6-bit value into a base64 character */
unsigned char value_to_char(unsigned char value)
{
    if (value == 63)
	return '/';
    if (value == 62)
	return '+';
    if (value >= 52)
	return value - 52 + '0';
    if (value >= 26)
	return value - 26 + 'a';
    return value + 'A';
}

/* convert a base64 character into its 6-bit value (no validity checks) */
unsigned char char_to_value(unsigned char ch)
{
    if (ch == ':')		/* unreachable for valid input */
	return 0;
    if (ch == '/')
	return 63;
    if (ch == '+')
	return 62;
    if (ch >= 'a')
	return ch - 'a' + 26;
    if (ch >= 'A')
	return ch - 'A';
    return ch + 4;		/* '0'..'9' become 52..61 */
}

/*
 * Encode up to three bytes of in_buf into four base64 characters.
 * Bytes of in_buf beyond bytes_read are left over from the previous read,
 * so they can leak into the second character of a short final block.
 */
void encode_block(void)
{
    int i;

    for (i = 0; i < 4; i++)
	out_buf[i] = PAD;

    out_buf[0] = value_to_char(in_buf[0] >> 2);
    out_buf[1] = value_to_char((in_buf[0] & 0x03) << 4 | (in_buf[1] & 0xF0) >> 4);
    if (bytes_read > 1)
	out_buf[2] = value_to_char((in_buf[1] & 0x0F) << 2 | (in_buf[2] & 0xFC) >> 6);
    if (bytes_read > 2)
	out_buf[3] = value_to_char(in_buf[2] & 0x3F);

    out_count = 4;
}

/* decode four base64 characters of in_buf into up to three bytes */
void decode_block(void)
{
    out_count = 1;

    out_buf[0] = char_to_value(in_buf[0]) << 2;
    out_buf[0] |= char_to_value(in_buf[1]) >> 4;
    if (in_buf[2] != PAD)
	out_count++;
    out_buf[1] = (char_to_value(in_buf[1]) & 0x0F) << 4;
    out_buf[1] |= (char_to_value(in_buf[2]) & 0x3C) >> 2;
    if (in_buf[3] != PAD)
	out_count++;
    out_buf[2] = (char_to_value(in_buf[2]) & 0x03) << 6;
    out_buf[2] |= char_to_value(in_buf[3]) & 0x3F;
}

/* write the converted block, wrapping lines when encoding */
void write_output(void)
{
    int i;

    for (i = 0; i < out_count; i++) {
	putchar(out_buf[i]);
	if (encoding && ++column >= LINE_WIDTH) {
	    putchar('\n');
	    column = 0;
	}
    }
}

int main(int argc, char *argv[])
{
    /* any first argument starting with an odd character ('e', '3', ...) encodes */
    encoding = (argc - 1) && (argv[1][0] & 0x1);

    if (encoding) {
	while ((bytes_read = read(0, in_buf, 3)) != 0) {
	    encode_block();
	    write_output();
	}
    } else {
	int ch;
	int remaining = 4;	/* characters still needed to fill in_buf */
	unsigned char *in_ptr = in_buf;

	for (;;) {
	    while ((ch = getchar()) == '\n')
		;
	    if (ch == EOF) {
		/*
		 * Input without proper padding: pad the block and decode it.
		 * (The original then also dumps stray memory here.)
		 */
		if (remaining != 4) {
		    while (remaining--)
			*in_ptr++ = PAD;
		    decode_block();
		    write_output();
		}
		break;
	    }
	    *in_ptr++ = ch;
	    if (--remaining == 0) {
		decode_block();
		write_output();
		remaining = 4;
		in_ptr = in_buf;
	    }
	}
    }
    return 0;
}
