#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

/*
 * Compares two text files with the O(NP) algorithm of Wu, Manber, Myers and
 * Miller and prints the edits in the default diff(1) format, or only the edit
 * distance with -d.
 *
 * Lines are compared by their FNV-1a hash alone. Line numbers start at 1, and
 * text after the last newline of a file is ignored.
 *
 * Exit status: 0 for no difference, 1 for a difference, 2 for an error.
 */

#define EXIT_TROUBLE 2

#define INITIAL_LINE_CAPACITY 65536
#define READ_BUFFER_SIZE 65536

#define FNV_OFFSET_BASIS 0xCBF29CE484222325ULL
#define FNV_PRIME 0x100000001B3ULL

/* One inserted or deleted line. Edits are linked from last to first. */
typedef struct edit {
	int is_insert;       /* 1 = line added from file 2, 0 = line deleted from file 1 */
	int line_1;          /* line number in file 1 */
	int line_2;          /* line number in file 2 */
	long offset_1;       /* byte offset of that line in file 1 */
	long offset_2;       /* byte offset of that line in file 2 */
	struct edit *next;
} Edit;

/* The furthest point reached on one diagonal of the edit graph. */
typedef struct {
	int furthest_y;
	Edit *edits;         /* the path that reached it */
} DiagonalEnd;

typedef struct {
	long offset;
	unsigned long long hash;
} Line;

/* A growable array of lines. lines[0] is unused so that line n is lines[n]. */
typedef struct {
	unsigned long capacity;
	unsigned long count;
	Line lines[1];
} LineTable;

int files_swapped;       /* the shorter file must be A, so file 2 may be A */
int print_distance_only;
LineTable *table_a, *table_b;      /* table_a is never longer than table_b */
FILE *file_1, *file_2;             /* always in command line order */
DiagonalEnd *diagonals;            /* indexed by diagonal, which may be negative */
DiagonalEnd *diagonal_storage;

/* Creates an empty table if given NULL, otherwise doubles its capacity. */
LineTable *
grow_line_table(LineTable *old_table)
{
	unsigned long capacity;
	LineTable *table;

	capacity = old_table ? old_table->capacity << 1 : INITIAL_LINE_CAPACITY;
	table = realloc(old_table, sizeof (*old_table) + capacity * sizeof (old_table->lines));
	if (table == NULL)
		err(EXIT_TROUBLE, 0);

	table->capacity = capacity;
	if (old_table == NULL)
		table->count = 0;
	return table;
}

/* Records the offset and FNV-1a hash of every line, then rewinds the file. */
LineTable *
hash_lines(FILE *file)
{
	unsigned long long hash;
	LineTable *table;
	long line, offset, bytes_left;
	unsigned char buffer[READ_BUFFER_SIZE], *byte;

	table = grow_line_table(NULL);
	line = 1;
	offset = 0;
	hash = FNV_OFFSET_BASIS;
	table->lines[line].offset = offset;

	while (0 < (bytes_left = fread(buffer, 1, sizeof (buffer), file))) {
		for (byte = buffer; 0 < bytes_left; byte++, bytes_left--) {
			hash ^= *byte;
			hash *= FNV_PRIME;
			offset++;

			if (*byte == '\n') {
				table->lines[line].hash = hash;
				line++;
				if (table->capacity <= line)
					table = grow_line_table(table);
				table->lines[line].offset = offset;
				hash = FNV_OFFSET_BASIS;
			}
		}
	}

	table->count = line - 1;
	rewind(file);
	return table;
}

/* Opens a file. A name of "-" that is not a real file means standard input,
 * which is copied to a temporary file because it must be seekable. */
FILE *
open_input(char *name)
{
	int ch;
	FILE *file;

	file = fopen(name, "r");
	if (file == NULL && strcmp("-", name) == 0) {
		file = tmpfile();
		if (file != NULL) {
			while ((ch = getchar()) != EOF)
				fputc(ch, file);
			rewind(file);
		}
	}
	if (file == NULL)
		err(EXIT_TROUBLE, "%s", name);
	return file;
}

/* Prints from the current file position up to and including the next newline. */
void
print_line(FILE *file)
{
	int ch;

	while ((ch = fgetc(file)) != EOF) {
		putchar(ch);
		if (ch == '\n')
			break;
	}
}

Edit *
reverse_edits(Edit *edit)
{
	Edit *next, *reversed;

	reversed = NULL;
	while (edit != NULL) {
		next = edit->next;
		edit->next = reversed;
		reversed = edit;
		edit = next;
	}
	return reversed;
}

/* Prints the edits in order, joining neighbouring lines into one diff hunk. */
void
print_edits(Edit *edits)
{
	Edit *first, *last, *after;

	first = reverse_edits(edits);
	while (first != NULL) {
		after = first;

		if (first->is_insert) {
			do {
				last = after;
				after = after->next;
			} while (after != NULL && after->is_insert
				&& last->line_2 + 1 == after->line_2);

			if (first->line_2 < last->line_2)
				printf("%da%d,%d\n", first->line_1, first->line_2, last->line_2);
			else
				printf("%da%d\n", first->line_1, first->line_2);

			fseek(file_2, first->offset_2, SEEK_SET);
			for ( ; first != after; first = first->next) {
				printf("> ");
				print_line(file_2);
			}
		} else {
			do {
				last = after;
				after = after->next;
			} while (after != NULL && !after->is_insert
				&& last->line_1 + 1 == after->line_1);

			if (first->line_1 < last->line_1)
				printf("%d,%dd%d\n", first->line_1, last->line_1, first->line_2);
			else
				printf("%dd%d\n", first->line_1, first->line_2);

			fseek(file_1, first->offset_1, SEEK_SET);
			for ( ; first != after; first = first->next) {
				printf("< ");
				print_line(file_1);
			}
		}
	}
}

/*
 * Moves diagonal k forward. x counts lines of A, y counts lines of B and
 * k = y - x. The step arrives from the neighbouring diagonal that got further,
 * which adds one edit, then slides along every matching line for free.
 */
void
extend_diagonal(int k)
{
	DiagonalEnd below = diagonals[k-1];
	DiagonalEnd above = diagonals[k+1];
	int x, y, is_insert;
	Edit *path;

	if (above.furthest_y < below.furthest_y + 1) {
		is_insert = 1;
		y = below.furthest_y + 1;
		path = below.edits;
	} else {
		is_insert = 0;
		y = above.furthest_y;
		path = above.edits;
	}
	x = y - k;

	/* The very first step only enters the graph, so it is not an edit. */
	if (0 < y || 0 < x) {
		Edit *edit = malloc(sizeof (*edit));

		edit->next = path;
		if (files_swapped) {
			/* An insert into A is a delete from file 1, and the reverse. */
			edit->is_insert = !is_insert;
			edit->line_1 = y;
			edit->line_2 = x;
			edit->offset_1 = table_b->lines[y].offset;
			edit->offset_2 = table_a->lines[x].offset;
		} else {
			edit->is_insert = is_insert;
			edit->line_1 = x;
			edit->line_2 = y;
			edit->offset_1 = table_a->lines[x].offset;
			edit->offset_2 = table_b->lines[y].offset;
		}
		diagonals[k].edits = edit;
	}

	while (x < table_a->count && y < table_b->count
	&& table_a->lines[x+1].hash == table_b->lines[y+1].hash) {
		x++;
		y++;
	}
	diagonals[k].furthest_y = y;
}

/* Runs the O(NP) search, prints the edits and returns the edit distance. */
int
compare_files(void)
{
	int k, deletions, length_difference;
	unsigned long diagonal_count;

	if (table_a->count > table_b->count) {
		LineTable *longer = table_a;
		table_a = table_b;
		table_b = longer;
		files_swapped = 1;
	}
	length_difference = table_b->count - table_a->count;

	diagonal_count = table_a->count + table_b->count + 3;
	diagonal_storage = calloc(diagonal_count, sizeof (*diagonals));
	if (diagonal_storage == NULL)
		err(EXIT_TROUBLE, 0);
	for (k = 0; k < diagonal_count; k++)
		diagonal_storage[k].furthest_y = -1;
	diagonals = diagonal_storage + table_a->count + 1;

	/* Allow one more deletion each round until the end of B is reached.
	 * Both sweeps work towards the diagonal holding the final point. */
	for (deletions = 0; ; deletions++) {
		for (k = -deletions; k < length_difference; k++)
			extend_diagonal(k);
		for (k = length_difference + deletions; length_difference <= k; k--)
			extend_diagonal(k);

		if (diagonals[length_difference].furthest_y == table_b->count)
			break;
	}

	if (!print_distance_only)
		print_edits(diagonals[length_difference].edits);
	free(diagonal_storage);

	/* Each deletion needs an insert to balance it, on top of the extra lines. */
	return length_difference + 2 * deletions;
}

int
main(int argc, char **argv)
{
	int option, distance;

	while ((option = getopt(argc, argv, "d")) != -1) {
		switch (option) {
		case 'd':
			print_distance_only = 1;
			break;
		default:
			optind = argc;    /* force the usage message below */
			break;
		}
	}
	if (argc <= optind)
		errx(EXIT_TROUBLE, "usage: %s [-d] file1 file2", argv[0]);

	file_1 = open_input(argv[optind]);
	file_2 = open_input(argv[optind+1]);
	table_a = hash_lines(file_1);
	table_b = hash_lines(file_2);

	distance = compare_files();
	if (print_distance_only)
		printf("%d\n", distance);

	return 0 < distance;
}
