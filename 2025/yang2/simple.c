#include<stdio.h>

/*
 * Fern: encode stdin as a "zoltraak" program.
 *
 * Every output line after the prelude is one bit (least significant first):
 * a line holding the magic word is a 1 bit, an empty line is a 0 bit.
 *
 *   literal: 8 bits of (byte ^ 0x20), then a 1 bit                 (9 lines)
 *   repeat : 8 bits of (length - 3), a 0 bit,
 *            8 bits of (distance - 1), then a 1 bit                 (18 lines)
 *
 * A literal counts as one instruction and a repeat as two; the distance of
 * a repeat is measured in these instruction units.
 */

#define BUFFER_SIZE   (1 << 17)
#define BUFFER_MASK   (BUFFER_SIZE - 1)   /* ring buffer index mask */
#define LOOKAHEAD     (BUFFER_MASK / 2)   /* bytes read ahead of the encoder */
#define MIN_MATCH     3
#define MATCH_LIMIT   258                 /* match lengths are < 258 */
#define MAX_DISTANCE  256                 /* instructions we may look back */
#define DEPTH_LIMIT   998                 /* generated program has k[999] */

static char input[BUFFER_SIZE];           /* ring buffer of input bytes */

/* instruction number that starts at this byte, or -1 if it is not the
   first byte of an instruction (i.e. it is inside a repeat) */
static int instruction_at[BUFFER_SIZE];

/* how deeply nested the generated program's repeat stack becomes when it
   replays this byte */
static int replay_depth[BUFFER_SIZE];

/* number of 1 bits: each costs a "zoltraak" line, so fewer is cheaper */
static int count_one_bits(int value)
{
    return value ? value % 2 + count_one_bits(value / 2) : 0;
}

/* depth needed to replay byte (source + length) when the match source is
   'source' and the bytes are being produced at 'current'; the match may
   overlap the bytes it is producing, in which case it repeats with period
   (current - source) */
static int depth_of_match_byte(int source, int current, int length)
{
    int past_end = source + length - current;
    int period;

    if (past_end < 0)
        return replay_depth[(source + length) & BUFFER_MASK];
    period = current - source;
    return replay_depth[(source + past_end % period) & BUFFER_MASK]
           + 2 * past_end / period;
}

/* Constant header of every generated program.  It makes the C
   preprocessor turn each magic word into "case __LINE__: a(__LINE__);"
   inside a switch on setjmp(), includes itself with #include __FILE__ to
   read the code lines, and uses longjmp() to jump back to earlier lines
   when a repeat instruction is executed. */
static void print_prelude(const char *word)
{
    printf("#ifndef %s\n", word);
    fputs("#include<stdio.h>\n"
          "#include<setjmp.h>\n"
          "#define p (void)(I=1+_):\n"
          "#define q p(i=3,--o,longjmp(j,*o--)):\n"
          "#define r __LINE__\n"
          "jmp_buf j;int s,i,I,l,k[999]={1<<30},*o=k;"
          "void a(int _){i%2?s=0,--i,p _%9?(void)(s|=1<<(_-I)):"
          "_-I-17?(putchar(s^32),s=0,--*o)?"
          "q(l=s%256+3,s=s/512+3,i)?(*o-=l)>0?s=i=0,"
          "q((o[2]=*o<l?*o:l),*++o=I-(i=1),++o,longjmp(j,_-s*9));}"
          "int main(void){switch(setjmp(j)){default:\n", stdout);
    printf("#define %s case r:a(r);\n", word);
    fputs("#define e case r+1:s=0;i&=2;I=r+2;\n"
          "#include __FILE__\n", stdout);
    printf("#undef %s\n", word);
    printf("#define %s\n", word);
    fputs("#undef e\n"
          "#define e\n"
          "}return 0;}\n"
          "#else\n"
          "e\n"
          "#endif\n", stdout);
}

int main(void)
{
    int window_start = 0;   /* oldest byte still usable as a match source */
    int encoded = 0;        /* number of bytes encoded so far */
    int read_count = 0;     /* number of bytes read so far */
    int instruction = 0;    /* number of instruction units emitted so far */
    int best_source = 0, best_instruction = 0, best_length;
    int ch, pos, offset, length, code, i;

    /* In C a character constant is an int, in C++ it is a char, so a C++
       build uses the shorter word "z" instead of "zoltraak". */
    const char *word = sizeof('z') > 1 ? "zoltraak" : "z";

    print_prelude(word);

    for (;;) {
        /* keep the ring buffer filled ahead of the encoder */
        while (read_count - encoded < LOOKAHEAD && (ch = getchar()) != EOF)
            input[read_count++ & BUFFER_MASK] = ch;
        if (encoded == read_count)
            break;

        /* slide the window past bytes that cannot start a repeat */
        while (window_start < encoded) {
            int start = instruction_at[window_start & BUFFER_MASK];
            if (start >= 0 && instruction - start <= MAX_DISTANCE)
                break;
            window_start++;
        }

        /* find the longest earlier match that starts on an instruction */
        best_length = -1;
        for (offset = 0; offset < encoded - window_start; offset++) {
            int candidate;

            pos = window_start + offset;
            candidate = instruction_at[pos & BUFFER_MASK];
            if (candidate < 0)
                continue;

            for (length = 0;
                 length < MATCH_LIMIT && encoded + length < read_count &&
                 input[(pos + length) & BUFFER_MASK] ==
                     input[(encoded + length) & BUFFER_MASK] &&
                 depth_of_match_byte(pos, encoded, length) < DEPTH_LIMIT;
                 length++) {
            }

            if (length > best_length ||
                (length == best_length &&
                 count_one_bits(instruction - candidate - 1) <
                     count_one_bits(instruction - best_instruction - 1))) {
                best_source = pos;
                best_instruction = candidate;
                best_length = length;
            }
        }

        if (best_length >= MIN_MATCH) {
            /* repeat instruction */
            for (i = 0; i < best_length; i++) {
                int slot = encoded++ & BUFFER_MASK;
                replay_depth[slot] =
                    replay_depth[(best_source + i) & BUFFER_MASK] + 2;
                instruction_at[slot] = i == 0 ? instruction : -1;
            }
            code = (best_length - MIN_MATCH)
                   | (instruction - best_instruction - 1) << 9
                   | 1 << 17;
            instruction += 2;
        } else {
            /* literal instruction */
            int slot = encoded++ & BUFFER_MASK;
            instruction_at[slot] = instruction++;
            code = ((input[slot] & 255) ^ 0x20) | 0x100;
            replay_depth[slot] = 1;
        }

        /* one line per bit; the final 1 bit marks the end */
        for (; code; code /= 2)
            puts(code & 1 ? word : "");
    }
    return 0;
}
