#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

/*
 * Music
 *
 * Each song voice is a pair of strings of the same length: one holds the
 * notes and the other holds how long each note lasts.
 *
 *   note character     note number = character - ':'  (':' is note 0)
 *                      note 45 ('g') is A4 = 440 Hz, one step is a semitone
 *                      '~' is a rest (silence)
 *   length character   length = (character - ':') * 1000 samples,
 *                      so ';' is 1/8 second at 8000 samples per second
 */
#define CODE_BASE        ':'
#define REST             '~'
#define SAMPLE_RATE      8000
#define SAMPLES_PER_TICK 1000
#define NOTE_COUNT       64
#define NOTE_A4          45

static char *melody_notes =
    "~\\^__a^\\Z~\\\\^_ZZffa~\\^__a^\\Z~\\\\^_\\Zaaaca~_~ac_aaacaZ~\\^_\\aca~ZZZ\\ccaZZ\\Zaa_ZZ\\Z_a^\\ZZa_ZZ\\Zcca~ZZ\\Zf^_^\\ZZ\\Z_a^\\ZZa_~__\\_a~^\\Z~\\\\^_\\Zffaca_\\\\^_\\_a^\\Z~\\\\^_\\Z~acca_~ac_aaacaZ~\\^_\\aca~ZZZ\\ccaZZ\\Zaa_ZZ\\Z_a^\\ZZa_ZZ\\ZccaZZ\\Zf^_^\\ZZ\\Z_a^\\ZZa_~ZZ\\ZccaZZ\\Zaa_ZZ\\Z_a^\\ZZa_ZZ\\";
static char *melody_lengths =
    "?<<<<<=;?E<<<<><><>D<<<<<=;?E<<<<<><<<<?=C;<<<<<<<>>B<<<><<?;;;;;==@;;;;==@;;;;><=;><>B;;;;==?;;;;;><;;>;;;;><=;><>@B<<<<?=<<@@<<<<<@<<><<><<<<<<><<@@<<<<<?;<<>@C;<<<<<<<>>B<<<><<?;;;;;==@;;;;==@;;;;><=;><>B;;;;==@;;;;><;;>;;;;><=;><>@<;;;;==@;;;;==@;;;;><=;><>B;;;";
static char *bass_notes =
    "~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~D~D~B~B~D~D~B~B~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~D~D~B~B~D~D~B~B~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L~L~N~K~L";
static char *bass_lengths =
    ";?;???;>@?;???;>@?;???;???;>@D@?;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;???;?";

/*
 * Pictures
 *
 * Each frame is an 80 x 24 picture made of run length encoded colored
 * spaces.  For each character, code = character - ':'
 *
 *   code  0..15   one space of color code
 *   code 16..31   two spaces of color (code & 15)
 *   code 32..67   a run of color (code & 15) whose length is given by the
 *                 next character minus '8'
 *   '~'           end of the frame
 */
#define END_OF_FRAME  '~'
#define RUN_BASE      '8'
#define SCREEN_WIDTH  80

static char frame_pose1[] =
    "ZxZxZxZxZxZxZl`<^<ZxZ>_>^>ZxZ<Nb@NZxZ<NTRd<NZxZ<QNQb>ZxZ>Q[<b<ZxZ>QVQRZxZBh<RZxZBh<RWh<ZxZ<h<QWhBZtOJVg<hDZnh<OJg<hHZjh@Jg<hHZjh@Jg<hHZjh@Jg<hHZjh@Jg<hJZhh@Jg<Qh@Yh>Zhh@Jg<a<h>Yh>Zhh@Jg<Xa<h<Yh@ZH~";
static char frame_pose2[] =
    "ZxZxZxZ>]<^<ZxZ@^<_<NZxZ>Na<RNZxZ>MQRSNZxZ<RPLRLQZxZ<a<VQRQZxZ>QVQRZxZ@a<RZxZBa<RZxZBa>Wh<Zv_<Wh<WhDZlXOXOWh<WhFZjh@Wh<WhFZjXYh>Jg<hFZjhBJg<hFZjhBJWhJZhhBJWh<Qb<Xi<h<Zfi>h<JWh<Qb<XYh@Zbh<i<h<JWh>a<YhBZbh@QXJWh>i>hDZbh<a>JWXiBhBZdi<Ra<JXiFZP~";
static char frame_pose3[] =
    "ZxZxZxZxZL`BZxZ>N_<^<ZxZ>TQRSTZxZ<TQKb<TZxZ<La<RTZxZ>a>KRQZxZ>Va<RZxZ@f<RZxZBa<RWZxZ>WQh<Wh<ZtOXOWJh<Wh@Znh@g<h<WhBZjhBg<JWhFZhhDWJWhFZhhDWJWhFZhXYh@WJhHZhXYh@WJhHZhXi<h>WJh>a<Rh<ZhXi<h>WJh>b<Qh<ZhXi<h>WJh>a>h<ZhXi>h<WJh>YQh>ZP~";

/* the dance: the order in which the frames are shown, over and over */
#define DANCE_STEPS 8
static char *dance[DANCE_STEPS] = {
    frame_pose1, frame_pose2, frame_pose2, frame_pose3,
    frame_pose3, frame_pose2, frame_pose2, frame_pose1
};

/* the 16 colors used by the pictures, as 0xRRGGBB */
static int palette[16] = {
    0x909090, 0xAC783C, 0xD0805C, 0xE09470,
    0x644818, 0x442800, 0x985C28, 0xD07070,
    0xE08888, 0xECA0A0, 0x846830, 0xB03C3C,
    0xC05858, 0xC8C8C8, 0x404040, 0x2C3000
};

static char *aplay_path = "/usr/bin/aplay";
static char *raw_output_file = "r.raw";

#define SONG_SAMPLES 704000             /* 88 seconds at 8000 Hz */

static int note_frequency[NOTE_COUNT];
static unsigned char *song;
static int sample_pos = 0;

/* add one voice of the song into the audio buffer as a square wave */
static void add_voice(char *notes, char *lengths)
{
    char *note, *length;
    int duration, period, j;

    for (note = notes, length = lengths; *note; note++, length++) {
        duration = (*length - CODE_BASE) * SAMPLES_PER_TICK;

        if (*note == REST)
            period = 0;
        else
            period = (double)SAMPLE_RATE / note_frequency[*note - CODE_BASE];

        for (j = 0; j < duration; j++, sample_pos++) {
            /* high for the first half of each wave, low for the second */
            if (period && j % period < period / 2) {
                /* the second half of each note is quieter */
                song[sample_pos] += (j > duration / 2) ? 32 : 63;
            }
        }
    }
}

/* play the song with aplay, or save it to r.raw if aplay is missing */
static void play_song(void)
{
    FILE *out;
    int use_pipe = 0;

    if ((out = fopen(aplay_path, "r"))) {
        fclose(out);
        out = popen(aplay_path, "w");
        use_pipe = 1;
    } else {
        out = fopen(raw_output_file, "w");
    }

    if (out) {
        fwrite(song, SONG_SAMPLES, 1, out);
        if (use_pipe)
            pclose(out);
        else
            fclose(out);
    }
}

/* draw a frame at the top left of the terminal with 24-bit colors */
static void draw_frame(char *frame)
{
    int code, run, color, i, column = 0;

    printf("\x1b[1;1H");

    while (*frame != END_OF_FRAME) {
        code = *frame - CODE_BASE;
        if (code < 16)
            run = 1;
        else if (code < 32)
            run = 2;
        else
            run = *++frame - RUN_BASE;
        color = code & 15;

        printf("\x1b[48;2;%d;%d;%dm",
               (palette[color] >> 16) & 255,
               (palette[color] >> 8) & 255,
               palette[color] & 255);

        for (i = 0; i < run; i++, column++)
            printf(column % SCREEN_WIDTH ? " " : "\n ");

        frame++;
    }
}

int main(int argc, char **argv)
{
    int i, step = 0;

    for (i = 0; i < NOTE_COUNT; i++)
        note_frequency[i] = 440 * pow(2, (i - NOTE_A4) / 12.0);

    if (!(song = calloc(SONG_SAMPLES, 1)))
        return 1;

    add_voice(melody_notes, melody_lengths);
    sample_pos = 0;
    add_voice(bass_notes, bass_lengths);

    if (!fork()) {
        /* the child plays the music */
        play_song();
        for (;;)
            ;           /* never fall through into the animation */
    }

    /* the parent shows the animation, about four frames a second */
    for (;;) {
        draw_frame(dance[step++ % DANCE_STEPS]);
        usleep(SONG_SAMPLES / 3);
    }

    return 0;
}
