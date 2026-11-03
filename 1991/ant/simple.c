/*
 * Ant's Editor - a tiny vi-like editor built on a "buffer gap".
 *
 * The file lives in one array.  The text before the cursor sits at the
 * start of the array, the text after the cursor sits at the end, and the
 * unused space (the gap) lies between them:
 *
 *     buffer            gap          gap_end          buffer_end
 *       |  front of file |   (unused)   |  back of file  |
 *
 * Inserting a character just writes it at `gap` and moves `gap` up.
 * A logical position (offset) counts characters of the file only,
 * skipping over the gap.
 */
#include <ctype.h>
#include <curses.h>

#define TAB_WIDTH 8
#define TAB_STOP(col) (TAB_WIDTH - ((col) & (TAB_WIDTH - 1)))

int screen_lines;     /* number of rows on the screen */
int page_start;       /* offset of the first character shown */
int page_end;         /* offset just after the last character shown */
int point;            /* offset of the cursor in the file */
int done;             /* set by the quit command */
int cursor_col;       /* screen column of the cursor (wanted column) */
int cursor_row;       /* screen row of the cursor */

char buffer[BUF];
char *buffer_end;     /* end of the array (= end of the back text) */
char *gap = buffer;   /* start of the gap (= end of the front text) */
char *gap_end;        /* end of the gap (= start of the back text) */
char *filename;

/* Convert a file offset into a pointer inside the buffer. */
char *ptr(int offset) {
  if (offset < 0)
    return buffer;
  if (buffer + offset < gap)
    return buffer + offset;
  return buffer + offset + (gap_end - gap);
}

/* Convert a pointer inside the buffer back into a file offset. */
int pos(char *pointer) {
  if (pointer < gap_end)
    return pointer - buffer;
  return pointer - buffer - (gap_end - gap);
}

int end_offset(void) { return pos(buffer_end); }

/* Move the gap so that it sits exactly at the cursor. */
void move_gap(void) {
  char *cursor = ptr(point);
  while (cursor < gap)
    *--gap_end = *--gap;
  while (gap_end < cursor)
    *gap++ = *gap_end++;
  point = pos(gap_end);
}

/* Offset of the start of the line containing `offset`. */
int line_start(int offset) {
  char *t;
  while (buffer < (t = ptr(--offset)) && *t != '\n')
    ;
  return buffer < t ? offset + 1 : 0;
}

/* Offset of the start of the line after the one containing `offset`. */
int next_line(int offset) {
  char *t;
  while ((t = ptr(offset++)) < buffer_end && *t != '\n')
    ;
  return t < buffer_end ? offset : end_offset();
}

/* From `offset`, step along the line until screen column `column`. */
int advance_to_column(int offset, int column) {
  char *t;
  int col = 0;
  while ((t = ptr(offset)) < buffer_end && *t != '\n' && col < column) {
    col += *t == '\t' ? TAB_STOP(col) : 1;
    ++offset;
  }
  return offset;
}

void cursor_left(void) {
  if (0 < point)
    --point;
}

void cursor_right(void) {
  if (point < end_offset())
    ++point;
}

void cursor_up(void) {
  point = advance_to_column(line_start(line_start(point) - 1), cursor_col);
}

void cursor_down(void) {
  point = advance_to_column(next_line(point), cursor_col);
}

void word_left(void) {
  char *t;
  while (!isspace(*(t = ptr(point))) && buffer < t)
    --point;
  while (isspace(*(t = ptr(point))) && buffer < t)
    --point;
}

void word_right(void) {
  char *t;
  while (!isspace(*(t = ptr(point))) && t < buffer_end)
    ++point;
  while (isspace(*(t = ptr(point))) && t < buffer_end)
    ++point;
}

void line_begin(void) { point = line_start(point); }

void line_end(void) {
  point = next_line(point);
  cursor_left();
}

void page_down(void) {
  page_start = point = line_start(page_end - 1);
  while (0 < cursor_row--)
    cursor_down();
  page_end = end_offset();
}

void page_up(void) {
  int count = screen_lines;
  while (0 < --count) {
    page_start = line_start(page_start - 1);
    cursor_up();
  }
}

void file_top(void) { point = 0; }

void file_bottom(void) { page_end = point = end_offset(); }

void display(void);

/* Insert typed characters until form-feed (ctrl-L) is pressed. */
void insert_mode(void) {
  int key;
  move_gap();
  while ((key = getch()) != '\f') {
    if (key != '\b') {
      if (gap != gap_end)
        *gap++ = key == '\r' ? '\n' : key;
    } else if (buffer < gap) {
      --gap;
    }
    point = pos(gap_end);
    display();
  }
}

/* Delete the character under the cursor by growing the gap. */
void delete_char(void) {
  move_gap();
  if (gap_end < buffer_end) {
    ++gap_end;
    point = pos(gap_end);
  }
}

/* Move the gap to the very start so the whole file is one block, save it. */
void write_file(void) {
  int fd;
  int saved = point;
  point = 0;
  move_gap();
  fd = creat(filename, MODE);
  write(fd, gap_end, (int)(buffer_end - gap_end));
  close(fd);
  point = saved;
}

void refresh_screen(void) {
  clear();
  display();
}

void quit(void) { done = 1; }

/* Redraw the visible page and place the cursor. */
void display(void) {
  int row, col, offset;
  char *t;

  /* cursor above the page: start the page at the cursor's line */
  if (point < page_start)
    page_start = line_start(point);

  /* cursor below the page: put the cursor's line near the bottom */
  if (page_end <= point) {
    int count;
    page_start = next_line(point);
    count = page_start != end_offset() ? screen_lines : screen_lines - 2;
    while (0 < count--)
      page_start = line_start(page_start - 1);
  }

  move(0, 0);
  row = col = 0;
  offset = page_start;
  while (1) {
    if (offset == point) {
      cursor_row = row;
      cursor_col = col;
    }
    t = ptr(offset);
    if (screen_lines <= row || buffer_end <= t)
      break;
    if (*t != '\r') {
      addch(*t);
      col += *t == '\t' ? TAB_STOP(col) : 1;
    }
    if (*t == '\n' || COLS <= col) {
      ++row;
      col = 0;
    }
    ++offset;
  }
  page_end = offset;

  clrtobot();
  if (++row < screen_lines)
    mvaddstr(row, 0, "<< EOF >>");
  move(cursor_row, cursor_col);
  refresh();
}

int main(int argc, char **argv) {
  int fd;

  gap_end = buffer_end = buffer + BUF;
  if (argc < 2)
    return 2;

  initscr();
  screen_lines = LINES;
  raw();
  noecho();
  idlok(stdscr, 1);

  filename = argv[1];
  if (0 < (fd = open(filename, 0))) {
    gap += read(fd, buffer, BUF);
    if (gap < buffer)
      gap = buffer;
    close(fd);
  }
  file_top();

  while (!done) {
    display();
    switch (getch()) {
    case 'h': cursor_left(); break;
    case 'j': cursor_down(); break;
    case 'k': cursor_up(); break;
    case 'l': cursor_right(); break;
    case 'H': word_left(); break;
    case 'J': page_down(); break;
    case 'K': page_up(); break;
    case 'L': word_right(); break;
    case '[': line_begin(); break;
    case ']': line_end(); break;
    case 't': file_top(); break;
    case 'b': file_bottom(); break;
    case 'i': insert_mode(); break;
    case 'x': delete_char(); break;
    case 'W': write_file(); break;
    case 'R': refresh_screen(); break;
    case 'Q': quit(); break;
    default: move_gap(); break;
    }
  }
  endwin();
  return 0;
}
