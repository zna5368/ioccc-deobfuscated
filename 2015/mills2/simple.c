/*
 * zcat: print a file made by compress(1), using no dictionary table.
 *
 * A compressed file is a sequence of variable-width LZW codes.  Each position
 * in that sequence is called a "slot" here.  Slots are numbered from
 * FIRST_SLOT, and that numbering restarts after every clear code.
 *
 * The encoder creates dictionary entry N out of two neighbouring slots:
 *
 *     entry N = (string of slot N - 1) + (first character of slot N)
 *
 * so the code stream already is the dictionary.  To expand an entry, the
 * decoder seeks back to those two slots and recurses, instead of keeping a
 * table in memory.
 */

enum {
    READ_ONLY = 0,          /* O_RDONLY for open() */
    HEADER_BYTES = 3,       /* two magic bytes and one flags byte */
    LITERAL_CODES = 256,    /* codes below this are plain characters */
    CLEAR_CODE = 256,       /* code that resets the dictionary */
    FIRST_SLOT = 256,       /* number of the first slot after a reset */
    MIN_CODE_BITS = 9,
    MAX_CODE_BITS = 16,
    CODES_PER_GROUP = 8,    /* compress(1) writes codes in groups of eight */
    BITS_PER_BYTE = 8,
    WINDOW_BYTES = 3,       /* any code fits in 3 bytes, wherever it starts */
    MIN_WINDOW_BYTES = 2    /* fewer bytes than this means end of stream */
};

int next_slot = FIRST_SLOT;         /* slot the main loop will print next */
int first_char_only;                /* nonzero: print only a first character */
int segment_start = HEADER_BYTES;   /* file offset of the first slot */
int input_file;

/* Width in bits of the code stored in a slot. */
int code_width(int slot)
{
    int width = MIN_CODE_BITS;

    while (width < MAX_CODE_BITS && slot >> width != 0)
        width++;
    return width;
}

/* Distance in bits from the start of the segment to a slot. */
int bit_offset(int slot, int width)
{
    int offset = 0;
    int narrower;

    /* There are 2^(w-1) slots that have a width of w bits. */
    for (narrower = MIN_CODE_BITS; narrower < width; narrower++)
        offset += narrower << (narrower - 1);

    /* The slots of this width start at number 2^(width-1). */
    return offset + width * (slot - (1 << (width - 1)));
}

/* Extract a code from the file, exiting when the stream has run out. */
int read_code(int offset, int width, int slot)
{
    /*
     * pread() replaces only the low bytes of this little-endian int.  If the
     * read comes up short, the remaining bytes keep the slot number's bytes.
     */
    int window = slot;

    if (pread(input_file, &window, WINDOW_BYTES,
              segment_start + offset / BITS_PER_BYTE) < MIN_WINDOW_BYTES)
        exit(0);

    window >>= offset % BITS_PER_BYTE;
    return window % (1 << width);
}

/* Print the string that is stored in a slot. */
void print_slot(int slot)
{
    int width = code_width(slot);
    int offset = bit_offset(slot, width);
    int code = read_code(offset, width, slot);

    if (code < LITERAL_CODES) {
        putchar(code);
    } else if (code == CLEAR_CODE) {
        /*
         * The encoder flushes the rest of the group of eight codes after a
         * clear code, so skip those too and start a new segment there.
         */
        int padding_slots = -next_slot & (CODES_PER_GROUP - 1);

        segment_start += (padding_slots * width + offset + width)
                         / BITS_PER_BYTE;
        next_slot = FIRST_SLOT;
    } else {
        /* Dictionary entry: first the string of the previous slot ... */
        print_slot(code - 1);

        /*
         * ... then the first character of this entry's own slot.  Following
         * only the prefix chain down to a literal yields that character.
         */
        if (!first_char_only) {
            first_char_only = 1;
            print_slot(code);
            first_char_only = 0;
        }
    }
}

int main(int argc, char **argv)
{
    input_file = open(argv[1], READ_ONLY);

    /* read_code() ends the program at the end of the stream. */
    for (;;)
        print_slot(next_slot++);
}
