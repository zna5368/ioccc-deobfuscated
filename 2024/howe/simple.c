/*
 * eh - Edit Here - a small vi(1) style editor built on a "buffer gap".
 *
 * The headers (curses.h, ctype.h, string.h, stdlib.h, iso646.h, regex.h,
 * fcntl.h, locale.h, unistd.h) are supplied by the Makefile with -include,
 * as are BUF (buffer size) and MODE (file creation mode).
 *
 * The text lives in buffer[] with an empty "gap" between gap_start and
 * gap_end:
 *
 *   buffer                gap_start      gap_end              buffer_end
 *   |  text before cursor |   (unused)   |  text after cursor |
 *
 * Positions ("pos") are byte offsets into the text as if the gap did not
 * exist.  Inserting writes into the gap, deleting just widens the gap, so
 * the old bytes stay in memory, which is what makes the one-level undo work.
 */

#define CTRL_C          3
#define BACKSPACE       8
#define ESCAPE          27
#define MOTION_COMMANDS 14      /* first 14 commands are cursor motions */
#define ALL_COMMANDS    99
#define MAX_LINE_COLUMN 999     /* column used to reach the end of a line */

int cursor_row, cursor_col;     /* screen position of the cursor */
int count;                      /* numeric prefix of the current command */
int dollar_pattern;             /* search pattern is just "$" */
char *filename, *clipboard;
char buffer[BUF], *gap_start = buffer, *gap_end, *buffer_end;
char *undo_gap_start, *undo_gap_end;
long point;                     /* cursor position */
long undo_point;
long page_top, page_end;        /* first and one past last position shown */
long match_length;
long clipboard_length;
long mark = -1;                 /* selection start, or -1 for none */
regex_t pattern;

void execute_command(int limit);

/* Convert a pointer into the buffer into a text position. */
long ptr_to_pos(char *ptr)
{
	if (ptr < gap_end)
		return ptr - buffer;
	return ptr - buffer - (gap_end - gap_start);
}

/* Convert a text position into a pointer into the buffer, skipping the gap. */
char *pos_to_ptr(long pos)
{
	if (buffer + pos < gap_start)
		return buffer + pos;
	return buffer + pos + (gap_end - gap_start);
}

long text_length(void)
{
	return ptr_to_pos(buffer_end);
}

/* Number of bytes in a UTF-8 sequence that starts with this byte. */
int utf8_length(int lead)
{
	return 1 + (lead >= 0xC2) + (lead >= 0xE0) + (lead >= 0xF0);
}

/* Position of the next character. */
long next_char(long pos)
{
	if (pos < text_length())
		return pos + utf8_length(*pos_to_ptr(pos));
	return pos;
}

/* Position of the previous character (skip UTF-8 continuation bytes). */
long prev_char(long pos)
{
	while (0 < pos) {
		--pos;
		if ((*pos_to_ptr(pos) & 0xC0) != 0x80)
			break;
	}
	return pos;
}

/* Swap the current gap and cursor with the ones saved before the last edit. */
void cmd_undo(void)
{
	char *ptr;
	long pos;

	ptr = undo_gap_start;
	undo_gap_start = gap_start;
	gap_start = ptr;

	ptr = undo_gap_end;
	undo_gap_end = gap_end;
	gap_end = ptr;

	pos = undo_point;
	undo_point = point;
	point = pos;

	page_end = point + 1;
}

/* Move the gap so that it starts at pos and remember it for undo. */
void move_gap(long pos)
{
	char *ptr = pos_to_ptr(pos);

	while (ptr < gap_start)
		*--gap_end = *--gap_start;
	while (gap_end < ptr)
		*gap_start++ = *gap_end++;

	undo_gap_start = gap_start;
	undo_gap_end = gap_end;
	undo_point = point;
}

/* Number of screen columns used by the character at ptr shown at column. */
int char_width(char *ptr, int column)
{
	wchar_t wc;
	int width;

	mbtowc(&wc, ptr, 4);
	if (wc == '\t')
		return 8 - (column & 7);
	width = wcwidth(wc);
	return width < 1 ? 1 : width;
}

/* Position of the start of the physical line containing pos. */
long line_start(long pos)
{
	while (0 < pos) {
		--pos;
		if (*pos_to_ptr(pos) == '\n')
			break;
	}
	return 0 < pos ? pos + 1 : 0;
}

/*
 * Starting at pos which is shown at screen column, move right until
 * target_column is reached or the end of the line.
 */
long advance_to_column(long pos, int column, int target_column)
{
	char *ptr;

	while (column < target_column) {
		ptr = pos_to_ptr(pos);
		if (buffer_end <= ptr || *ptr == '\n')
			break;
		column += char_width(ptr, column);
		pos = next_char(pos);
	}
	return pos;
}

/*
 * Starting from the beginning of a physical line, find where the screen
 * row (a wrapped piece of the line) containing target starts.
 */
long row_start(long pos, long target)
{
	char *ptr;
	int column = 0;
	long start = pos;

	while (pos < target) {
		pos = next_char(pos);
		ptr = pos_to_ptr(pos);
		if (buffer_end <= ptr)
			break;
		column += char_width(ptr, column);
		if (COLS <= column) {
			start = pos;
			column = 0;
		}
	}
	return start;
}

/* Start of the screen row before the one containing pos. */
long prev_row(long pos)
{
	long line = line_start(pos);
	long row = row_start(line, pos);

	if (line < row)
		return row_start(line, row - 1);
	return row_start(line_start(line - 1), line - 1);
}

/* Start of the screen row after the one containing pos. */
long next_row(long pos)
{
	return next_char(advance_to_column(pos, cursor_col, COLS - 1));
}

/* Fill the rest of the screen line with spaces (shows the standout bar). */
void pad_line(void)
{
	printw("%*s", COLS - getcurx(stdscr), "");
}

void display(void)
{
	char *ptr;
	int row, column, length;
	long old_top, sel_start, sel_end, end = text_length();

	/* Scroll the page so that the cursor is visible. */
	if (point < page_top) {
		page_top = row_start(line_start(point), point);
	} else if (page_end <= point && point < next_row(page_end)) {
		page_top = next_row(page_top);
	} else if (page_end <= point) {
		old_top = page_top;
		page_top = point;
		row = LINES - 1 - (point == end);
		while (0 < --row && old_top < page_top)
			page_top = prev_row(page_top);
		if (page_top < old_top)
			page_top = old_top;
	}

	/* Status line. */
	erase();
	standout();
	printw("%s %ldB", filename, point);
	pad_line();

	if (mark < 0) {
		sel_start = sel_end = mark;
	} else if (point < mark) {
		sel_start = point;
		sel_end = mark;
	} else {
		sel_start = mark;
		sel_end = point;
	}

	/* Text, one character at a time. */
	row = 1;
	column = 0;
	page_end = page_top;
	for (;;) {
		standend();
		if (LINES <= row)
			break;
		if (point == page_end) {
			cursor_row = row;
			cursor_col = column;
		}
		ptr = pos_to_ptr(page_end);
		if (buffer_end <= ptr)
			break;
		if (sel_start <= page_end && page_end < sel_end)
			standout();
		length = utf8_length(*ptr);
		mvaddnstr(row, column, ptr, length);
		page_end += length;
		column += char_width(ptr, column);
		if (*ptr == '\n' || COLS <= column) {
			column = 0;
			row++;
		}
	}

	/* End of file marker. */
	if (row < LINES) {
		standout();
		mvaddstr(row + 1, 0, "^D");
	}

	move(cursor_row, cursor_col);
	refresh();
}

void cmd_redraw(void)
{
	clear();
	count = 0;
}

void cmd_left(void)
{
	point = prev_char(point);
}

void cmd_right(void)
{
	point = next_char(point);
}

void cmd_up(void)
{
	point = advance_to_column(prev_row(point), 0, cursor_col);
}

void cmd_down(void)
{
	point = advance_to_column(next_row(point), 0, cursor_col);
}

void cmd_goto_column(void)
{
	point = advance_to_column(line_start(point), 0, count - 1);
	count = 0;
}

void cmd_word_left(void)
{
	while (0 < point && isspace(*pos_to_ptr(point - 1)))
		--point;
	while (0 < point && !isspace(*pos_to_ptr(point - 1)))
		--point;
}

void cmd_word_right(void)
{
	long end = text_length();

	while (point < end && !isspace(*pos_to_ptr(point)))
		++point;
	while (point < end && isspace(*pos_to_ptr(point)))
		++point;
}

void cmd_page_top(void)
{
	point = page_top;
}

void cmd_page_bottom(void)
{
	point = row_start(line_start(page_end - 1), page_end - 1);
}

void cmd_page_down(void)
{
	int row;

	point = page_end;
	for (row = 1; row < cursor_row; row++)
		point = next_row(point);
	point = advance_to_column(point, 0, cursor_col);
	if (point < text_length())
		page_top = page_end;
	for (page_end = point; row < LINES; row++)
		page_end = next_row(page_end);
}

void cmd_page_up(void)
{
	int row;

	for (row = 0; row < LINES - 1; row++)
		point = prev_row(point);
	page_top = point;
	while (1 < cursor_row--)
		page_top = prev_row(page_top);
	point = advance_to_column(point, 0, cursor_col);
}

/* nG goes to line n, G alone goes to the end of the file. */
void cmd_goto_line(void)
{
	long end = text_length();

	point = count ? 0 : end;
	while (point < end && 1 < count) {
		point = advance_to_column(point, 0, MAX_LINE_COLUMN);
		if (point < end)
			point++;
		count--;
	}
	page_top = end;
}

void cmd_insert(void)
{
	int key, length;
	long original_end = text_length();

	move_gap(point);
	while ((key = getch()) != CTRL_C && key != ESCAPE) {
		length = utf8_length(key);
		if (key == BACKSPACE) {
			/* Remove one inserted UTF-8 character. */
			while (original_end < text_length()) {
				--gap_start;
				if ((*gap_start & 0xC0) != 0x80)
					break;
			}
		} else if (gap_start + length < gap_end) {
			/* Store the lead byte and any continuation bytes. */
			do {
				*gap_start++ = key;
				page_end++;
			} while (0 < --length && (key = getch()) != 0);
		}
		point = ptr_to_pos(gap_end);
		display();
	}
	count = 0;
}

/* Copy the selection, or the text covered by a motion, to the clipboard. */
void cmd_yank(void)
{
	long other_end = mark;
	long tmp;

	if (mark < 0) {
		other_end = point;
		execute_command(MOTION_COMMANDS);
	}
	if (other_end < point) {
		tmp = other_end;
		other_end = point;
		point = tmp;
	}
	free(clipboard);
	move_gap(point);
	clipboard_length = other_end - point;
	clipboard = strndup(gap_end, clipboard_length);
	mark = -1;
}

/* Cut: yank then drop the text by widening the gap. */
void cmd_delete(void)
{
	cmd_yank();
	gap_end += clipboard_length;
	point = ptr_to_pos(gap_end);
}

/* x is the same as dl. */
void cmd_delete_char(void)
{
	ungetch('l');
	cmd_delete();
}

void cmd_paste(void)
{
	if (clipboard) {
		move_gap(point);
		memcpy(gap_start, clipboard, clipboard_length);
		gap_start += clipboard_length;
		point = ptr_to_pos(gap_end);
		page_end = point + 1;
	}
}

void cmd_write(void)
{
	int fd;

	move_gap(0);
	fd = creat(filename, MODE);
	write(fd, gap_end, buffer_end - gap_end);
	close(fd);
}

void cmd_quit(void)
{
	filename = 0;
}

/* Search forward for the pattern, wrapping to the top of the buffer. */
void cmd_find_next(void)
{
	regmatch_t match[1];

	/* Move the gap to the end so the text is one C string. */
	move_gap(text_length());
	*gap_start = 0;

	if (point + match_length < text_length()
	    && !regexec(&pattern, pos_to_ptr(point + match_length), 1, match, REG_NOTBOL)) {
		point += match_length + match[0].rm_so;
	} else if (!regexec(&pattern, buffer, 1, match, 0)) {
		point = match[0].rm_so;
	} else {
		match_length = 0;
		return;
	}
	match_length = match[0].rm_eo - match[0].rm_so + dollar_pattern;
}

void cmd_search(void)
{
	/* Read the pattern on the status line, into the unused gap. */
	echo();
	standout();
	mvaddch(0, 0, '/');
	pad_line();
	mvgetnstr(0, 1, gap_start, gap_end - gap_start);

	regfree(&pattern);
	if (regcomp(&pattern, gap_start, REG_EXTENDED | REG_NEWLINE)) {
		beep();
	} else {
		dollar_pattern = gap_start[0] == '$' && !gap_start[1];
		cmd_find_next();
	}
	count = 0;
}

void cmd_toggle_selection(void)
{
	mark = mark < 0 ? point : -1;
}

char command_keys[] = "hjklbwHJKL|G/nixydPu\\WQ\003";

void (*command_table[])(void) = {
	/* motions */
	cmd_left, cmd_down, cmd_up, cmd_right, cmd_word_left, cmd_word_right,
	cmd_page_top, cmd_page_down, cmd_page_up, cmd_page_bottom,
	cmd_goto_column, cmd_goto_line,
	cmd_search, cmd_find_next,
	/* edits */
	cmd_insert, cmd_delete_char,
	cmd_yank, cmd_delete, cmd_paste, cmd_undo,
	cmd_toggle_selection, cmd_write, cmd_quit, cmd_quit,
	/* any other key */
	cmd_redraw
};

/*
 * Read an optional count and a command key, then run the command count
 * times.  Only the first limit commands are allowed.  A count given before
 * d or y is multiplied by the count given before the motion (2d3w = d6w).
 */
void execute_command(int limit)
{
	int outer_count = count, key, index;

	for (count = 0; isdigit(key = getch()); )
		count = count * 10 + key - '0';

	if (outer_count && count)
		count = outer_count * count;
	else if (!count)
		count = outer_count;

	for (index = 0; command_keys[index] && key != command_keys[index]; index++)
		;

	if (index < limit) {
		do
			command_table[index]();
		while (1 < count--);
	}
	count = 0;
}

int main(int argc, char **argv)
{
	int fd;
	ssize_t bytes;

	setlocale(LC_ALL, "");
	if (!initscr())
		return 1;
	raw();

	gap_end = buffer_end = buffer + BUF;
	filename = argc > 1 ? argv[1] : 0;
	fd = open(filename, O_RDONLY);
	if (0 < fd) {
		bytes = read(fd, buffer, buffer_end - buffer);
		close(fd);
		if (bytes < 0 || BUF <= bytes)
			return 2;
		gap_start += bytes;
	}

	page_end = 1;
	while (filename) {
		noecho();
		display();
		execute_command(ALL_COMMANDS);
	}
	endwin();
	return 0;
}
