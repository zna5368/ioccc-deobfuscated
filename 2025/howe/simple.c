#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Glyph table.  Every glyph takes exactly 4 bytes so that glyph number n
 * starts at glyphs + n * GLYPH_SIZE.  The full width digits and the full
 * width space are 3 bytes of UTF-8 followed by a NUL; the two emoji are a
 * full 4 bytes.  Print one with "%.4s".
 */
#define GLYPH_SIZE 4

enum {
	VETERAN = 10,	/* 👾 */
	UFO     = 11,	/* 🛸 */
	BLANK   = 12,	/* full width space, an empty cell */
	TARGETS = 12	/* aim cycles through 0..9, veteran, UFO */
};

char glyphs[] =
	"０\0" "１\0" "２\0" "３\0" "４\0"
	"５\0" "６\0" "７\0" "８\0" "９\0"
	"👾" "🛸" "　";

#define SCREEN_CELLS 6		/* invaders visible at once */
#define WAVE_START   6		/* first invader of the wave in field[] */
#define WAVE_SIZE    16		/* invaders per wave */
#define WAVE_END     (WAVE_START + WAVE_SIZE)	/* 22 */
#define FIELD_SIZE   (WAVE_END + SCREEN_CELLS)	/* 28 */
#define AMMO_PER_WAVE 30
#define SHIELD_LAYERS 3

int aim, shield_damage, ammo;

/*
 * The invaders of the current wave.  The screen is a window of
 * SCREEN_CELLS cells starting at field[view]; field[view] is the cell next
 * to the base, so moving the window to the right makes the invaders march
 * left towards the base.
 */
char field[FIELD_SIZE];
unsigned view, ticks, score, kills;
struct termios term[2];		/* [0] saved settings, [1] game settings */

int main()
{
	char key;
	unsigned i;

	/* Raw terminal: no line buffering, no echo, ^C is a normal key, and
	 * read() gives up after 0.1 seconds so the game keeps running. */
	tcgetattr(0, term);
	term[1] = term[0];
	term[1].c_cc[VMIN] = 0;
	term[1].c_cc[VTIME] = 1;
	term[1].c_lflag &= ~(ICANON | ECHO | ISIG);
	tcsetattr(0, 0, term + 1);

	view = FIELD_SIZE;	/* force a new wave on the first pass */
	do {
		/* Start a new wave when the old one has passed or is cleared. */
		if (view >= WAVE_END || field[WAVE_END - 1] == BLANK) {
			memset(field, BLANK, FIELD_SIZE);
			getentropy(field + WAVE_START, WAVE_SIZE);
			for (i = WAVE_START; i < WAVE_END; i++)
				field[i] %= VETERAN + 1;	/* 0..9 or veteran */
			ammo = AMMO_PER_WAVE;
			view = 1;
		}

		/* Status line: ammo, aim, shield, then the invaders. */
		printf(" %.2d %.4s%s ", ammo, glyphs + aim * GLYPH_SIZE,
		       "|||" + shield_damage);
		for (i = 0; i < SCREEN_CELLS; i++)
			printf("%.4s", glyphs + field[view + i] * GLYPH_SIZE);
		printf(" \r");

		key = fflush(stdout);	/* key = 0 if no key is pressed */
		read(0, &key, 1);

		switch (key) {
		case '\t':
			aim = (aim + 1) % TARGETS;
			break;
		case '\r':
		case '\n':
			if (ammo > 0) {
				ammo--;
				/* Kill the matching invader closest to the base. */
				for (i = 0; i < SCREEN_CELLS; i++) {
					if (field[view + i] == aim) {
						int who = field[view + i];

						if (who < 10)
							score += who + (i + 1) * 10;
						else if (who == VETERAN)
							score += 150;
						else
							score += 300;

						/* Invaders in front of the target are
						 * knocked back one cell. */
						memmove(field + view + 1, field + view, i);
						field[view] = BLANK;

						/* A score that is a multiple of 10 calls
						 * in a UFO just beyond the screen. */
						if (view < WAVE_END - SCREEN_CELLS && score % 10 == 0)
							field[view + SCREEN_CELLS] = UFO;
						kills++;
						break;
					}
				}
			}
			break;
		}

		/* Every so many ticks the invaders advance one cell.  The pace
		 * speeds up for 16 kills out of every 64.  An invader that
		 * reaches the base damages the shield. */
		if (++ticks % (20 - kills % 64 / 16 * PACE) == 0) {
			if (field[view++] != BLANK)
				shield_damage++;
		}
	} while (shield_damage <= SHIELD_LAYERS && key != '\003');

	printf("\r%u %s%u%30s\n", kills, "👽 ", score, "");
	return tcsetattr(0, 0, term);
}
