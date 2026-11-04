#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Sliding tile puzzle (the "Fifteen Puzzle") on a 2D or 3D board.
 *
 * The original keeps all of its data in a stack region it reserves by
 * recursing into main() thousands of times, and implements every loop by
 * recursion. This version uses an ordinary array and ordinary loops.
 * Consequently it does not crash after "a very large number of moves" (or
 * after end of input, where the original keeps recursing until the stack
 * overflows); this version just keeps looping instead.
 */

#ifndef NO_STTY
#define RAW_MODE() system("stty cbreak -echo")
#define COOKED_MODE() system("stty echo")
#else
#define RAW_MODE() 0
#define COOKED_MODE() 0
#endif

static long width;  /* tiles per row (x) */
static long depth;  /* rows per level (y), 1 for a 2D board */
static long height; /* number of levels (z) */
static long size;   /* total number of cells: width * depth * height */
static long hole;   /* position of the empty cell */
static long field_width; /* characters used to print one tile */
static long *board; /* board[pos] is the tile at pos, 0 is the empty cell */

#define COLUMN(pos) ((pos) % width)
#define ROW(pos) (((pos) % (width * depth)) / width)
#define LEVEL(pos) ((pos) / (width * depth))

/*
 * Restore the terminal and exit. The original only exits when the stty
 * command succeeds; otherwise it falls through and returns a true value.
 */
static int quit(void)
{
	if (COOKED_MODE() == 0)
		exit(0);
	return 1;
}

static void render(void)
{
	long pos;

	printf("\x1b[;H"); /* cursor to the top-left corner */
	for (pos = 0; pos < size; pos++) {
		/* indent each row so the levels look like they have depth */
		if (COLUMN(pos) == 0)
			printf("%*s", (int)(depth - ROW(pos)), "");

		if (board[pos])
			printf("%*ld", (int)field_width, board[pos]);
		else
			printf("%*.*s", (int)field_width, (int)field_width, "#####");

		if (COLUMN(pos) == width - 1)
			printf("\n");
		else
			printf("%*s", (int)(depth + 1), "");

		/* blank line between levels of a 3D board */
		if ((pos + 1) % (width * depth) == 0 && depth > 1)
			printf("\n");
	}
}

/* true if tile already appears in board[0 .. count-1] */
static int already_placed(long tile, long count)
{
	long pos;

	for (pos = 0; pos < count; pos++)
		if (board[pos] == tile)
			return 1;
	return 0;
}

static void shuffle(void)
{
	long pos, tile;

	for (pos = 0; pos < size; pos++) {
		do
			tile = rand() % size;
		while (already_placed(tile, pos));

		board[pos] = tile;
		if (tile == 0)
			hole = pos;
	}
}

/* solved when the tiles read 1, 2, ..., size-1 followed by the hole */
static int solved(void)
{
	long pos;

	for (pos = 0; pos < size; pos++)
		if (board[pos] != (pos + 1) % size)
			return 0;
	return 1;
}

/* move the hole by offset cells by sliding the neighbouring tile into it */
static void slide(long offset)
{
	board[hole] = board[hole + offset];
	board[hole + offset] = 0;
	hole += offset;
	render();
}

int
main(int argc, char **argv)
{
	if (argc < 3) {
		printf("Syntax: %s <x> <y> [<z>]\n", argv[0]);
		return quit();
	}
	if (argc > 4) {
		/* the original uses argc as an internal state number here,
		   which crashes or does something strange */
		return 1;
	}

	if (argc == 3) {
		/* a 2D board is a 3D board that is one row deep */
		sscanf(argv[1], "%ld", &width);
		sscanf(argv[2], "%ld", &height);
		depth = 1;
	} else {
		sscanf(argv[1], "%ld", &width);
		sscanf(argv[2], "%ld", &depth);
		sscanf(argv[3], "%ld", &height);
	}
	size = width * depth * height;
	board = calloc(size > 0 ? size : 1, sizeof *board);

	srand(time(0));
	shuffle();

	/* number of digits in size */
	field_width = snprintf(NULL, 0, "%ld", size);

	if (RAW_MODE())
		return 1; /* the original returns into its smashed stack */

	printf("\x1b[2J"); /* clear the screen */
	render();

	while (!solved()) {
		switch (getchar()) {
		case 'i': /* hole up */
			if (0 < LEVEL(hole))
				slide(-width * depth);
			break;
		case 'j': /* hole left */
			if (0 < COLUMN(hole))
				slide(-1);
			break;
		case 'k': /* hole down */
			if (LEVEL(hole) < height - 1)
				slide(width * depth);
			break;
		case 'l': /* hole right */
			if (COLUMN(hole) < width - 1)
				slide(1);
			break;
		case 'n': /* hole forwards */
			if (ROW(hole) < depth - 1)
				slide(width);
			break;
		case 'o': /* hole backwards */
			if (0 < ROW(hole))
				slide(-width);
			break;
		case 'q':
			printf("You lose :-)\n");
			return quit();
		}
	}
	printf("You win!\n");
	return quit();
}
