#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;

#define MAX_INPUT_SIZE (1 << 20) /* 1 MiB, inputs this big are rejected */

/* Exit codes (the original never produces the documented code 1) */
#define EXIT_BAD_VARINT        0 /* original computes !k with k == 1 */
#define EXIT_VARINT_TOO_BIG    2
#define EXIT_BAD_EVENT         3
#define EXIT_BAD_TRACK         4
#define EXIT_SMPTE             5
#define EXIT_NO_BEAT_MEMORY    6
#define EXIT_NO_TRACK_MEMORY   7
#define EXIT_NO_INPUT_MEMORY   8
#define EXIT_INPUT_TOO_BIG     9
#define EXIT_BAD_HEADER       10

/* MIDI header layout */
#define HEADER_TRACK_COUNT    10
#define HEADER_DIVISION       12
#define HEADER_SIZE           14
#define TRACK_HEADER_SIZE      8

/* Percussion (General MIDI channel 10) notes */
#define NOTE_BASS_DRUM        36
#define NOTE_SNARE            40
#define NOTE_CLOSED_HIHAT     42
#define FIRST_NOTE_VELOCITY  100
#define FIRST_NOTE_OFF_NOTE   16 /* the first beat turns off this note */

u1 *beat;          /* one note-on/note-off pair, using running status */
u1 *note_off_note; /* points at the note number of the beat's note-off */

u2 read_be16(u1 *c) { return c[1] | (u2)c[0] << 8; }

void write_be16(u1 *c, u2 value) {
  c[0] = value >> 8;
  c[1] = value;
}

u4 read_be32(u1 *c) { return (u4)read_be16(c) << 16 | read_be16(c + 2); }

/* Reads a MIDI variable-length integer, returns how many bytes it used. */
int read_varint(u1 *c, u4 *value) {
  u1 i, b;
  *value = 0;
  for (i = 0; i < 4; i++) {
    b = c[i];
    *value |= b & 0x7f;
    if (!(b & 0x80)) return i + 1;
    *value = *value << 7;
  }
  exit(EXIT_BAD_VARINT);
  return 0;
}

/* Number of 7-bit groups needed to hold value. */
u1 varint_bytes(u4 value) {
  u1 bits = 0;
  if (value) {
    bits = 1;
    while (value >>= 1) bits++;
  }
  return bits / 7 + !!(bits % 7);
}

/* Writes value as a MIDI variable-length integer (without continuation bits,
 * exactly like the original). Returns the number of bytes written. */
int write_varint(u4 value, u1 *c) {
  u1 size = varint_bytes(value);
  *c = 0;
  if (!value) return size;
  if (size > 4) exit(EXIT_VARINT_TOO_BIG);
  c += size - 1;
  while (value) {
    *c = value & 0x7f;
    value >>= 7;
    c--;
  }
  return size;
}

/* Skips one MIDI event (after its delta time), tracking running status. */
u1 *skip_event(u1 *c, u1 *running_status) {
  u4 length;
  u1 status, type = *c;
  if (type == 0xf0 || type == 0xf7) { /* system exclusive */
    *running_status = 0;
    c++;
    c += read_varint(c, &length);
    return c + length;
  }
  if (type == 0xff) { /* meta event */
    *running_status = 0;
    c += 2;
    c += read_varint(c, &length);
    return c + length;
  }
  if (type & 0x80) { /* new status byte */
    status = *c++;
    *running_status = status;
  } else {
    status = *running_status;
  }
  /* Only note off/on, aftertouch, controller and program change */
  type = status >> 4;
  if (type < 0x8 || type > 0xc) exit(EXIT_BAD_EVENT);
  if (type != 0xc) c++; /* two data bytes */
  c++;
  return c;
}

/* Sums the delta times of one track. Returns a pointer to the next track. */
u1 *track_ticks(u1 *c, u4 *ticks) {
  u4 length, delta;
  u1 *end, running_status = 0;
  *ticks = 0;
  if (c[3] != 'k') exit(EXIT_BAD_TRACK);
  length = read_be32(c + 4);
  c += TRACK_HEADER_SIZE;
  end = c + length;
  while (c < end) {
    c += read_varint(c, &delta);
    *ticks += delta;
    c = skip_event(c, &running_status);
  }
  return c;
}

u4 longest_track_ticks(u1 *c, u4 track_count) {
  u4 i, ticks, longest = 0;
  for (i = 0; i < track_count; i++) {
    c = track_ticks(c, &ticks);
    if (ticks > longest) longest = ticks;
  }
  return longest;
}

/* Writes beat number i: bass drum on 0, snare on 2, hi-hat otherwise. */
u1 *write_beat(u1 *c, u4 i, u4 beat_size) {
  u1 note = NOTE_CLOSED_HIHAT, velocity = 80;
  if (i % 4 == 0) {
    note = NOTE_BASS_DRUM;
    velocity = 120;
  } else if (i % 4 == 2) {
    note = NOTE_SNARE;
    velocity = 100;
  }
  beat[1] = note;
  beat[2] = velocity;
  *note_off_note = note;
  memcpy(c, beat, beat_size);
  return c + beat_size;
}

void add_drum_track(u1 *midi, u4 midi_size) {
  u1 *track, *d;
  u2 ticks_per_beat, track_count;
  u4 beats, beat_size, track_size, i;

  track_count = read_be16(midi + HEADER_TRACK_COUNT);
  ticks_per_beat = read_be16(midi + HEADER_DIVISION) >> 1; /* eighth notes */
  if (ticks_per_beat >> 15) exit(EXIT_SMPTE);
  beats = longest_track_ticks(midi + HEADER_SIZE, track_count) / ticks_per_beat;

  /* note-on: 00 note velocity, note-off: <delta> note 00 */
  beat_size = 5 + varint_bytes(ticks_per_beat);
  if (!(beat = malloc(beat_size))) exit(EXIT_NO_BEAT_MEMORY);

  /* track header + running status byte + beats + end-of-track */
  track_size = beats * beat_size + 13;
  track = malloc(track_size);
  if (!track) exit(EXIT_NO_TRACK_MEMORY);
  memcpy(track, "MTrk", 4);
  write_be16(track + 6, track_size - TRACK_HEADER_SIZE);
  write_be16(track + 4, (track_size - TRACK_HEADER_SIZE) >> 16);

  d = beat;
  *d++ = 0;
  *d++ = NOTE_BASS_DRUM;
  *d++ = FIRST_NOTE_VELOCITY;
  d += write_varint(ticks_per_beat, d);
  note_off_note = d;
  *d++ = FIRST_NOTE_OFF_NOTE;
  *d = 0;

  d = track + TRACK_HEADER_SIZE;
  *d++ = 0;    /* delta time */
  *d++ = 0x99; /* note on, channel 10 (percussion) */
  memcpy(d, beat + 1, beat_size - 1);
  d += beat_size - 1;
  for (i = 1; i < beats; i++) d = write_beat(d, i, beat_size);

  *d++ = 0; /* end of track meta event */
  *d++ = 0xff;
  *d++ = 0x2f;
  *d = 0;

  midi[135] = 1; /* the original meant midi[9] (format 1) but misses */
  write_be16(midi + HEADER_TRACK_COUNT, track_count + 1);
  fwrite(midi, midi_size, 1, stdout);
  fwrite(track, track_size, 1, stdout);
}

int main() {
  u4 size;
  u1 *midi = malloc(MAX_INPUT_SIZE);
  if (!midi) exit(EXIT_NO_INPUT_MEMORY);
  size = fread(midi, 1, MAX_INPUT_SIZE, stdin);
  if (size >= MAX_INPUT_SIZE) exit(EXIT_INPUT_TOO_BIG);
  /* "MThd" header with a length of 6 */
  if (midi[3] != 'd' || midi[7] != 6) exit(EXIT_BAD_HEADER);
  add_drum_track(midi, size);
  return 0;
}
