#include <stdio.h>

/*
 * MOD player: converts an Amiga MOD file to raw 16-bit 44.1kHz stereo
 * little-endian audio on stdout.
 */

#define ROWS_PER_PATTERN 64
#define MAX_PATTERN_CELLS (1 << 14)
#define NUM_NOTES 60

/*
 * Amiga period for each note.  The original stored this table as a string
 * of ' ', '{', '}' and ';' (worth 0, 1, 2 and 3) where ten characters are
 * summed as value << position.  Only ten bits are available, so the first
 * nine periods (1712, 1616, ... 1077) overflow and lose 1024.
 */
static const int note_period_table[NUM_NOTES + 1] = {
    0,   688, 592, 501, 416, 333, 257, 185, 117, 53,  1017, 961, 907,
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,  453, 428,
    404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,  214, 202,
    190, 180, 170, 160, 151, 143, 135, 127, 120, 113, 107,  101, 95,
    90,  85,  80,  76,  71,  67,  64,  60,  57};

/* Fields of a sample header (lengths are in 16-bit words) */
enum { SMP_LENGTH, SMP_FINETUNE, SMP_VOLUME, SMP_LOOP_START, SMP_LOOP_LENGTH };

/*
 * Per channel effect state.  Indexes 1..15 remember the last parameter
 * given to each effect, 16..31 remember the last parameter of each
 * extended (Exy) effect.  Index 8 doubles as the current period because
 * effect 8 is not used.
 */
enum {
    CH_VOLUME = 0,
    CH_PERIOD = 8,
    CH_EXT_PARAM = 16,
    CH_NOTE = 32,
    CH_SAMPLE = 34,
    CH_FINETUNE = 35,
    CH_LOOP_ROW = 37,
    CH_LOOP_COUNT = 38,
    CH_RETRIG_COUNT = 39,
    CH_STATE_SIZE = 50
};

/* Effect numbers */
enum {
    FX_ARPEGGIO = 0,
    FX_SLIDE_UP = 1,
    FX_SLIDE_DOWN = 2,
    FX_TONE_PORTA = 3,
    FX_TONE_PORTA_VOL_SLIDE = 5,
    FX_VIBRATO_VOL_SLIDE = 6,
    FX_SAMPLE_OFFSET = 9,
    FX_VOL_SLIDE = 10,
    FX_POSITION_JUMP = 11,
    FX_SET_VOLUME = 12,
    FX_PATTERN_BREAK = 13,
    FX_EXTENDED = 14,
    FX_SET_SPEED = 15
};

/* Extended effect numbers (Exy, x selects the effect) */
enum {
    EX_FINE_SLIDE_UP = 1,
    EX_FINE_SLIDE_DOWN = 2,
    EX_SET_FINETUNE = 5,
    EX_PATTERN_LOOP = 6,
    EX_RETRIGGER = 9,
    EX_FINE_VOL_UP = 10,
    EX_FINE_VOL_DOWN = 11,
    EX_NOTE_CUT = 12,
    EX_NOTE_DELAY = 13,
    EX_PATTERN_DELAY = 14
};

struct cell {
    int sample;
    int note; /* index into note_period_table, 0 = none */
    int effect;
    int param;
};

/* Mixer state for a playing channel */
struct voice {
    int position; /* byte offset into the sample */
    int phase;    /* fractional step accumulator */
    int unused;
    int end;         /* byte offset where the sample (or loop) ends */
    int loop_length; /* bytes to jump back at the end */
    int step;        /* phase needed to move one byte (999 * period) */
    int volume;      /* 0..64 */
};

char module[1 << 25];

char *channel_data[8];  /* sample being played on each channel */
char *sample_data[32];  /* start of each sample's data in the module */
int sample_info[32][5]; /* sample headers */
int channel_state[8][CH_STATE_SIZE];
struct voice voices[8];
struct cell cells[MAX_PATTERN_CELLS];

char *order_table;
int song_length;
int num_channels = 4;
int speed;
int samples_per_tick = 882; /* 44100 Hz / 50 ticks per second */
int tick, row, order, pattern_delay, done;

static int min(int a, int b) { return a < b ? a : b; }

static int max(int a, int b) { return a > b ? a : b; }

static int clamp(int x, int lo, int hi) { return max(lo, min(hi, x)); }

static int big_endian_word(unsigned char *p) { return (p[0] << 8) | p[1]; }

/* Period of the channel's current note shifted by some semitones */
static int note_period(int *chan, int semitones) {
    /* (finetune - 24) / 256 is always 0 since finetune is in -16..7 */
    if (!chan[CH_NOTE])
        return 0;
    return note_period_table[clamp(
        chan[CH_NOTE] + semitones + (chan[CH_FINETUNE] - 24) / 256, 0, 59)];
}

/* Start playing the channel's sample from the beginning (or an offset) */
static void trigger_note(int ch, struct cell *cell) {
    int *chan = channel_state[ch];
    struct voice *v = &voices[ch];
    int *smp = sample_info[chan[CH_SAMPLE]];
    int loop_length, offset;

    /* tone portamento slides to the note instead of restarting it */
    if (cell->effect == FX_TONE_PORTA ||
        cell->effect == FX_TONE_PORTA_VOL_SLIDE)
        return;

    if (chan[CH_NOTE])
        chan[CH_PERIOD] = note_period(chan, 0);
    loop_length = smp[SMP_LOOP_LENGTH];
    channel_data[ch] = sample_data[chan[CH_SAMPLE]];
    if (loop_length > 1) {
        v->end = 2 * (smp[SMP_LOOP_START] + loop_length);
        v->loop_length = 2 * loop_length;
    } else {
        v->end = 2 * smp[SMP_LENGTH];
        v->loop_length = 1;
    }
    offset = cell->effect == FX_SAMPLE_OFFSET ? chan[FX_SAMPLE_OFFSET] << 8 : 0;
    v->position = min(offset, v->end - 1);
}

/* Effects applied on the first tick of a row */
static void first_tick(int ch, struct cell *cell) {
    int *chan = channel_state[ch];
    int *smp;
    int x = cell->param >> 4, y = cell->param & 15;

    if (cell->sample) {
        chan[CH_SAMPLE] = cell->sample;
        smp = sample_info[chan[CH_SAMPLE]];
        chan[CH_FINETUNE] = smp[SMP_FINETUNE];
        chan[CH_VOLUME] = smp[SMP_VOLUME];
    }
    if (cell->param)
        chan[cell->effect] = cell->param;
    if (cell->note &&
        (cell->effect != FX_EXTENDED || x != EX_NOTE_DELAY)) {
        chan[CH_NOTE] = cell->note;
        trigger_note(ch, cell);
    }
    if (cell->effect == FX_SET_VOLUME)
        chan[CH_VOLUME] = clamp(cell->param, 0, 64);
    if (cell->effect == FX_SET_SPEED && cell->param) {
        if (cell->param < 33)
            speed = cell->param;
        else
            samples_per_tick = 110250 / cell->param; /* tempo in BPM */
    }
    if (cell->effect == FX_EXTENDED) {
        if (y)
            chan[CH_EXT_PARAM + x] = y;
        if (x == EX_FINE_SLIDE_UP)
            chan[CH_PERIOD] = max(113, chan[CH_PERIOD] -
                                           chan[CH_EXT_PARAM + EX_FINE_SLIDE_UP]);
        else if (x == EX_FINE_SLIDE_DOWN)
            chan[CH_PERIOD] = min(856, chan[CH_PERIOD] +
                                           chan[CH_EXT_PARAM + EX_FINE_SLIDE_DOWN]);
        if (x == EX_SET_FINETUNE)
            chan[CH_FINETUNE] = (y & 7) - 2 * (y & 8);
        if (x == EX_RETRIGGER) {
            if (chan[CH_EXT_PARAM + EX_RETRIGGER] && !cell->note)
                trigger_note(ch, cell);
            chan[CH_RETRIG_COUNT] = 0;
        }
        if (x == EX_FINE_VOL_UP)
            chan[CH_VOLUME] = min(chan[CH_VOLUME] +
                                      chan[CH_EXT_PARAM + EX_FINE_VOL_UP], 64);
        else if (x == EX_FINE_VOL_DOWN)
            chan[CH_VOLUME] = max(chan[CH_VOLUME] -
                                      chan[CH_EXT_PARAM + EX_FINE_VOL_DOWN], 0);
        if (x == EX_PATTERN_DELAY)
            pattern_delay = chan[CH_EXT_PARAM + EX_PATTERN_DELAY];
    }
}

/* Effects applied on the other ticks of a row */
static void other_tick(int ch, struct cell *cell) {
    int *chan = channel_state[ch];
    int effect = cell->effect;
    int remembered = chan[effect];
    int x = cell->param >> 4, y = cell->param & 15;
    int last_tick = tick == speed - 1;
    int target;

    if (effect == FX_TONE_PORTA_VOL_SLIDE || effect == FX_VIBRATO_VOL_SLIDE ||
        effect == FX_VOL_SLIDE) {
        if (remembered >> 4)
            chan[CH_VOLUME] = min(chan[CH_VOLUME] + (remembered >> 4), 64);
        else
            chan[CH_VOLUME] = max(chan[CH_VOLUME] - (remembered & 15), 0);
    }
    if (effect == FX_ARPEGGIO && cell->param && chan[CH_NOTE])
        chan[CH_PERIOD] =
            note_period(chan, tick % 3 == 1 ? x : tick % 3 == 2 ? y : 0);
    if (effect == FX_SLIDE_UP)
        chan[CH_PERIOD] = max(113, chan[CH_PERIOD] - chan[FX_SLIDE_UP]);
    else if (effect == FX_SLIDE_DOWN)
        chan[CH_PERIOD] = min(856, chan[CH_PERIOD] + chan[FX_SLIDE_DOWN]);
    if (effect == FX_TONE_PORTA_VOL_SLIDE || effect == FX_TONE_PORTA) {
        target = note_period(chan, 0);
        /* the original computes the upward slide too but never stores it */
        if (chan[CH_PERIOD] > target)
            chan[CH_PERIOD] = max(chan[CH_PERIOD] - chan[FX_TONE_PORTA], target);
    }
    /* position jump is not supported, it ends the song instead */
    if (effect == FX_POSITION_JUMP && last_tick)
        done = 1;
    if (effect == FX_PATTERN_BREAK && last_tick) {
        order++;
        row = 10 * x + y - 1; /* parameter is decimal, row++ comes later */
    }
    if (effect == FX_EXTENDED) {
        if (x == EX_PATTERN_LOOP) {
            if (!y)
                chan[CH_LOOP_ROW] = row;
            else if (last_tick) {
                if (chan[CH_LOOP_COUNT] < y) {
                    row = chan[CH_LOOP_ROW] - 1;
                    chan[CH_LOOP_COUNT]++;
                } else
                    chan[CH_LOOP_COUNT] = 0;
            }
        }
        if (x == EX_RETRIGGER && ++chan[CH_RETRIG_COUNT] ==
                                     chan[CH_EXT_PARAM + EX_RETRIGGER]) {
            chan[CH_RETRIG_COUNT] = 0;
            trigger_note(ch, cell);
        }
        if (x == EX_NOTE_CUT && tick == chan[CH_EXT_PARAM + EX_NOTE_CUT])
            chan[CH_VOLUME] = 0;
        /* the original compares with sample 5's loop length, not ED's param */
        if (x == EX_NOTE_DELAY && tick == sample_info[5][SMP_LOOP_LENGTH])
            trigger_note(ch, cell);
    }
}

/*
 * Advance the sample position by one sub-step (six per output sample).
 */
static void advance(struct voice *v) {
    if ((v->phase += 13200) >= v->step) {
        v->phase -= v->step;
        if (++v->position >= v->end)
            v->position -= v->loop_length;
    }
}

int main(int argc, char **argv) {
    static const int filter[6] = {1, 4, 12, 12, 4, 1};
    unsigned char *q;
    char *sig = module + 1080;
    int num_samples, num_patterns = 0;
    int i, k, n, period, best, diff;
    int left, right, sum, pan;
    struct cell *cell;
    size_t bytes_read;

    (void)argc;
    bytes_read = fread(module, 1, 1 << 24, fopen(argv[1], "rb"));
    (void)bytes_read;

    /* "FLTn" or "nCHN" give the channel count, "M.K." means 31 samples */
    if (sig[0] == 'F' && sig[2] == 'T') {
        num_channels = sig[3] - '0';
        num_samples = 32;
    } else if (sig[1] == 'C' && sig[3] == 'N') {
        num_channels = sig[0] - '0';
        num_samples = 32;
    } else if (sig[0] == 'M' && sig[2] == 'K')
        num_samples = 32;
    else
        num_samples = 16;

    /* sample headers follow the 20 byte song title */
    q = (unsigned char *)module + 20;
    for (i = 1; i < num_samples; i++) {
        q += 22; /* sample name */
        sample_info[i][SMP_LENGTH] = big_endian_word(q);
        sample_info[i][SMP_FINETUNE] = (q[2] & 7) - 2 * (q[2] & 8);
        sample_info[i][SMP_VOLUME] = q[3];
        sample_info[i][SMP_LOOP_START] = big_endian_word(q + 4);
        sample_info[i][SMP_LOOP_LENGTH] = big_endian_word(q + 6);
        q += 8;
    }

    song_length = q[0];
    order_table = (char *)(q += 2);
    q += num_samples > 15 ? 132 : 128; /* orders and the signature */
    for (k = 0; k < 128; k++)
        num_patterns = max(order_table[k] + 1, min(128, num_patterns));

    /* decode the pattern cells and convert periods to notes */
    cell = cells;
    for (n = 0; n < num_channels * ROWS_PER_PATTERN * num_patterns; n++) {
        cell->sample = (q[0] & 0xF0) | (q[2] >> 4);
        cell->note = 0;
        cell->effect = q[2] & 15;
        cell->param = q[3];
        period = ((q[0] & 15) << 8) | q[1];
        best = 9 << 19;
        if (period) {
            for (i = 1; i <= NUM_NOTES; i++) {
                diff = period - note_period_table[i];
                if (diff * diff < best) {
                    cell->note = i;
                    best = diff * diff;
                }
            }
        }
        cell++;
        q += 4;
    }

    /* the sample data follows the patterns */
    for (i = 1; i < num_samples; i++) {
        sample_data[i] = (char *)q;
        q += 2 * sample_info[i][SMP_LENGTH];
    }

    speed = 6;
    while (!done) {
        for (i = 0; i < num_channels; i++) {
            cell = &cells[num_channels * (ROWS_PER_PATTERN * order_table[order] +
                                          row) +
                          i];
            if (tick)
                other_tick(i, cell);
            else
                first_tick(i, cell);
            voices[i].volume = clamp(channel_state[i][CH_VOLUME], 0, 64);
            voices[i].step = 999 * channel_state[i][CH_PERIOD];
        }

        if (++tick >= speed + speed * pattern_delay) {
            tick = pattern_delay = 0;
            if (++row >= ROWS_PER_PATTERN) {
                row = 0;
                if (++order >= song_length)
                    done = 1;
            }
        }

        for (n = samples_per_tick; n--;) {
            left = right = 0;
            for (i = 0; i < num_channels; i++) {
                if (!channel_data[i])
                    continue;
                /*
                 * The original intends to read the sample after each of the
                 * six sub-steps and weight them 1, 4, 12, 12, 4, 1 (total
                 * 34), but all six terms are in one unsequenced sum.  GCC
                 * performs every step first and then reads the same
                 * sample six times, which is reproduced here.
                 */
                for (k = 0; k < 6; k++)
                    advance(&voices[i]);
                sum = 0;
                for (k = 0; k < 6; k++)
                    sum += filter[k] * voices[i].volume *
                           channel_data[i][voices[i].position] / 34;
                /* Amiga panning: channels 0 and 3 left, 1 and 2 right */
                pan = 1 << ((i + 1) & 2);
                left += sum / pan;
                right += sum / (pan ^ 5);
            }
            putchar(left);
            putchar(left >> 8);
            putchar(right);
            putchar(right >> 8);
        }
    }
    return 0;
}
