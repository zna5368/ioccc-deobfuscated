#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <io.h>
#include <fcntl.h>
#define SET_BINARY_MODE(fp) _setmode(_fileno(fp), _O_BINARY)
#else
#define SET_BINARY_MODE(fp)
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * dtmf.bin layout:
 *   bytes   0..43   template WAV header (16 kHz, mono, 16-bit PCM, sizes 0)
 *   bytes  44..171  8 resonators x {sin(w), cos(w)} as doubles, where
 *                   w = 2*pi*freq/16000 for the frequencies
 *                   697 770 852 941 (rows) 1209 1336 1477 1633 (columns)
 *   bytes 172..252  key table: index = column*4 + row, repeated with letters
 *                   "147*2580369#ABCD1gp*ajt0dmw#ABCD1hq*bku0enx#ABCD"
 *                   "1ir*clv0foy#ABCD14s*258036z#ABCD"
 */
#define DATA_FILE      "dtmf.bin"
#define WAV_HEADER_LEN 44
#define NUM_RESONATORS 8
#define NUM_KEYS       16
#define KEY_TABLE_LEN  81

#define SAMPLE_RATE      16000.0
#define SAMPLES_PER_KEY  3200       /* 0.2 seconds of audio per key */
#define TONE_SAMPLES     1900       /* tone length before fade out */
#define FADE_STEP        0.03
#define AMPLITUDE        4900.0
#define DECODE_DECAY     0.994      /* resonator damping when listening */

/* resonator state: current and previous output of each resonator */
struct resonator {
    double y0, y1;
};

static struct resonator res[NUM_RESONATORS];
static double res_sin[NUM_RESONATORS];
static double res_cos[NUM_RESONATORS];

/* the order in which the resonators are processed (affects rounding/ties) */
static const int order[NUM_RESONATORS] = { 1, 5, 3, 6, 2, 4, 7, 0 };

/*
 * Advance a damped two-pole resonator by one sample.
 * Returns the new (undamped) output value.
 */
static double resonate(int r, double input, double decay)
{
    double out = (1 - decay) * input + 2 * res_cos[r] * res[r].y0
               - res[r].y1 * decay;
    res[r].y1 = res[r].y0;
    res[r].y0 = out * decay;
    return out;
}

static int encode(const char *text, unsigned char *header, const char *keys)
{
    FILE *out = stdout;
    uint32_t size;
    double fade = 0;

    setvbuf(out, 0, _IOFBF, 65536);
    SET_BINARY_MODE(out);

    /* fill in the data and RIFF chunk sizes, then write the header */
    size = 2 * SAMPLES_PER_KEY * strlen(text);
    memcpy(header + 40, &size, 4);
    size += 36;
    memcpy(header + 4, &size, 4);
    fwrite(header, WAV_HEADER_LEN, 1, out);

    while (*text) {
        const char *key = strchr(keys, *text++);
        int n;

        memset(res, 0, sizeof res);
        if (key) {
            /* kick the row and column resonators with an impulse */
            int index = key - keys;
            int row = index % 4;
            int col = (index / 4) % 4;
            res[row].y0 = res_sin[row];
            res[4 + col].y0 = res_sin[4 + col];
        }

        for (n = SAMPLES_PER_KEY; n; --n) {
            double mix = 0;
            int16_t sample;
            int i;

            for (i = 0; i < NUM_RESONATORS; ++i)
                mix += resonate(order[i], 0, 1) * fade * AMPLITUDE;
            sample = mix;
            fwrite(&sample, 2, 1, out);

            /* fade in at the start of a key and fade out at the end */
            fade += (n > SAMPLES_PER_KEY - TONE_SAMPLES ? 1 : -1) * FADE_STEP;
            fade = fade > 1 ? 1 : (fade > 0 ? fade : 0);
        }
    }
    return 0;
}

/* print the key that was detected most often during the last tone */
static void print_key(const double counts[NUM_KEYS], const char *keys)
{
    double best = -1;
    char key = 0;
    int i;

    for (i = 0; i < NUM_RESONATORS; ++i) {
        int k = 2 * order[i];
        if (counts[k] > best) {
            best = counts[k];
            key = keys[k];
        }
        if (counts[k + 1] > best) {
            best = counts[k + 1];
            key = keys[k + 1];
        }
    }
    putchar(key);
}

static int decode(unsigned char *ref_header, const char *keys)
{
    FILE *in = stdin;
    unsigned char header[WAV_HEADER_LEN];
    unsigned char raw[4];
    uint32_t rate, upsample, skip, p;
    uint16_t block_align, bytes_per_sample;
    int16_t s16;
    double step, phase = 0;
    double prev = 0, sample, value;
    double interp[4] = { 0 };       /* resampled samples waiting for output */
    double peak = 1e-4;             /* loudest tone energy seen so far */
    double counts[NUM_KEYS] = { 0 };
    int in_tone = 0;

    setvbuf(in, 0, _IOFBF, 65536);
    SET_BINARY_MODE(in);

    if (!fread(header, WAV_HEADER_LEN, 1, in))
        goto read_error;
    /* "WAVEfmt ", fmt chunk size 16 and PCM format must match */
    if (memcmp(header + 8, ref_header + 8, 14)) {
invalid:
        fprintf(stderr, "%s\n", "Invalid WAV input");
        return 1;
    }

    /* resample the input to 16 kHz: upsample by an integer factor with
     * linear interpolation, then step through it at a fractional rate */
    memcpy(&rate, header + 24, 4);
    step = SAMPLE_RATE / rate;
    upsample = step;
    if (upsample != step)
        ++upsample;
    step /= upsample;

    memcpy(&bytes_per_sample, header + 34, 2);
    bytes_per_sample /= 8;
    memcpy(&block_align, header + 32, 2);
    if (!block_align || !bytes_per_sample || bytes_per_sample > 4)
        goto invalid;
    block_align -= bytes_per_sample;    /* bytes of the other channels */

    while (fread(raw, bytes_per_sample, 1, in)) {
        /* only the first channel is used */
        skip = block_align;
        while (skip-- && getc(in) >= 0)
            ;
        if (bytes_per_sample > 1) {
            /* use the most significant 16 bits */
            memcpy(&s16, raw + bytes_per_sample - 2, 2);
            sample = s16 / 32768.0;
        } else
            sample = raw[0] / 128.0 - 1;

        for (p = upsample; p; --p) {
            int k;

            value = (prev - sample) * p / upsample + sample;

            /* spread the value over the neighbouring output samples */
            for (k = 0; k < 3; ++k) {
                double weight = k - phase;
                weight = weight < 0 ? 1 + weight : 1 - weight;
                interp[k] += weight > 0 ? weight * step * value : 0;
            }

            phase += step;
            if (phase >= 1) {
                double best[2] = { 0, 0 };
                uint32_t found[2] = { 0, 0 };
                double total = 0;
                int is_tone, i;

                /* run the 16 kHz sample through every resonator and
                 * measure how loud each frequency is */
                for (i = 0; i < NUM_RESONATORS; ++i) {
                    int r = order[i];
                    int group = r / 4;      /* 0 = row, 1 = column */
                    double re, im, energy;

                    resonate(r, interp[0], DECODE_DECAY);
                    re = res[r].y0 - DECODE_DECAY * res_cos[r] * res[r].y1;
                    im = DECODE_DECAY * res[r].y1 * res_sin[r];
                    energy = re * re + im * im;
                    total += energy;
                    if (energy >= best[group]) {
                        found[group] = r;
                        best[group] = energy;
                    }
                }

                /* a tone is present when the energy is near the peak
                 * (with hysteresis so it does not flicker) */
                is_tone = in_tone ? total > peak * 0.45 : total > peak * 0.6;
                if (is_tone) {
                    if (!in_tone)
                        memset(counts, 0, sizeof counts);
                    if (total > peak)
                        peak = total;
                    ++counts[(found[1] - 4) * 4 + found[0]];
                } else if (in_tone)
                    print_key(counts, keys);
                in_tone = is_tone;

                interp[0] = interp[1];
                interp[1] = interp[2];
                interp[2] = interp[3];
                --phase;
            }
        }
        prev = sample;
    }
    if (ferror(in))
        goto read_error;
    if (in_tone)
        print_key(counts, keys);
    puts("");
    return 0;

read_error:
    fprintf(stderr, "%s\n", "Read error");
    return 1;
}

int main(int argc, char **argv)
{
    unsigned char wav_header[WAV_HEADER_LEN];
    double coeffs[2 * NUM_RESONATORS];
    char keys[KEY_TABLE_LEN];
    FILE *data;
    int failed, i;

    data = fopen(DATA_FILE, "rb");
    if (!data) {
cannot_read:
        fprintf(stderr, "%s\n", "Cannot read " DATA_FILE);
        return 1;
    }
    failed = !fread(wav_header, WAV_HEADER_LEN, 1, data)
          || !fread(coeffs, sizeof coeffs, 1, data)
          || !fread(keys, KEY_TABLE_LEN, 1, data);
    fclose(data);
    if (failed)
        goto cannot_read;

    for (i = 0; i < NUM_RESONATORS; ++i) {
        res_sin[i] = coeffs[2 * i];
        res_cos[i] = coeffs[2 * i + 1];
    }

    if (argc > 1)
        return encode(argv[1], wav_header, keys);
    return decode(wav_header, keys);
}
