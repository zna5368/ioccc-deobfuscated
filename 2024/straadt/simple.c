#include  <math.h>
#include<stdlib.h>
#include <stdio.h>

/*
 * Song synthesizer: renders a stereo 16-bit, 262144 Hz song into prog.wav.
 *
 * Time is measured in "steps" of STEP_LEN samples.  step(n) counts how many
 * blocks of (STEP_LEN << n) samples have passed, and phase(n) is the position
 * (0..1) inside the current block, which is used as an envelope timer.
 *
 * NOTE: every floating point expression keeps the exact operand order of the
 * original so that the output file is bit-for-bit identical.
 */

#define STEP_LEN     17000              /* samples per step                 */
#define SAMPLE_RATE  (1 << 18)          /* 262144 Hz                        */
#define BYTE_RATE    (4 << 18)          /* 2 channels * 2 bytes             */

/* mixing buses */
enum { LEFT, RIGHT, REVERB_SEND, ECHO_SEND };

static const double freq_scale = 1.5e-3; /* oscillator phase per sample      */

/* memory for all the filter/delay line state, handed out in order each sample */
static double filter_memory[1 << 20], *filter_alloc;
static double bus[4];

static int sample;          /* current sample index                         */
static int total_samples;
static int progress_count;

/* bass: one note (semitone, 0..15) per nibble, one nibble per 8 steps */
static const unsigned bass_notes[] = { 0x0ce97522, 0xc02972e5 };
/* bass rhythm: one bit per 2 steps (on/off) */
static const unsigned bass_rhythm[] = { 0xeceddaca, 0xfeedfaad };

/* note tables in semitones */
static const int arpeggio[] = {          /* 2 sets of 3 notes              */
  -2, 0,  7,
   0, 3, 10,
};
static const int melody[] = {            /* 2 phrases of 8 notes           */
  -5, 0, 0,  2, 3, 0, 2,  2,
  -2, 0, 7, 12, 3, 7, 5, 10,
};
static const int chords[] = {            /* 4 chords of 3 notes            */
   0, 3, -5,
  -2, 7,  3,
  -2, 2, -5,
 -10,-2,  5,
};

static const char progress_msg[] = "\njust a moment..";

/* the whole wav file is built in memory, then written at the end */
static char wav_buffer[1 << 29], *wav_ptr;

/* ------------------------------------------------------------------ timing */

static int step(int level)
{
  return sample / (STEP_LEN << level);
}

static double phase_offset(int level, int offset_steps)
{
  return (double)((sample + STEP_LEN * offset_steps) % (STEP_LEN << level))
         / (STEP_LEN << level);
}

static double phase(int level)
{
  return phase_offset(level, 0);
}

/* square wave with a 25% duty cycle and a period of `period` samples */
static double pulse(int period)
{
  return sample % period < period / 4 ? 1 : -1;
}

/* ------------------------------------------------------------- wav output */

static void put16(short value)
{
  *wav_ptr++ = (char)value;
  *wav_ptr++ = value >> 8;
}

static void put32(int value)
{
  put16((short)value);
  put16((short)(value >> 16));
}

static void put_text(const char *text)
{
  while (*text)
    *wav_ptr++ = *text++;
}

static void write_wav_header(void)
{
  put_text("RIFF");
  put32(36 + (total_samples << 2));
  put_text("WAVEfmt ");
  put32(16);              /* fmt chunk size  */
  put16(1);               /* PCM             */
  put16(2);               /* stereo          */
  put32(SAMPLE_RATE);
  put32(BYTE_RATE);
  put16(4);               /* block align     */
  put16(16);              /* bits per sample */
  put_text("data");
  put32(total_samples << 2);
}

/* ---------------------------------------------------------------- filters */

static double *alloc_state(int count)
{
  filter_alloc += count;
  return filter_alloc - count;
}

/* add a voice to the mix: volume, stereo pan (-1..1) and effect sends */
static void mix(double x, double volume, double pan, double reverb, double echo)
{
  bus[REVERB_SEND] += reverb * x;
  bus[LEFT]        += x * volume * fmin(1, 1 - pan);
  bus[ECHO_SEND]   += echo * x;
  bus[RIGHT]       += fmin(pan + 1, 1) * x * volume;
}

/* biquad filter (transposed direct form II) */
static double biquad(double x, double b0, double b1, double b2,
                     double a1, double a2)
{
  double *z = alloc_state(2);
  double y = b0 * x + z[0];
  z[0] = x * b1 - a1 * y + z[1];
  z[1] = b2 * x - y * a2;
  return y;
}

/*
 * Freeverb style comb filter with a low-pass in the feedback loop.
 * State layout: delay line[length], filter store, write position.
 */
static double comb(double x, int length, double feedback, double damp)
{
  double *line = alloc_state(length + 2), output;
  int pos = (int)line[length + 1];

  output = line[pos];
  line[length] = output * (1 - damp) + line[length] * damp;
  line[pos] = x + line[length] * feedback;

  if (++pos >= length)
    pos = 0;
  line[length + 1] = pos;
  return output;
}

/* Freeverb style all-pass filter */
static double allpass(double x, int length, double feedback)
{
  double *line = alloc_state(length + 2), output;
  int pos = (int)line[length + 1];

  double delayed = line[pos];
  output = delayed - x;
  line[pos] = feedback * delayed + x;

  if (++pos >= length)
    pos = 0;
  line[length + 1] = pos;
  return output;
}

/* ------------------------------------------------------------ instruments */

/* kick drum: a sine whose pitch drops quickly, overdriven with tanh */
static void kick_drum(void)
{
  double t, x;
  int roll = ((step(3) + 32) % 64) - 61;   /* 0..2 during a drum roll */

  t = tanh(phase(roll < 0 ? 3 : roll));
  x = tanh(50 * sin(t * 141 + t * (15e3 / (4 + t * 330))) * pow(1 - tanh(t), 9));
  if (step(10))                            /* no kick in the outro */
    x = 0;
  mix(x, .5, 0, .07, .1);
}

/* hi-hat tone: 8 square waves through a high-pass filter (TR-808 style) */
static double hihat_tone(void)
{
  double x = 0;
  int j;

  for (j = 0; j < 8; ++j)
    x += pulse(211 + (j * 113 + step(2) * 91) % 1291);
  x = biquad(x, .973, -1.946, .973, -1.942, .951);
  return tanh(x * 2);
}

/* hi-hat envelope timer: the rhythm changes with each song section */
static double hihat_timer(void)
{
  int fast = step(4) % 8 > 6;

  switch (step(8)) {
  case 0:
    return tanh(phase(step(4) % 4));
  case 2:
    return tanh(phase(2 - fast));
  default:
    return tanh(phase(1 - fast));
  }
}

static void hihat(double x, double t)
{
  x *= pow(1 - t, 4) * pow(phase(3), 2);
  x = tanh(x * 1.4);
  if (!(step(7) & 7) || step(10))
    x = 0;
  mix(x, .8, sin(sample * 1e-5), .16, .4);
}

/* snare drum: switches between noise and a falling square wave (SID style) */
static void snare_drum(void)
{
  double t, x;

  if ((step(2) & 63) > 61)                 /* fast roll */
    t = 1 - tanh(phase(1)) / 6;
  else                                     /* on the off-beat */
    t = 1 - tanh(phase_offset(4, 8));

  if (sample & (1 << 12))
    x = (double)(rand()) / RAND_MAX - .5;
  else
    x = .1 * pulse(1400 - (int)(t * 301));

  x = tanh(30 * x * pow(t, 40));
  if (!(step(7) & 3) || step(10))
    x = 0;
  mix(x, .8, .3, 1, .3);
}

/* FM bass; returns whether the bass note is on (used by the chords) */
static int bass(void)
{
  double phase_acc, t, x;
  int part = step(6) % 4;
  int note = (bass_notes[part > 2] >> ((step(3) & 7) << 2)) & 15;
  int octave_mod = step(8) % 2;
  int gate = ((bass_rhythm[part % 2]) >> (step(1) & 31)) & 1;

  phase_acc = freq_scale * sample * pow(2, (double)note / 12);

  if (gate)
    x = sin(phase_acc + 1.5 * sin(sample * 7e-5)
            + sin(sample * 3e-7) * (4 + 3 * sin(sample * 1.7e-7))
              * sin(phase_acc * (1 + octave_mod)));
  else
    x = 0;

  if (step(8))                             /* extra clicky pulses */
    x += ((sin(phase_acc * .501) > .95) - (sin(phase_acc * .498) > .8))
         * 15 * pow(phase(3), 2);

  t = 1 - tanh(phase(1));
  x = tanh(25 * x * pow(t, 3));
  if (step(10))
    x = 0;
  mix(x, .4, 0, .12, .02);
  return gate;
}

/* three detuned voices playing arpeggios, chords or the lead melody */
static void chord_voices(int bass_gate)
{
  double t, x;
  int voice, note, lead;

  for (voice = 0; voice < 3; ++voice) {
    note = step(1) % 32 + 3;
    note -= note / 4 * 3;
    lead = step(6) & 1;

    if (lead)
      note = 24 + melody[step(7) % 2 * 8 + step(3) % 8];
    else if (step(9))
      note = 24 + chords[(note + voice) % 3 + step(4) % 4 * 3];
    else
      note = arpeggio[note % 3 + step(7) % 2 * 3] + (note / 3) * 12;

    /* oscillator phase with vibrato */
    x = freq_scale * sample * pow(2, ((double)(14 + note) + (voice - 1) * .4) / 12)
        + sin(voice + sample * (lead ? 1e-4 : 3e-4)) * (lead ? 4 : 1);

    /* sine plus sawtooth, then pulse-width modulation */
    t = sin(x) + .3 * (fmod(x, 6.3) - 3);
    x = t + .3 * (t > .9 * sin(sample * 6.3e-7) ? 1 : -1);

    t = 1 - tanh(phase(lead ? 2 : bass_gate ? 2 : 1));
    x = tanh(x * pow(t, 9) * (step(10) ? pow(1 - phase(8), 3) : 1));

    if (step(6) % 2 * !step(9) || !step(8))
      x = 0;
    mix(x, 1, sin(2 * voice + sample * 3e-6), .3, .3);
  }
}

/* echo effect built from a long comb and a long all-pass */
static void echo(void)
{
  mix(comb(bus[ECHO_SEND], (int)(STEP_LEN * 6.1), .2, .2), 1, -.6, .1, 0);
  mix(allpass(bus[ECHO_SEND], (int)(STEP_LEN * 3.8), .2), .7, .6, .1, 0);
}

/*
 * Schroeder/Freeverb reverb, one per channel: 8 parallel combs followed by
 * 4 all-passes.  The all-pass lengths change every 256 steps, which shifts
 * where the later filters keep their state in filter_memory (the deliberate
 * "memory corruption" effect).
 */
static void reverb(void)
{
  double x;
  int channel, j;

  for (channel = 0; channel < 2; ++channel) {
    x = 0;
    for (j = 0; j < 8; ++j)
      x += comb(bus[REVERB_SEND], 4001 + (j * 2131 + channel * 1555) % 3491, .8, .5);
    for (j = 0; j < 4; ++j)
      x = allpass(x, 800 + ((j + step(8)) * 1337 + channel * 1441) % 1303, .4);
    x = tanh(x / 4);
    mix(x, .3, channel ? -1 : 1, 0, 0);
  }
}

/* ------------------------------------------------------------------- main */

int main(void)
{
  double hat_tone, hat_timer;
  int bass_gate, channel;
  FILE *file;

  wav_ptr = wav_buffer;
  total_samples = (STEP_LEN * 5) << 8;
  write_wav_header();

  for (;;) {
    filter_alloc = filter_memory;
    for (channel = 0; channel < 4; ++channel)
      bus[channel] = 0;

    kick_drum();
    hat_tone = hihat_tone();
    hat_timer = hihat_timer();

    /* the sample counter advances here: everything below sees sample + 1 */
    if (++sample > total_samples)
      break;

    hihat(hat_tone, hat_timer);
    snare_drum();
    bass_gate = bass();
    chord_voices(bass_gate);
    echo();
    reverb();

    for (channel = 0; channel < 2; ++channel)
      put16((short)(fmin(1, fmax(-1, bus[channel])) * 32e3));

    if (!(sample % (STEP_LEN * 8))) {
      putchar(progress_msg[++progress_count & 15]);
      fflush(stdout);
    }
  }

  file = fopen("prog.wav", "wb");
  fwrite(wav_buffer, 1, (unsigned long)(wav_ptr - wav_buffer), file);
  fclose(file);
  puts("now enjoy prog.wav");
  return 0;
}
