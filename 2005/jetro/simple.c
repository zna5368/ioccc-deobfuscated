#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

/*
 * Song data.  Each character is one step of the song (4096 samples, about
 * 0.19 seconds at 22050 Hz).  A '.' is silence; a letter is a note:
 *
 *   'A'..'Z'  are semitones  0 .. 25
 *   'a'..'y'  are semitones -24 .. 0  (so 'y' is the same pitch as 'A')
 *
 * Every voice also adds its own transposition to the note number.
 */

/* Lead melody: 512 steps.  Silent for 256 steps, then a 128 step tune twice */
static const char melody[] =
    "................................................................"
    "................................................................"
    "................................................................"
    "................................................................"
    "J..H..M.........J..H..E.........F..E..A.........F..E..J........."
    "H..E..O.........H..E..C.........H..F..x.........H..F..L........."
    "J..H..M.........J..H..E.........F..E..A.........F..E..J........."
    "H..E..O.........H..E..C.........H..F..x.........H..F..L.........";

/* Bass line: 512 steps.  128 silent, a 128 step riff twice, 128 silent */
static const char bass[] =
    "................................................................"
    "................................................................"
    "j.jv.j.vj.jv.j.vj.jv.j.vj.jv.j.vf.fr.f.rf.fr.f.rf.fr.f.rf.fr.f.r"
    "e.eq.e.qe.eq.e.qe.eq.e.qe.eq.e.qh.ht.h.th.ht.h.th.ht.h.th.ht.h.t"
    "j.jv.j.vj.jv.j.vj.jv.j.vj.jv.j.vf.fr.f.rf.fr.f.rf.fr.f.rf.fr.f.r"
    "e.eq.e.qe.eq.e.qe.eq.e.qe.eq.e.qh.ht.h.th.ht.h.th.ht.h.th.ht.h.t"
    "................................................................"
    "................................................................";

/* Three chord voices: 128 steps each, looped for the whole song */
static const char *chords[3] = {
    "v..v..q.v..v..q.v..v..q.v..v..q.r..r..m.r..r..m.r..r..m.r..r..m."
    "q..q..l.q..q..l.q..q..l.q..q..l.t..t..o.t..t..o.t..t..o.t..t..o.",

    "J.J.....J.J.....J.J.....J.J.....F.F.....F.F.....F.F.....F.F....."
    "E.E.....E.E.....E.E.....E.E.....H.H.....H.H.....H.H.....H.H.....",

    "M.M.....M.M.....M.M.....M.M.....J.J.....J.J.....J.J.....J.J....."
    "H.H.....H.H.....H.H.....H.H.....L.L.....L.L.....L.L.....L.L.....",
};

#define SAMPLES_PER_STEP_SHIFT 12 /* 4096 samples per song step */
#define ENVELOPE_LENGTH 8192      /* volume decays over two steps */
#define HISTORY_LENGTH 16384      /* samples remembered for the echo */

#define VOICE_MELODY 0
#define VOICE_CHORD 1 /* voices 1, 2 and 3 */
#define VOICE_BASS 4
#define VOICE_COUNT 5

static int chord_length, melody_length, bass_length;
static int sample_number = 0;

/*
 * Output history of every voice.  The buffer holds the last HISTORY_LENGTH
 * samples twice in a row, so "pos + HISTORY_LENGTH - k" is always the sample
 * from k samples ago without having to wrap the index around.
 */
static float history[VOICE_COUNT][2 * HISTORY_LENGTH];

/* Turn a song letter into a semitone number */
static int note_number(char letter, int transpose)
{
    return letter - (letter >= 'a' ? 'y' : 'A') + transpose;
}

/* Sine wave of the given note; speed scales the base pitch */
static float tone(int note, int sample, double speed)
{
    double step = 0.12 * pow(2, note / 12.);
    return (float)sin(step * sample * speed);
}

/*
 * Mix the new sample with the voice's own output from "delay" samples ago
 * (an echo), then average with the previous output (a soft low-pass filter).
 */
static float echo_filter(int voice, int pos, float in, float dry, int delay)
{
    float *h = history[voice];
    return (in * dry + h[HISTORY_LENGTH + pos - delay] * (1 - dry)) / 2.f +
           h[HISTORY_LENGTH + pos - 1] / 2.f;
}

static void remember(int voice, int pos, float value)
{
    history[voice][pos] = history[voice][pos + HISTORY_LENGTH] = value;
}

/* SDL audio callback: fill the buffer with 16 bit mono samples */
void fill_audio(void *userdata, Uint8 *stream, int length)
{
    (void)userdata;

    while (length > 0) {
        int pos = sample_number & (HISTORY_LENGTH - 1);
        int step = sample_number >> SAMPLES_PER_STEP_SHIFT;
        char melody_note = melody[step % melody_length];
        char bass_note = bass[step % bass_length];
        int voice, note;
        float mix = 0, sample = 0, detuned;
        /* fades from 1 to 0 over every two steps */
        float envelope = (ENVELOPE_LENGTH -
                          (sample_number & (ENVELOPE_LENGTH - 1))) /
                         8192e0f;

        /* Lead: two slightly detuned, clipped sines with a decay */
        if (melody_note != '.') {
            note = note_number(melody_note, 12);
            sample = tone(note, sample_number, 1);
            detuned = tone(note, sample_number, 1.01f);
            sample *= sample > 0.6 ? sample : 0;
            detuned *= detuned > 0.6 ? detuned : 0;
            sample += detuned;
            sample *= 0.5f * (envelope + 1);
        }
        sample = echo_filter(VOICE_MELODY, pos, sample, 0.2f, 12288);
        remember(VOICE_MELODY, pos, sample);
        mix += sample;

        /* Chords: three low sines, each a separate voice */
        for (voice = 0; voice < 3; ++voice) {
            char chord_note = chords[voice][step % chord_length];
            sample = 0;
            if (chord_note != '.') {
                note = note_number(chord_note, 12);
                sample = tone(note, sample_number, 0.25f);
                sample *= sample < 0.3 ? sample : 0;
            }
            sample = echo_filter(VOICE_CHORD + voice, pos, sample, 0.2f, 12288);
            remember(VOICE_CHORD + voice, pos, sample);
            mix += sample;
        }

        /* Bass: a very low sine shaped into a buzzy pulse */
        sample = 0;
        if (bass_note != '.') {
            note = note_number(bass_note, 0);
            sample = tone(note, sample_number, 1 / 8.f);
            sample *= sample; /* raise to the 16th power */
            sample *= sample;
            sample *= sample;
            sample *= sample;
            sample = (float)fmod(sample * 20, 4) * 4;
        }
        sample = echo_filter(VOICE_BASS, pos, sample, 0.25f, 6144);
        if (sample < -0.75f)
            sample = -0.75f;
        if (sample > 0.75f)
            sample = 0.75f;
        sample *= envelope;
        remember(VOICE_BASS, pos, sample);
        mix += sample;

        mix *= 0.6f;
        *(Sint16 *)stream = (Sint16)(mix * 32767);

        stream += 2;
        length -= 2;
        ++sample_number;
    }
}

int main(void)
{
    SDL_AudioSpec wanted, obtained;

    chord_length = strlen(chords[0]);
    melody_length = strlen(melody);
    bass_length = strlen(bass);

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        exit(1);
    atexit(SDL_Quit);

    wanted.freq = 22050;
    wanted.format = AUDIO_S16;
    wanted.channels = 1;
    wanted.samples = 4096;
    wanted.callback = fill_audio;
    wanted.userdata = 0;
    if (SDL_OpenAudio(&wanted, &obtained) < 0)
        exit(2);

    SDL_PauseAudio(0);
    /* play until enter is pressed */
    getchar();
    SDL_CloseAudio();
    return 0;
}
