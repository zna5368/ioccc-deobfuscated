#include<stdio.h>
#include<stdlib.h>

/*
 * Audio is 32-bit float samples at 44.1 kHz.  Every character of the
 * 5x8 pixel font is played as 6 columns (5 glyph columns plus one blank
 * spacing column) and each column lasts 1/75 of a second.
 */
#define SAMPLE_RATE         44100
#define COLUMNS_PER_SECOND  75
#define SAMPLES_PER_COLUMN  (SAMPLE_RATE / COLUMNS_PER_SECOND)   /* 588 */
#define GLYPH_WIDTH         5
#define COLUMNS_PER_CHAR    (GLYPH_WIDTH + 1)
#define GLYPH_HEIGHT        8
#define PATTERN_COUNT       256    /* every possible 8 pixel column */

/*
 * Pixel row y (0 = bottom) of a column is drawn around the frequency
 * BASE_FREQUENCY + ROW_SPACING * (2 * y + 1), in units of 55/2 Hz.
 * Each pixel is a cross made of an up and a down chirp.
 */
#define FREQUENCY_UNIT      55
#define BASE_FREQUENCY      16
#define ROW_SPACING         7
#define CHIRP_RATE          (FREQUENCY_UNIT * ROW_SPACING)          /* 385 */

/* one full sine period is stored in 1024 entries (+1 for interpolation) */
#define SINE_TABLE_SIZE     1024
#define SINE_PERIOD         (2 * SAMPLE_RATE)   /* sine_wave(t) = sin(pi*t/SAMPLE_RATE) */

/*
 * Font based on gohufont.  Each glyph is 5 columns, bit 0 of a column is
 * the bottom pixel and bit 7 the top pixel.
 */
static const unsigned char newline_glyph[GLYPH_WIDTH] = { 0x08, 0x1c, 0x2a, 0x08, 0x78 };

static const unsigned char font[95][GLYPH_WIDTH] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space */
    { 0x00, 0x00, 0xfd, 0x00, 0x00 }, /* ! */
    { 0x00, 0xe0, 0x00, 0xe0, 0x00 }, /* " */
    { 0x28, 0xfe, 0x28, 0xfe, 0x28 }, /* # */
    { 0x24, 0x52, 0xff, 0x52, 0x2c }, /* $ */
    { 0x23, 0x54, 0x2a, 0x15, 0x62 }, /* % */
    { 0x36, 0x49, 0x55, 0x22, 0x05 }, /* & */
    { 0x00, 0x00, 0xe0, 0x00, 0x00 }, /* ' */
    { 0x00, 0x18, 0x66, 0x81, 0x00 }, /* ( */
    { 0x00, 0x81, 0x66, 0x18, 0x00 }, /* ) */
    { 0x14, 0x08, 0x3e, 0x08, 0x14 }, /* asterisk */
    { 0x08, 0x08, 0x3e, 0x08, 0x08 }, /* + */
    { 0x00, 0x0d, 0x0e, 0x00, 0x00 }, /* , */
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, /* - */
    { 0x00, 0x03, 0x03, 0x00, 0x00 }, /* . */
    { 0x00, 0x03, 0x0c, 0x30, 0xc0 }, /* slash */
    { 0x3e, 0x45, 0x49, 0x51, 0x3e }, /* 0 */
    { 0x10, 0x20, 0x7f, 0x00, 0x00 }, /* 1 */
    { 0x21, 0x43, 0x45, 0x49, 0x31 }, /* 2 */
    { 0x22, 0x41, 0x49, 0x49, 0x36 }, /* 3 */
    { 0x0c, 0x14, 0x24, 0x7f, 0x04 }, /* 4 */
    { 0x72, 0x51, 0x51, 0x51, 0x4e }, /* 5 */
    { 0x3e, 0x51, 0x51, 0x51, 0x0e }, /* 6 */
    { 0x40, 0x40, 0x47, 0x58, 0x60 }, /* 7 */
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 8 */
    { 0x30, 0x49, 0x49, 0x49, 0x3e }, /* 9 */
    { 0x00, 0x00, 0x1b, 0x1b, 0x00 }, /* : */
    { 0x00, 0x35, 0x36, 0x00, 0x00 }, /* ; */
    { 0x00, 0x08, 0x14, 0x22, 0x41 }, /* < */
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, /* = */
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, /* > */
    { 0x20, 0x40, 0x45, 0x48, 0x30 }, /* ? */
    { 0x3e, 0x41, 0x5d, 0x55, 0x3d }, /* @ */
    { 0x7f, 0x90, 0x90, 0x90, 0x7f }, /* A */
    { 0xff, 0x91, 0x91, 0x91, 0x6e }, /* B */
    { 0x7e, 0x81, 0x81, 0x81, 0x42 }, /* C */
    { 0xff, 0x81, 0x81, 0x81, 0x7e }, /* D */
    { 0xff, 0x91, 0x91, 0x91, 0x81 }, /* E */
    { 0xff, 0x90, 0x90, 0x90, 0x80 }, /* F */
    { 0x7e, 0x81, 0x91, 0x91, 0x5e }, /* G */
    { 0xff, 0x10, 0x10, 0x10, 0xff }, /* H */
    { 0x00, 0x81, 0xff, 0x81, 0x00 }, /* I */
    { 0x06, 0x01, 0x01, 0x01, 0xfe }, /* J */
    { 0xff, 0x10, 0x28, 0x44, 0x83 }, /* K */
    { 0xff, 0x01, 0x01, 0x01, 0x01 }, /* L */
    { 0xff, 0x40, 0x30, 0x40, 0xff }, /* M */
    { 0xff, 0x60, 0x18, 0x06, 0xff }, /* N */
    { 0x7e, 0x81, 0x81, 0x81, 0x7e }, /* O */
    { 0xff, 0x90, 0x90, 0x90, 0x60 }, /* P */
    { 0x7e, 0x81, 0x85, 0x82, 0x7d }, /* Q */
    { 0xff, 0x90, 0x90, 0x98, 0x67 }, /* R */
    { 0x62, 0x91, 0x91, 0x91, 0x4e }, /* S */
    { 0x80, 0x80, 0xff, 0x80, 0x80 }, /* T */
    { 0xfe, 0x01, 0x01, 0x01, 0xfe }, /* U */
    { 0xf0, 0x0c, 0x03, 0x0c, 0xf0 }, /* V */
    { 0xfc, 0x03, 0x1c, 0x03, 0xfc }, /* W */
    { 0xc7, 0x28, 0x10, 0x28, 0xc7 }, /* X */
    { 0xc0, 0x20, 0x1f, 0x20, 0xc0 }, /* Y */
    { 0x87, 0x89, 0x91, 0xa1, 0xc1 }, /* Z */
    { 0x00, 0x00, 0xff, 0x81, 0x81 }, /* [ */
    { 0x00, 0xc0, 0x30, 0x0c, 0x03 }, /* backslash */
    { 0x81, 0x81, 0xff, 0x00, 0x00 }, /* ] */
    { 0x10, 0x20, 0x40, 0x20, 0x10 }, /* ^ */
    { 0x01, 0x01, 0x01, 0x01, 0x01 }, /* _ */
    { 0x00, 0x80, 0x40, 0x20, 0x00 }, /* ` */
    { 0x02, 0x15, 0x15, 0x15, 0x0f }, /* a */
    { 0xff, 0x09, 0x11, 0x11, 0x0e }, /* b */
    { 0x0e, 0x11, 0x11, 0x11, 0x11 }, /* c */
    { 0x0e, 0x11, 0x11, 0x12, 0xff }, /* d */
    { 0x0e, 0x15, 0x15, 0x15, 0x0d }, /* e */
    { 0x00, 0x7f, 0x90, 0x90, 0x00 }, /* f */
    { 0x38, 0x45, 0x45, 0x49, 0x7e }, /* g */
    { 0xff, 0x08, 0x10, 0x10, 0x0f }, /* h */
    { 0x00, 0x10, 0x5f, 0x01, 0x00 }, /* i */
    { 0x01, 0x11, 0x5e, 0x00, 0x00 }, /* j */
    { 0xff, 0x04, 0x0c, 0x12, 0x01 }, /* k */
    { 0x00, 0x80, 0xfe, 0x01, 0x01 }, /* l */
    { 0x1f, 0x10, 0x1f, 0x10, 0x0f }, /* m */
    { 0x1f, 0x10, 0x10, 0x10, 0x0f }, /* n */
    { 0x0e, 0x11, 0x11, 0x11, 0x0e }, /* o */
    { 0x7f, 0x24, 0x44, 0x44, 0x38 }, /* p */
    { 0x38, 0x44, 0x44, 0x48, 0x7f }, /* q */
    { 0x1f, 0x08, 0x10, 0x10, 0x08 }, /* r */
    { 0x09, 0x15, 0x15, 0x15, 0x02 }, /* s */
    { 0x10, 0xfe, 0x11, 0x11, 0x00 }, /* t */
    { 0x1e, 0x01, 0x01, 0x02, 0x1f }, /* u */
    { 0x18, 0x06, 0x01, 0x06, 0x18 }, /* v */
    { 0x1e, 0x01, 0x0e, 0x01, 0x1e }, /* w */
    { 0x11, 0x0a, 0x04, 0x0a, 0x11 }, /* x */
    { 0x38, 0x05, 0x05, 0x09, 0x3e }, /* y */
    { 0x11, 0x13, 0x15, 0x19, 0x11 }, /* z */
    { 0x10, 0x10, 0x6e, 0x81, 0x81 }, /* { */
    { 0x00, 0x00, 0xff, 0x00, 0x00 }, /* | */
    { 0x81, 0x81, 0x6e, 0x10, 0x10 }, /* } */
    { 0x08, 0x10, 0x08, 0x04, 0x08 }, /* ~ */
};

static float sine_table[SINE_TABLE_SIZE + 1];

/* pixel pattern of column `column` (0..5) of character `ch` */
static int glyph_column(int ch, int column)
{
    if (column >= GLYPH_WIDTH)
        return 0;                       /* spacing between characters */
    if (ch == '\n')
        return newline_glyph[column];
    if (ch >= ' ' && ch <= '~')
        return font[ch - ' '][column];
    return 0;                           /* anything else is drawn as a space */
}

/*
 * Fill the sine table by repeatedly rotating the vector (cos, sin) by
 * 2*pi/1024 radians:
 *   sin(k+1) = sin(step) * cos(k) + cos(step) * sin(k)
 *   cos(k+1) = cos(step) * cos(k) - sin(step) * sin(k)
 */
static void build_sine_table(void)
{
    float sin_step = 6.135885e-3;       /* sin(2*pi/1024) */
    float cos_step = -1.882472e-5 + 1;  /* cos(2*pi/1024) */
    float cosine = 1;
    int k;

    sine_table[0] = 0;
    for (k = 0; k < SINE_TABLE_SIZE - 1; ++k) {
        float sine = sine_table[k];
        sine_table[k + 1] = sin_step * cosine + cos_step * sine;
        cosine = cos_step * cosine - sin_step * sine;
    }
    sine_table[SINE_TABLE_SIZE] = 0;    /* sin(2*pi), used by the interpolation */
}

/* sin(pi * t / SAMPLE_RATE) by linear interpolation in the sine table */
static float sine_wave(int t)
{
    static int table_ready = 0;
    int position, index, fraction;

    if (!table_ready) {
        build_sine_table();
        table_ready = 1;
    }
    /* table position measured in 1/SAMPLE_RATE steps */
    position = (t % SINE_PERIOD + SINE_PERIOD) % SINE_PERIOD * (SINE_TABLE_SIZE / 2);
    index = position / SAMPLE_RATE;
    fraction = position % SAMPLE_RATE;
    return (sine_table[index] * (SAMPLE_RATE - fraction) +
            sine_table[index + 1] * fraction) / SAMPLE_RATE;
}

/*
 * Render one column with the given pixel pattern.  Samples are written to
 * out[0], out[stride], out[2*stride], ... until end is reached so that
 * several channels can be interleaved in one buffer.
 */
static void render_column(float *out, float *end, int stride, int pattern)
{
    int sample, row, lit_pixels;

    for (sample = 0; out < end; ++sample, out += stride) {
        float level, window;

        lit_pixels = pattern == 0;      /* avoid dividing by zero */
        *out = 0;
        for (row = 0; row < GLYPH_HEIGHT; ++row) {
            if (pattern >> row & 1) {
                int frequency = FREQUENCY_UNIT * (BASE_FREQUENCY + ROW_SPACING * (2 * row + 1));
                /* cosine of a quadratic phase: an up and down chirp */
                int chirp = SAMPLE_RATE / 2 - CHIRP_RATE * sample
                    + CHIRP_RATE * sample * sample / SAMPLES_PER_COLUMN;
                *out += sine_wave(frequency * sample) * sine_wave(chirp);
                ++lit_pixels;
            }
        }
        /* Hann window z = sin^2 turned into a smoother cosine sum window */
        level = sine_wave(sample * COLUMNS_PER_SECOND);
        window = level * level;
        *out = window * (1 + 3 * window + 2 * window * window) / 6 * *out / lit_pixels;
    }
}

/* find the pixel pattern whose waveform exactly matches the samples */
static int match_column(float **reference, float *samples, int channels, int channel)
{
    int pattern, sample;

    for (pattern = PATTERN_COUNT - 1; pattern > 0; --pattern) {
        for (sample = 0; sample < SAMPLES_PER_COLUMN; ++sample)
            if (reference[pattern][sample] != samples[channels * sample + channel])
                break;
        if (sample == SAMPLES_PER_COLUMN)
            return pattern;
    }
    return 0;
}

/* find the character drawn by the five column patterns, or -1 */
static int match_character(int *patterns)
{
    int ch, column;

    for (ch = '\n'; ch <= '~'; ch = ch == '\n' ? ' ' : ch + 1) {
        for (column = 0; column < GLYPH_WIDTH && glyph_column(ch, column) == patterns[column]; ++column)
            ;
        if (column == GLYPH_WIDTH)
            return ch;
    }
    return -1;
}

/* decode channel `channel` out of `channels` interleaved channels */
static void decode(int channels, int channel, FILE *in)
{
    int column, ch;
    int patterns[GLYPH_WIDTH];
    float **reference = malloc(sizeof(float *) * PATTERN_COUNT);
    float *samples = malloc(sizeof(float) * SAMPLES_PER_COLUMN * channels);
    int pattern;

    /* render the waveform of every possible column */
    for (pattern = 0; pattern < PATTERN_COUNT; ++pattern) {
        reference[pattern] = malloc(sizeof(float) * SAMPLES_PER_COLUMN);
        render_column(reference[pattern], reference[pattern] + SAMPLES_PER_COLUMN, 1, pattern);
    }

    for (;;) {
        for (column = 0; column < GLYPH_WIDTH; ++column) {
            fread(samples, sizeof(float), SAMPLES_PER_COLUMN * channels, in);
            patterns[column] = match_column(reference, samples, channels, channel);
        }
        /* skip the spacing column */
        if (fread(samples, sizeof(float), SAMPLES_PER_COLUMN * channels, in) != (size_t)(SAMPLES_PER_COLUMN * channels))
            break;
        ch = match_character(patterns);
        if (ch >= 0)
            putchar(ch);
    }

    for (pattern = PATTERN_COUNT - 1; pattern > 0; --pattern)
        free(reference[pattern]);
    free(reference);
    fclose(in);
}

int main(int argc, char **argv)
{
    char *usage = malloc(256);
    char *usage_pos = usage;
    int channels, channel, column, finished, total;
    FILE **in;
    float *samples;
    int *chars;

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'd') {
        decode(atoi(argv[2]), atoi(argv[3]), argc < 5 ? stdin : fopen(argv[4], "r"));
        return 0;
    }

    usage[0] = '\0';
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h') {
        sprintf(usage, "Usage\n    %s [-h] [-d #c cid] file1 file2 ... (or stdin)", argv[0]);
        argc = 1;                       /* the usage text is read through stdin */
    }

    /* one channel per input file, stdin if there are none */
    channels = argc < 2 ? 1 : argc - 1;
    in = malloc(sizeof(FILE *) * channels);
    in[0] = stdin;
    if (argc > 1)
        for (channel = 0; channel < channels; ++channel)
            in[channel] = fopen(argv[channel + 1], "r");

    total = SAMPLES_PER_COLUMN * channels;
    samples = malloc(sizeof(float) * total);
    chars = malloc(sizeof(int) * channels);

    for (;;) {
        /* feed the usage text one character at a time into stdin */
        if (*usage) {
            if (*usage_pos == '\0')
                break;
            ungetc(*usage_pos++, stdin);
        }

        finished = 0;
        for (channel = channels; channel--; ) {
            chars[channel] = getc(in[channel]);
            if (chars[channel] < 0) {
                chars[channel] = 0;     /* finished inputs play spaces */
                ++finished;
            }
        }
        if (finished == channels)
            break;

        for (column = 0; column < COLUMNS_PER_CHAR; ++column) {
            for (channel = 0; channel < channels; ++channel)
                render_column(samples + channel, samples + total, channels,
                              glyph_column(chars[channel], column));
            fwrite(samples, sizeof(float), total, stdout);
        }
        fflush(stdout);
    }

    while (fclose(in[--channels]), channels)
        ;
    free(in);
    free(samples);
    free(chars);
    free(usage);
    return 0;
}
