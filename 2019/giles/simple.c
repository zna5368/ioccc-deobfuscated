#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Artificial reverb: the input WAV file is passed through pairs of
 * damped all-pass delay lines (one line per stereo channel), with a
 * small rotation that mixes the left and right channels between stages.
 *
 * usage: prog input [output [size [dry [wet [damping [feedback]]]]]]
 */

#define NUM_LINES 24     /* number of delay lines (12 left/right pairs) */
#define MIN_ACTIVE 12    /* always run at least this many delay lines */
#define SAMPLE_MAX 32767

/* 44-byte canonical WAV header (assumes little-endian, no padding) */
typedef struct {
    char    riff_id[4];      /* "RIFF" (not checked) */
    int32_t riff_size;
    char    wave_id[4];      /* "WAVE" */
    char    fmt_id[4];       /* "fmt " */
    int32_t fmt_size;
    int16_t audio_format;    /* 1 = PCM */
    int16_t channels;
    int32_t sample_rate;
    int32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;
    char    data_id[4];      /* "data" */
    int32_t data_size;
} WavHeader;

static FILE *output_file = 0, *input_file = 0;
static float *delay_buffer[NUM_LINES];

static void cleanup()
{
    if (output_file)
        fclose(output_file);
    if (input_file)
        fclose(input_file);
    for (int32_t line = 0; line < NUM_LINES; line++)
        if (delay_buffer[line])
            free(delay_buffer[line]);
}

static int fail(const char *message)
{
    fprintf(stderr, "%s\n", message);
    return 1;
}

/* limit a value to the range [low, high] */
static float clamp(float value, float low, float high)
{
    if (value > high)
        return high;
    if (value < low)
        return low;
    return value;
}

/* percentage rounded half away from zero */
static int percent(float value)
{
    return (int)((value > 0 ? 1 : -1) * .5f + 100 * value);
}

/* clip a sample to the 16-bit range */
static int16_t to_sample(float value)
{
    return (int16_t)(value > SAMPLE_MAX ? SAMPLE_MAX :
                     value > -SAMPLE_MAX ? value : -SAMPLE_MAX);
}

static int32_t write_pos[NUM_LINES], read_pos[NUM_LINES], line_length[NUM_LINES];
static float lowpass[NUM_LINES];
static float feedback, damping, undamped;

/* one damped all-pass filter stage built on a circular delay line */
static float allpass(int line, float input)
{
    float delayed = delay_buffer[line][read_pos[line]];
    float into_line = input + feedback * delayed;
    float output = delayed - into_line * feedback;

    /* one-pole low-pass filter softens high frequencies in the echo */
    lowpass[line] *= damping;
    lowpass[line] += into_line * undamped;
    delay_buffer[line][write_pos[line]] = lowpass[line];

    write_pos[line] = read_pos[line]++;
    if (read_pos[line] >= line_length[line])
        read_pos[line] = 0;
    return output;
}

int main(int argc, char **argv)
{
    /* size (seconds), dry mix, wet mix, damping, all-pass feedback */
    float param[5] = {1.60f, .30f, 0.5f, .06f, .77f};
    const float cos15 = .965926f, sin15 = .258819f;  /* stereo rotation */
    const float length_ratio = .64731f;   /* each pair is shorter than the last */
    const float right_ratio = .9f;        /* right line is 90% of left line */
    WavHeader header;
    float lengths[NUM_LINES];
    char *end;
    int32_t frame, line;

    for (int n = 0; n < 5; n++)
        if (argc > n + 3) {
            param[n] = strtod(argv[n + 3], &end);
            param[n] /= *end == '%' ? 100 : 1;
        }
    float room = param[0], dry = param[1], wet = param[2];
    damping = param[3];
    feedback = param[4];
    dry = clamp(dry, 0, 1);
    wet = clamp(wet, 0, 1);
    damping = clamp(damping, -1, 1);
    undamped = damping < 0 ? 1 + damping : 1 - damping;

    input_file = fopen(1 < argc ? argv[1] : "in.wav", "rb");
    if (!input_file)
        return fail("Bad input file");
    atexit(cleanup);
    setvbuf(input_file, 0, _IOFBF, 65536);
    if (!fread(&header, 44, 1, input_file))
        return fail("Bad input file");
    if (memcmp(header.wave_id, "WAVE", 4) || memcmp(header.fmt_id, "fmt ", 4) ||
        memcmp(header.data_id, "data", 4))
        return fail("Bad input file");

    int16_t channels = header.channels;
    float tail = room * header.sample_rate;   /* reverb tail in samples */
    tail = clamp(tail, 4e3f, 6e5f);
    int32_t total_frames = (int32_t)tail + header.data_size / (2 * channels);
    int16_t frame_in[2], frame_out[2];
    if (3 <= channels || header.audio_format != 1 || header.bits_per_sample != 16)
        return fail("Wave file must be 16 bits,1-2 channels.");

    /* the output is always 16-bit stereo PCM */
    header.audio_format = 1;
    header.channels = 2;
    header.block_align = 4;
    header.bits_per_sample = 16;
    header.byte_rate = 2 * header.byte_rate / channels;
    header.data_size = total_frames * 4;
    header.riff_size = 36 + header.data_size;

    output_file = fopen(argc > 2 ? argv[2] : "out.wav", "wb");
    if (!output_file)
        return fail("Can't open output file");
    setvbuf(output_file, 0, _IOFBF, 32768);
    fwrite(&header, 44, 1, output_file);
    printf("Size: %7.3f\nDry mix:%4d%%\nWet mix:%4d%%\nDamping:%4d%%\n",
           tail / header.sample_rate, percent(dry), percent(wet), percent(damping));

    /* delay line lengths: pairs of left/right lines, shrinking geometrically */
    tail /= 16;
    lengths[0] = tail;
    lengths[1] = tail * right_ratio;
    int32_t last_long = 0;
    for (line = 0; line < NUM_LINES; line++) {
        if (line > 1)
            lengths[line] = lengths[line - 2] * length_ratio;
        line_length[line] = (int32_t)lengths[line];
        lowpass[line] = read_pos[line] = 0;
        write_pos[line] = 1;
        if (lengths[line] > 99)
            last_long = (line - 1) | 1;
        delay_buffer[line] = calloc(line_length[line], sizeof(float));
    }
    /* only lines longer than 99 samples are used, but at least MIN_ACTIVE */
    int32_t active_lines = last_long + 1 < MIN_ACTIVE ? MIN_ACTIVE : last_long + 1;

    int end_of_input = 0;
    for (frame = 0; frame < total_frames; frame++) {
        /* after the input runs out, feed silence to let the echo die away */
        if (end_of_input ||
            (end_of_input = !fread(frame_in, 2 * channels, 1, input_file))) {
            channels = 1;
            frame_in[0] = 0;
        }
        float dry_left = frame_in[0], dry_right = frame_in[channels - 1];
        float left = dry_left, right = dry_right;

        for (line = 0; line < NUM_LINES; line += 2) {
            float out_left = left, out_right = right;
            if (line < active_lines) {
                out_left = allpass(line, left);
                out_right = allpass(line + 1, right);
            }
            /* rotate by 15 degrees to mix the two channels */
            left = cos15 * out_left - sin15 * out_right;
            right = sin15 * out_left + cos15 * out_right;
        }
        right *= wet;
        left *= wet;
        left += dry_left * dry;
        right += dry_right * dry;
        frame_out[0] = to_sample(left);
        frame_out[1] = to_sample(right);
        fwrite(frame_out, 4, 1, output_file);
    }
}
