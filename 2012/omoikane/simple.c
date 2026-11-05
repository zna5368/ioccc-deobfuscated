#include<stdio.h>

/*
 * The generated program is a polyglot: bash prints its first half (the key),
 * perl prints its second half (the data), and a C compiler sees both.
 *
 * Its data string starts with a hint, encrypted with an all-zero key, so
 * the data half alone still prints the hint when compiled without the key.
 * The real key's first word is HINT_LENGTH, which skips past the hint.
 */

#define KEY_WORDS 12          /* words of key printed in the output */
#define KEY_WORDS_PER_LINE 6
#define STATE_WORDS 256       /* size of the ISAAC state */
#define WARM_UP_ROUNDS (1 << 24)

#define LINE_WIDTH 79         /* longest line of generated code */
#define STRING_LINE_END (LINE_WIDTH - 1)  /* column of a string's closing " */
#define LAST_ESCAPE_COLUMN 76
#define PADDED_LINE_START 6   /* leaves DECODER_MAIN exactly LINE_WIDTH long */

#define BLOCK_BYTES 4         /* 4 input bytes become 5 base 85 digits */
#define BLOCK_DIGITS 5
#define BASE 85

#define HINT_LENGTH 86

/* Printed by bash, and hidden from C by #ifdef and from perl by q{} quotes. */
static const char POLYGLOT_HEADER[] =
    "#ifdef/*\n"
    "'true' or q{\n"
    "exec head -8 $0\n"
    "};for(open$O,$0;<$O>;print if$f){$f|=/^$/;}q{*/q";

/* Ends the key half. The C program includes itself to fill in the key array. */
static const char KEY_TRAILER[] =
    "#endif/*}||1;\n"
    "__DATA__ */\n"
    "\n"
    "#ifndef q\n"
    "#define q\n"
    "#include<stdio.h>\n"
    "typedef unsigned int _;_ K[]={\n"
    "#include __FILE__\n"
    "#undef q";

/* Start of the generated decoder: a compact copy of the random generator
 * below, then the opening of the string that holds the encoded data. */
static const char DECODER_HEAD[] =
    "0},L,O,l[256],I[256],n,y,a,r,u,k,o;"
    "_*x(){"
        "if(!L--){"
            "y+=++a;"
            "for(o=0;o<256;y=l[o++]=I[255&(k>>10)]+u){"
                "n^=(o&1)?n>>((o&2)?16:6):n<<((o&2)?2:13);"
                "u=I[o];"
                "k=I[o]=I[255&(u>>2)]+(n+=I[(o+128)&255])+y;"
            "}"
            "L=255;"
        "}"
        "return&l[L];"
    "}"
    "_*X(){"
        "for(O=0;256>O;I[O++]=0);"
        "for(O=0;sizeof(K)/sizeof(_)>O;O++)I[O&255]^=K[O];"
        "for(n=y=a=L=O=0;O<1<<24;++O)x();"
        "r=O=0x0;"
        "return&O;"
    "}"
    "char*S,s[]=\"";

/* End of the generated decoder: closes the data string and starts main. */
static const char DECODER_MAIN[] =
    "\";int main(){"
        "X();"
        "for(S=s+*K;*S>37;){"
            "for(o=0;o<5;o++)r=r*85+(83+*S++)%89;"
            "r";

static const char DECODER_TAIL[] =
            "^=*x();"
            "for(o=0;o<4;o++){s[O++]=r&255;r>>=8;}"
        "}"
        "return!fwrite(s,O-*S%5,1,stdout);"
    "}\n"
    "#endif";

/* Last character of the data, indexed by the number of bytes in the final
 * block. The decoder uses (marker % 5) to drop the bytes that were not real. */
static const char END_MARKERS[] = "#!%$";

/*
 * Holds the text of the next piece of the data string. It starts as the hint
 * and is reused for each encoded block. A final short block keeps old bytes in
 * its unread positions; the decoder drops them again.
 *
 * Decodes to: "The chaos that always crawls up to you with a smile, Nyarlathotep\n"
 */
static char block[] =
    "KfWWw|Z};fRo?JtJaV<x4@*?R?&JV1.s{Fyj2_;khB1xQ5oxm~mS@B|(pa>oRURonB}h@o?)d.X)NSTIUCz7@%";

/* ISAAC random number generator */
static unsigned int key[STATE_WORDS];
static unsigned int state[STATE_WORDS];
static unsigned int results[STATE_WORDS];
static unsigned int results_left;
static unsigned int accumulator;
static unsigned int last_result;
static unsigned int counter;

/* Output line being built */
static char line[256];
static char *line_end = line;

/* Generate the next batch of 256 results. */
static void refill_results(void)
{
    unsigned int i;
    unsigned int old_word;
    unsigned int new_word;

    counter++;
    last_result += counter;
    for (i = 0; i < STATE_WORDS; i++) {
        switch (i % 4) {
        case 0:
            accumulator ^= accumulator << 13;
            break;
        case 1:
            accumulator ^= accumulator >> 6;
            break;
        case 2:
            accumulator ^= accumulator << 2;
            break;
        case 3:
            accumulator ^= accumulator >> 16;
            break;
        }
        accumulator += state[(i + 128) % STATE_WORDS];

        old_word = state[i];
        new_word = state[(old_word >> 2) % STATE_WORDS] + accumulator + last_result;
        state[i] = new_word;

        last_result = state[(new_word >> 10) % STATE_WORDS] + old_word;
        results[i] = last_result;
    }
    results_left = STATE_WORDS;
}

static unsigned int next_random(void)
{
    if (results_left == 0) {
        refill_results();
    }
    results_left--;
    return results[results_left];
}

/* Restart the generator from the key, then discard its early output. */
static void seed_generator(void)
{
    unsigned int i;

    for (i = 0; i < STATE_WORDS; i++) {
        state[i] = key[i];
    }
    accumulator = 0;
    last_result = 0;
    counter = 0;
    results_left = 0;
    for (i = 0; i < WARM_UP_ROUNDS; i++) {
        next_random();
    }
}

/* Fill the key from a file, then use that to pick the key that gets printed. */
static void choose_key(const char *seed_path)
{
    FILE *seed_file = fopen(seed_path, "rb");
    unsigned int i;

    if (seed_file) {
        fread(key, 256, 4, seed_file);
        fclose(seed_file);
    }
    seed_generator();

    key[0] = HINT_LENGTH;
    for (i = 1; i < KEY_WORDS; i++) {
        key[i] = next_random();
    }
    for (i = KEY_WORDS; i < STATE_WORDS; i++) {
        key[i] = 0;
    }
    seed_generator();
}

static void print_key(void)
{
    int i;

    for (i = 0; i < KEY_WORDS; i++) {
        printf("%uU,", key[i]);
        if ((i + 1) % KEY_WORDS_PER_LINE == 0) {
            putchar('\n');
        }
    }
}

static void flush_line(void)
{
    *line_end = '\0';
    puts(line);
    line_end = line;
}

/* Add a character, escaping a second '?' so that "??" is never a trigraph. */
static void append_char(char character)
{
    int after_question_mark = line_end > line && line_end[-1] == '?';

    if (character == '?' && after_question_mark
            && line_end <= line + LAST_ESCAPE_COLUMN) {
        *line_end++ = '\\';
    }
    *line_end++ = character;
}

static void append_text(const char *text)
{
    for (; *text; text++) {
        append_char(*text);
    }
}

/* Add code, wrapping it wherever the line becomes full. */
static void append_wrapped_code(const char *code)
{
    for (; *code; code++) {
        append_char(*code);
        if (line_end - line >= LINE_WIDTH) {
            flush_line();
        }
    }
}

static void open_string_line(void)
{
    if (line_end == line) {
        *line_end++ = '"';
    }
}

static void close_full_string_line(void)
{
    if (line_end - line >= STRING_LINE_END) {
        *line_end++ = '"';
        flush_line();
    }
}

/* Add text to the data string, which is split into one string per line. */
static void append_string_text(const char *text)
{
    for (; *text; text++) {
        open_string_line();
        append_char(*text);
        close_full_string_line();
    }
}

/* Fill the data string with random characters until DECODER_MAIN will fit. */
static void pad_string(void)
{
    open_string_line();
    while (line_end - line != PADDED_LINE_START) {
        *line_end++ = next_random() % 34 + 93;
        close_full_string_line();
        open_string_line();
    }
}

/* Encrypt 4 bytes and store them as 5 printable base 85 digits. */
static void encode_block(void)
{
    unsigned int value = 0;
    int i;

    for (i = 0; i < BLOCK_BYTES; i++) {
        value += (unsigned int)(block[i] & 255) << (8 * i);
    }
    value ^= next_random();

    for (i = BLOCK_DIGITS - 1; i >= 0; i--) {
        unsigned int digit = value % BASE;

        value /= BASE;
        block[i] = digit < 32 ? digit + 95 : digit + 6;
    }
}

/* Replace block with the text for the next input bytes, and return how many
 * bytes were read. Fewer than BLOCK_BYTES means the input has ended. */
static size_t read_next_block(void)
{
    size_t bytes_read = fread(block, 1, BLOCK_BYTES, stdin);
    size_t length = 0;

    if (bytes_read > 0) {
        encode_block();
        length = BLOCK_DIGITS;
    }
    if (bytes_read < BLOCK_BYTES) {
        block[length++] = END_MARKERS[bytes_read];
    }
    block[length] = '\0';
    return bytes_read;
}

int main(int argc, char **argv)
{
    choose_key(argc > 1 ? argv[1] : "/dev/urandom");

    /* Key half */
    puts(POLYGLOT_HEADER);
    print_key();
    puts(KEY_TRAILER);

    /* Data half */
    append_wrapped_code(DECODER_HEAD);
    append_string_text(block);
    while (read_next_block() == BLOCK_BYTES) {
        append_string_text(block);
    }
    append_string_text(block);
    pad_string();

    append_text(DECODER_MAIN);
    flush_line();
    puts(DECODER_TAIL);
    return 0;
}
