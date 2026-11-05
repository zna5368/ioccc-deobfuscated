/*
 * Sound synthesizer for a small subset of ABC music notation.
 *
 * The score is read from standard input as a list of notes, each one a letter
 * optionally followed by a length ("C2 E2 G4"). Standard output receives raw
 * unsigned 8-bit audio at 8000 samples per second.
 */

/* An eighth note (length 1) lasts 2000 samples, which is 0.25 seconds. */
#define SAMPLES_PER_LENGTH 2e3

/* The twelfth root of two, the frequency ratio between adjacent semitones. */
#define SEMITONE_RATIO (89 / 84.)

/* Tick at which tuning the pitch ends and generating samples begins. */
#define FIRST_SAMPLE_TICK 24

/* Letter codes at or below this value, such as the rest 'z', are silent. */
#define MAX_SILENT_CODE 9

/* Number of semitones in an octave, and the period of the letter codes. */
#define OCTAVE_SEMITONES 12
#define LETTER_CODE_PERIOD 13

/*
 * Left in the note variable before each read. When scanf() hits the end of the
 * input it leaves the variable alone, and this value gives a start tick of 0.
 */
#define END_OF_SCORE 2

/*
 * Maps a letter onto a code that falls by about 9/5 for each letter of the
 * alphabet. The 9/5 slope spreads the 7 letters over the 12 semitones of an
 * octave, and it leaves the double gap or single gap between neighbouring
 * notes that the major scale needs.
 */
static int letter_code(char note)
{
	return 55 - note % 32 * 9 / 5;
}

/*
 * Tick the note starts counting from. Starting further below
 * FIRST_SAMPLE_TICK means more semitones are applied and so a higher pitch.
 * Lowercase letters have a larger note / 32 and sit one octave above uppercase.
 */
static int start_tick(char note)
{
	return letter_code(note) % LETTER_CODE_PERIOD
		- note / 32 * OCTAVE_SEMITONES;
}

/*
 * Plays one note. The tick counter first raises the pitch by one semitone per
 * tick, then carries on counting up as the time axis of a saw wave. Assigning
 * pitch * tick to a char wraps it around 256, so the output climbs and drops
 * back repeatedly, and a larger pitch makes it do so more often.
 *
 * The note ends once tick has passed last_tick. A very short note can end
 * during tuning and produce no samples. The tests are written as tick > limit
 * because a length of "nan" must never end the note.
 */
static void play_note(int tick, float pitch, float length)
{
	double last_tick = SAMPLES_PER_LENGTH * length;
	char sample;

	while (++tick < FIRST_SAMPLE_TICK) {
		pitch *= SEMITONE_RATIO;
		if (tick > last_tick)
			return;
	}

	for (;; tick++) {
		sample = pitch * tick;
		putchar(sample);
		if (tick > last_tick)
			return;
	}
}

int main(void)
{
	char note = 0;
	float length;
	float pitch;
	int tick;

	for (;;) {
		/* A missing length keeps the default of one eighth note. */
		length = 1;
		scanf(" %c%f", &note, &length);

		/* Silent notes keep a pitch of 0, so the wave stays flat. */
		pitch = letter_code(note) > MAX_SILENT_CODE;

		tick = start_tick(note);
		if (tick == 0)
			break;

		play_note(tick, pitch, length);
		note = END_OF_SCORE;
	}

	return 0;
}
