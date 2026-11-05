#include <stdio.h>
#include <string.h>

/* The output is raw 44.1kHz stereo audio made of 16 bit little endian samples.
   Morse timing is counted in slots of SLOT_BYTES bytes of audio. */
#define SLOT_BYTES 13000
#define TONE_PERIOD_BYTES 176           /* one square wave cycle, about 1 kHz */
#define TONE_LOW_LEVEL 85
#define TONE_HIGH_LEVEL 170

#define DOT_SLOTS 1
#define DASH_SLOTS 3
#define SYMBOL_GAP_SLOTS 1              /* silence after every dot or dash */
#define LETTER_GAP_SLOTS 2              /* extra silence after every letter */
#define WORD_GAP_SLOTS 6                /* silence for a space in the text */

/* The decoder measures the loudness of the input in blocks of this many bytes. */
#define BLOCK_BYTES 1500
#define MIN_LOUDNESS_RATIO 5            /* change that counts as a tone edge */

#define HISTORY_SIZE 32                 /* remembered tones and silences */
#define MIN_EVENTS 6                    /* needed before anything is decoded */
#define MAX_MORSE_SYMBOLS 6

/* Morse code as a binary tree stored level by level. The children of the node
   at index k are k * 2 + 1 for a dot and k * 2 + 2 for a dash, so a letter of
   n symbols, read as the binary number "bits" with dash = 1, is at index
   (1 << n) - 1 + bits. A ':' is an unused code, apart from the last one which
   is the real ':'. The array is larger than the text since a code of six dashes
   lands on index 126, where it finds a zero byte.

   Element 0 is the root of the tree. It is a space when decoding. When encoding
   it holds the character being encoded, which means searching this array finds
   the character at index 0 if it has no Morse code. */
char morse_tree[128] =
    " "
    "ET"
    "IANM"
    "SURWDKGO"
    "HVF:L:PJBXCYZQ::"
    "54:3:::2&:+::::16=/:::(:7:::8:90"
    "::::::::::::?_::::\"::.::::@:::'::-::::::::;!:):::::,:::::";

/* Lengths in blocks of the alternating silences and tones found in the input,
   as a ring buffer. Even numbered events are silences, odd ones are tones. */
int event_lengths[HISTORY_SIZE];

/* Length of each event added to the length of the event before it. A dot and
   the gap after it make up two Morse units, and this is what the unit length is
   estimated from.

   There is one spare element which is always zero, because the last comparison
   in estimate_unit_length reads one element past the ring buffer. */
int event_pair_lengths[HISTORY_SIZE + 1];

int event_count;                        /* events completed so far */
int next_event;                         /* first event not yet decoded */
int symbol_count;                       /* dots and dashes in the current letter */
int symbol_bits;                        /* those dots and dashes, dash = 1 */

/* Write audio lasting total_slots, where tone_slots of it are a tone and the
   rest is silence. Only the high byte of each sample is used. */
void play(int total_slots, int tone_slots)
{
    int i;

    for (i = 0; i < total_slots * SLOT_BYTES; i++) {
        int is_tone = i / SLOT_BYTES < tone_slots;
        int is_high_byte = i % 2;
        int level = 0;

        if (is_tone && is_high_byte) {
            level = i % TONE_PERIOD_BYTES < TONE_PERIOD_BYTES / 2
                ? TONE_LOW_LEVEL : TONE_HIGH_LEVEL;
        }
        putchar(level);
    }
}

/* Change a lower case letter to upper case. This looks for bits 5 and 6 both
   being set, so the symbols {|}~ and some bytes above 127 are changed too. The
   result has no Morse code and no longer matches morse_tree[0] either, so the
   search in morse_tree_index fails. */
int fold_case(int character)
{
    if ((character & 32) && (character / 2 & 32))
        return character & ~32;
    return character;
}

/* Find a character in the tree. A failed search has the result NULL and this
   gives an index made from the address of morse_tree, which is a bug inherited
   from the original program: the Morse code for those characters is random. */
int morse_tree_index(int character)
{
    return strrchr(morse_tree, fold_case(character)) - morse_tree;
}

/* Play the path from the root of the tree to a node, followed by the gap that
   goes after the character. Each level of the tree starts at index 2^k - 1. */
void play_character(int character)
{
    int index = morse_tree_index(character);
    int level_start;

    if (index != 0) {
        for (level_start = 255; level_start != 0; level_start /= 2) {
            if (index >= level_start) {
                int offset = index - level_start;
                int is_dash = offset / (level_start / 2 + 1) % 2;

                play(is_dash ? DASH_SLOTS + SYMBOL_GAP_SLOTS
                             : DOT_SLOTS + SYMBOL_GAP_SLOTS,
                     is_dash ? DASH_SLOTS : DOT_SLOTS);
            }
        }
    }

    play(character == ' ' ? WORD_GAP_SLOTS : LETTER_GAP_SLOTS, 0);
}

/* Convert text to audio. */
void encode(void)
{
    while (fread(morse_tree, 1, 1, stdin) == 1)
        play_character(morse_tree[0]);
}

/* Estimate how many blocks one Morse unit lasts. Every pair length is compared
   with the others. Two lengths match if they are about the same, or if the
   first is about a third of the second, which is a dot and gap compared to a
   dash and letter gap. The length with the most matches is taken to be two
   units, and the unit is half of the average of its matches. */
int estimate_unit_length(void)
{
    int best_matches = 0;
    int best_unit = 0;
    int matches = 0;
    int unit = 0;
    int pair;

    for (pair = 1; pair <= HISTORY_SIZE * HISTORY_SIZE; pair++) {
        int length = event_pair_lengths[pair / HISTORY_SIZE];
        int other_length = event_pair_lengths[pair % HISTORY_SIZE];
        int ratio = other_length ? length * 10 / other_length : 0; /* in tenths */

        if (ratio < 5)
            ratio *= 3;

        if (ratio > 7 && ratio < 13) {
            unit = (matches * unit + length / 2) / (matches + 1);
            matches++;
        }

        if (pair % HISTORY_SIZE == 0) {
            if (matches > best_matches) {
                best_unit = unit;
                best_matches = matches;
            }
            matches = 0;
            unit = 0;
        }
    }

    return best_unit;
}

/* Print the letter made of the symbols collected so far and start a new one. */
void finish_letter(void)
{
    if (symbol_count <= MAX_MORSE_SYMBOLS)
        putchar(morse_tree[symbol_bits - 1 + (1 << symbol_count)]);

    symbol_count = 0;
    symbol_bits = 0;
}

/* Turn the events that have not been looked at yet into text. A tone longer
   than two units is a dash. A silence longer than two units ends a letter and
   one longer than six units ends a word. At the end of the input one more event
   is handled, which is the one still being measured. */
void decode_events(int unit_length, int end_of_input)
{
    int last_event = event_count + end_of_input;

    while (next_event != last_event && event_count >= MIN_EVENTS) {
        if (next_event % 2 == 1) {
            int is_dash = event_lengths[next_event % HISTORY_SIZE] > 2 * unit_length;

            next_event++;
            symbol_count++;
            symbol_bits = 2 * symbol_bits + is_dash;
        }

        if (next_event != last_event) {
            int silence = event_lengths[next_event % HISTORY_SIZE];

            if (silence > 2 * unit_length || next_event == event_count)
                finish_letter();
            if (silence > 6 * unit_length)
                putchar(' ');
            next_event++;
        }
    }
}

/* Convert audio to text. Each block is compared with the block three blocks
   ago, so an edge is still seen if the sound takes a while to rise or fall. */
void decode(void)
{
    char sample;
    int block_position = 0;
    int loudness = 0;
    int loudness_1_ago = 0;
    int loudness_2_ago = 0;
    int loudness_3_ago = 0;
    int current_length = 0;
    int previous_length = 0;

    while (fread(&sample, 1, 1, stdin) == 1) {
        block_position++;

        if (block_position < BLOCK_BYTES) {
            /* Only the high byte of each sample is measured. */
            if (block_position % 2 == 0)
                loudness += sample < 0 ? -sample : sample;
        } else {
            /* After an even number of events a tone should start, so the sound
               has to get louder. After an odd number it has to get quieter. */
            int direction = event_count % 2 == 0 ? 1 : -1;
            int smaller = loudness < loudness_3_ago ? loudness : loudness_3_ago;

            int change = direction * (loudness - loudness_3_ago) / (smaller | 1);

            if (change > MIN_LOUDNESS_RATIO) {
                int slot = event_count % HISTORY_SIZE;

                event_pair_lengths[slot] = current_length + previous_length;
                event_lengths[slot] = current_length;
                previous_length = current_length;
                current_length = 0;
                event_count++;
            }
            current_length++;

            decode_events(estimate_unit_length(), 0);

            loudness_3_ago = loudness_2_ago;
            loudness_2_ago = loudness_1_ago;
            loudness_1_ago = loudness;
            loudness = 0;
            block_position = 0;
        }
    }

    /* The unit length of zero makes any silence the end of a letter. */
    decode_events(0, 1);
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        encode();
    } else if (argc > 1 && argc < 5) {
        if (argv[1][0] == 'e') {
            decode();
        } else {
            /* Any other argument reads the input and produces nothing. */
            while (getchar() != EOF)
                ;
        }
    } else if (argc >= 5) {
        /* The original treats a large argument count as a tone to play. */
        play(argc / 4, argc % 4);
    }

    return 0;
}
