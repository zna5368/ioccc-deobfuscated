#include<math.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>

#define SAMPLE_RATE      44100
#define NUM_SOURCES      13      /* sounds per layer */
#define WAVE_LAYERS      15      /* layers built by the first pass */
#define SURF_LAYERS      10      /* layers built by the second pass */
#define BUBBLE_LENGTH    28098   /* samples in one bubble */
#define WAVE_LENGTH      300000  /* room for one mixed wave */
#define DEFAULT_SECONDS  300

/* uniform random number in [0, 1] */
static double random_unit(void)
{
    return (double)rand() / RAND_MAX;
}

/*
 * The single filter used for everything.  It is three damped rotating
 * (complex) resonators that are all fed the same input.  Each resonator
 * has a pair of state values in state[] and loses energy at the rate
 * decay, decay^2 and decay^3.  (cos_w, sin_w) is how far the resonator
 * rotates per sample: (1, 0) gives a plain smoothing filter while other
 * values make it ring at a pitch.
 */
static double filter(double *state, double decay, double cos_w, double sin_w,
                     double input)
{
    double damping[] = { decay, decay * decay, decay * decay * decay };
    int stage;

    for (stage = 0; stage < 3; stage++) {
        double *re = &state[2 * stage];
        double *im = &state[2 * stage + 1];
        double old_re = *re;

        *re = (*re * cos_w - *im * sin_w) * damping[stage] + input;
        *im = (old_re * sin_w + *im * cos_w) * damping[stage];
    }
    return sqrt(1 - decay) * 20 * (state[0] - 2 * state[2] + state[4]);
}

double *source_sound[NUM_SOURCES];                 /* bubbles, later whole waves */
int     source_length[NUM_SOURCES];
double *layer_sound[WAVE_LAYERS * NUM_SOURCES];    /* every sound of every layer */
int     layer_length[WAVE_LAYERS * NUM_SOURCES];

double filter_state[18][6];   /* state for the many small filters */
double hiss_state[6];         /* state for the high pitched hiss filter */

int    wave_position[3];      /* sample counter of each playing wave voice */
int    wave_id[3];            /* which layer_sound[] each voice plays (0 = none) */
double wave_speed[3];         /* playback speed of each voice */

double note_energy[10];       /* how loudly each of the 10 notes is ringing */
double rumble_noise, wash_noise, splash_noise;
int    num_samples;

/*
 * Build layers of sounds.  Every sound in a layer is the sum of two random
 * sounds from the layer below: one is stretched in time, the other is
 * stretched and delayed.  Layer 0 is made from source_sound[].  Repeating
 * this turns a single bubble into the noise of a breaking wave.
 *
 * pass 0 builds WAVE_LAYERS layers with growing delays (bubbles to waves),
 * pass 1 builds SURF_LAYERS layers with short delays from the mixed waves.
 */
static void build_layers(int pass)
{
    double **below = source_sound;
    int     *below_length = source_length;
    int      num_layers = pass ? SURF_LAYERS : WAVE_LAYERS;
    int      layer, k;

    for (layer = 0; layer < num_layers; layer++) {
        int first = layer * NUM_SOURCES;

        for (k = NUM_SOURCES - 1; k >= 0; k--) {
            int other = rand() % NUM_SOURCES;
            double delay, stretch1, stretch2, gain1, gain2, swap, *out;
            int length, j;

            if (pass) {
                delay = random_unit() * SAMPLE_RATE / 2;
                stretch1 = 1;
                stretch2 = 1 + random_unit() / 5;
                gain2 = .3;
            } else {
                delay = (.2 + random_unit()) * SAMPLE_RATE / 3 * pow(1.1, layer);
                stretch1 = 1.2 + random_unit() / 10;
                stretch2 = 1.3 + random_unit() / 5;
                gain2 = .5;
            }
            if (rand() % 2) {
                swap = stretch1;
                stretch1 = stretch2;
                stretch2 = swap;
            }
            gain1 = gain2 + random_unit();
            gain2 += random_unit();

            length = fmax(below_length[k] / stretch1,
                          below_length[other] / stretch2 + delay) + 1;
            layer_length[first + k] = length;
            layer_sound[first + k] = out = calloc(length, sizeof(double));

            for (j = length - 1; j >= 0; j--) {
                int pos1 = j * stretch1;
                int pos2 = fmax(j - delay, 0) * stretch2;

                out[j] = gain1 * (pos1 < below_length[k] ? below[k][pos1] : 0)
                       + gain2 * (pos2 < below_length[other] ? below[other][pos2] : 0);
            }
        }
        below = layer_sound + first;
        below_length = layer_length + first;
    }
}

static void write_u32(FILE *out, int value)
{
    int i;

    for (i = 0; i < 4; i++)
        fputc(value >> i * 8, out);
}

static void write_u16(FILE *out, int value)
{
    fputc(value, out);
    fputc(value >> 8, out);
}

int main(int argc, char **argv)
{
    /* each scale is five semitone offsets, one per hex digit */
    int scales[] = { 0xB7540, 0x97420, 0x87320, 0xA7530 };
    int note_history[256];
    int note_count, scale, phrase_length, rhythm_step, pattern_shift;
    double wave[3], drift[4], hiss, music, x, y, fade;
    char *filename = "sound.wav", *end;
    int i, j, k;
    FILE *out;

    /* arguments: duration in seconds, random seed, output file name */
    srand(time(0));
    while (*++argv) {
        int value = strtol(*argv, &end, 0);

        if (value && !*end) {
            if (num_samples)
                srand(value);
            else
                num_samples = value * SAMPLE_RATE;
        } else {
            filename = *argv;
        }
    }
    if (!num_samples)
        num_samples = SAMPLE_RATE * DEFAULT_SECONDS;

    /* make 13 bubbles: ringing filtered noise that dies away quickly */
    for (k = NUM_SOURCES - 1; k >= 0; k--) {
        double state[6] = { 0 };

        source_length[k] = BUBBLE_LENGTH;
        source_sound[k] = calloc(BUBBLE_LENGTH, sizeof(double));
        for (i = 0; i < BUBBLE_LENGTH; i++)
            source_sound[k][i] = filter(state, .998, .9995, .0316, random_unit() - .5)
                               * pow(.1, i * 8. / SAMPLE_RATE) - 8e-6;
    }

    /* layer bubbles into waves */
    build_layers(0);

    /* mix one sound from every layer, quieter for higher layers, into 13 waves */
    for (k = NUM_SOURCES - 1; k >= 0; k--) {
        int layer;

        source_sound[k] = calloc(WAVE_LENGTH, sizeof(double));
        for (layer = WAVE_LAYERS - 1; layer >= 0; layer--) {
            int src = layer * NUM_SOURCES + (k + layer) % NUM_SOURCES;
            int offset = rand() % 3000;
            double gain = pow(.63, layer);

            for (j = layer_length[src] - 1; j >= 0; j--)
                source_sound[k][j + offset] += layer_sound[src][j] * gain;
            if (layer_length[src] + offset > source_length[k])
                source_length[k] = layer_length[src] + offset;
        }
    }

    /* layer the waves again into overlapping surf sounds */
    build_layers(1);

    out = filename[0] != '-' ? fopen(filename, "wb") : stdout;

    /* WAV header: 16 bit stereo PCM */
    fputs("RIFF", out);
    write_u32(out, num_samples * 4 + 36);
    fputs("WAVE", out);
    fputs("fmt ", out);
    write_u32(out, 16);               /* size of fmt chunk */
    write_u16(out, 1);                /* PCM */
    write_u16(out, 2);                /* channels */
    write_u32(out, SAMPLE_RATE);
    write_u32(out, SAMPLE_RATE * 4);  /* bytes per second */
    write_u16(out, 4);                /* bytes per frame */
    write_u16(out, 16);               /* bits per sample */
    fputs("data", out);
    write_u32(out, num_samples * 4);

    for (i = 0; i < num_samples; i++) {
        int minute_pos, beat, beat_length = SAMPLE_RATE / 4;

        /* three voices, each playing a random surf sound at a random speed */
        for (k = 2; k >= 0; k--) {
            int pos;

            j = wave_id[k];
            if (!j || wave_position[k] > layer_length[j]) {
                j = wave_id[k] = 4 * NUM_SOURCES + rand() % (6 * NUM_SOURCES);
                wave_speed[k] = 1 + random_unit() / 3;
                wave_position[k] = 0;
            }
            pos = wave_position[k] * wave_speed[k];
            wave[k] = pos < layer_length[j] ? layer_sound[j][pos] : 0;
            wave_position[k]++;
        }

        /* background noises */
        rumble_noise = rumble_noise * .96 + random_unit() - .5;
        wash_noise = wash_noise * .99 + (x = random_unit() - .5);
        splash_noise = splash_noise * .8 + x;
        hiss = filter(hiss_state, .7, .71, .71, random_unit() - .5) * .6;
        hiss = hiss + 11 * (hiss_state[1] - 2 * hiss_state[3] + hiss_state[5]);

        /* slowly wandering values for loudness and stereo placement */
        for (k = 3; k >= 0; k--)
            drift[k] = filter(filter_state[k], 1 - 1e-5, 1, 0, random_unit() - .5);

        /* every minute pick a new scale and melody pattern */
        if (i % (SAMPLE_RATE * 60) == 0) {
            scale = rand() % 4;
            phrase_length = 8 + rand() % 4;
            if (phrase_length > 10)
                phrase_length = 12;
            rhythm_step = 2 + rand() % 10;
            pattern_shift = 2 + rand() % 10;
            note_count = 0;
        }

        /* music plays from 10 to 50 seconds into each minute, 4 beats a second */
        music = 0;
        minute_pos = (i - SAMPLE_RATE * 10) % (SAMPLE_RATE * 60);
        beat = minute_pos / beat_length;
        if (minute_pos >= 0 && minute_pos % beat_length == 0 && minute_pos <= SAMPLE_RATE * 40) {
            double volume = fmin(1, (SAMPLE_RATE * 45 - minute_pos) * .1 / SAMPLE_RATE);
            int note, phrase_start, ending, step, gate;

            /* repeat the note from one phrase ago, or eight phrases ago */
            note = note_count < phrase_length ? -1 : note_history[note_count - phrase_length];
            phrase_start = beat % (phrase_length * 8) < phrase_length;
            if (phrase_start)
                note = note_count < phrase_length * 8 ? -1 : note_history[note_count - phrase_length * 8];

            /* sometimes pick a new note instead */
            if (note < 0 || (beat % (phrase_length * 4) < phrase_length && !phrase_start
                             ? random_unit() < .5 : random_unit() < .2))
                note = rand() % 5;
            /* sometimes move it up an octave */
            if (random_unit() < .1)
                note = note % 5 + 5;
            note_history[note_count++] = note;

            ending = (SAMPLE_RATE * 40 - minute_pos) < 8. * SAMPLE_RATE * random_unit();
            step = beat % phrase_length;
            gate = step * rhythm_step % NUM_SOURCES;

            /* rhythm: decide if a note is struck on this beat */
            if (gate < 3 + ((beat / phrase_length / 2 ^ pattern_shift) % 4)
                || ((beat + pattern_shift) / phrase_length % 2 && gate > 11)) {
                if (ending) {
                    note_energy[0] += volume * random_unit();
                } else {
                    int harmony = step % 3;

                    note_energy[note] += volume * random_unit();
                    /* also play an older note as harmony */
                    if (harmony) {
                        int old = note_count - 7 - 6 * harmony;

                        if (old >= 0)
                            note_energy[note_history[old]] += volume * random_unit();
                    }
                }
            }
            /* root note on the first beat of every phrase */
            if (step == 0)
                note_energy[0] += .4 * volume;
        }

        /* ten sine wave notes (two octaves of the scale) with soft envelopes */
        for (k = 9; k >= 0; k--) {
            int semitone = (scales[scale] >> k % 5 * 4) % 16;
            double pitch = pow(2, k / 5 + semitone / 12.) / 48 * i;

            note_energy[k] *= 1 - 4e-5;
            music += sin(pitch) * filter(filter_state[k + 8], .99, 1, 0, note_energy[k]);
        }

        /* fade in at the start and out at the end */
        fade = fmin(i / 1e5, fmin((num_samples - i) / 2e5, 1));

        /* left (side = 1) then right (side = -1) channel */
        for (k = 1; k >= 0; k--) {
            int side = 2 * k - 1, sample;

            x = wave[0] + (wave[1] + wave[2]) / 2 + (wave[1] - wave[2]) * side / 5;
            y = filter(filter_state[k + 4], 1 - 1e-4, 1, 0, fabs(x));   /* loudness */
            x += y * splash_noise / 1e4
               + filter(filter_state[k + 6], 1 - 5e-5, 1, 0, y) * hiss / 1e7
               + (rumble_noise * pow(1.5, drift[0] + side * drift[2] / 9)
                  + wash_noise * pow(1.5, drift[1] + side * drift[3] / 9)) / 32;
            x = x / 9 + music / 400;

            /* smoothstep fade and soft clipping */
            sample = fade * fade * (3 - 2 * fade) * x / sqrt(1 + x * x) * 32768;
            write_u16(out, sample);
        }
    }
    fclose(out);
    return 0;
}
