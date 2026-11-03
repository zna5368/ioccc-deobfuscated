#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>

/* size of the stored spreadsheet: SHEET_SIZE columns by SHEET_SIZE rows */
#define SHEET_SIZE 40
/* maximum length of the formula in one cell */
#define FORMULA_LENGTH 98
/* number of bytes in a spreadsheet file */
#define SHEET_BYTES ((FORMULA_LENGTH + 1) * SHEET_SIZE * SHEET_SIZE)
/* only the columns A to Z can be used */
#define COLUMN_LIMIT 26

/* dimensions of a cell on screen, the margins are one cell high */
#define CELL_WIDTH 80
#define CELL_HEIGHT 20
#define MARGIN 20
/* indentation of the text inside a cell */
#define TEXT_X 10
#define TEXT_Y 5
/* number of rows visible at one time */
#define VISIBLE_ROWS 23
/* one less than the number of columns visible at one time */
#define VISIBLE_COLUMNS 7
/* window size */
#define WINDOW_WIDTH (((VISIBLE_COLUMNS + 1) * CELL_WIDTH) + MARGIN)
#define WINDOW_HEIGHT ((VISIBLE_ROWS * CELL_HEIGHT) + MARGIN + MARGIN)

/* x and y offsets of the cell currently being drawn relative to the view */
#define DRAW_X (draw_column - first_column)
#define DRAW_Y (draw_row - first_row)

int key, formula_length;
/* cell being drawn by the main loop, charts are drawn relative to it */
int draw_column, draw_row;
/* currently selected cell */
int cursor_column, cursor_row;
/* first visible column and the row just above the first visible row */
int first_column, first_row;

/* the spreadsheet: one formula string per cell, indexed [column][row] */
char sheet[SHEET_SIZE][SHEET_SIZE][FORMULA_LENGTH + 1];
/* text shown for a cell: either its formatted value or its formula */
char text[FORMULA_LENGTH];

Window window;
GC gc;
XEvent event;
Display *display;

/* draw a histogram of the stack values, the tallest bar is "highest" */
void draw_histogram(double *stack, int count, double highest) {
  int bar, height;

  for (bar = count - 1; bar >= 0; bar--) {
    height = CELL_WIDTH * stack[bar] / highest;
    XFillRectangle(display, window, gc, (DRAW_X + bar) * CELL_WIDTH + MARGIN,
                   DRAW_Y * CELL_HEIGHT + CELL_HEIGHT - height, CELL_WIDTH / 2,
                   height);
  }
}

/* draw a pie chart of the stack values with alternating black/white slices */
void draw_pie(double *stack, int count, double total) {
  int slice;
  double start = 0;

  for (slice = count - 1; slice >= 0; slice--) {
    if (slice % 2)
      XSetForeground(display, gc, WhitePixel(display, 0));
    XFillArc(display, window, gc, DRAW_X * CELL_WIDTH - CELL_WIDTH,
             (DRAW_Y + 2) * CELL_HEIGHT, CELL_WIDTH * 3, CELL_WIDTH * 3,
             360 * start / total * 64, 360 * stack[slice] / total * 64);
    XSetForeground(display, gc, BlackPixel(display, 0));
    XDrawArc(display, window, gc, DRAW_X * CELL_WIDTH - CELL_WIDTH,
             (DRAW_Y + 2) * CELL_HEIGHT, CELL_WIDTH * 3, CELL_WIDTH * 3,
             360 * start / total * 64, 360 * stack[slice] / total * 64);
    start += stack[slice];
  }
}

/*
 * Evaluate the postfix formula of a cell.  On success the formatted value
 * is put into text and 0 is returned.  On failure (syntax error, reference
 * error, too deep recursion or a chart operator) the formula itself is put
 * into text and 1 is returned.
 */
int evaluate_cell(int column, int row, int depth) {
  char *formula = sheet[column][row], *end;
  int count = 0, i;
  double highest = 0, total = 0, stack[FORMULA_LENGTH];

  while (depth < SHEET_SIZE && *formula) {
    stack[count] = strtod(formula, &end);

    /* remember the sum (pie chart) or the maximum (histogram) of the stack */
    for (i = 0; i < count; i++) {
      if (*formula == '@')
        total += stack[i];
      if (*formula == '!')
        highest = stack[i] > highest ? stack[i] : highest;
      else
        highest = 0;
    }

    if (isupper(*formula) && isdigit(formula[1])) {
      /* cell reference such as B12 */
      if (evaluate_cell(*formula - 'A', atoi(formula + 1), depth + 1))
        goto error;
      stack[count++] = atof(text);
      do
        formula++;
      while (isdigit(*formula));
    } else if (formula != end) {
      /* number */
      count++;
      formula = end;
    } else {
      if (count > 1 && *formula == '=') {
        /* drop the top of the stack */
        count--;
      } else if (count > 1 && *formula == '+') {
        count--;
        stack[count - 1] += stack[count];
      } else if (count > 1 && *formula == '-') {
        count--;
        stack[count - 1] -= stack[count];
      } else if (count > 1 && *formula == '*') {
        count--;
        stack[count - 1] *= stack[count];
      } else if (count > 1 && *formula == '/') {
        count--;
        stack[count - 1] /= stack[count];
      } else if (*formula != ' ')
        goto error; /* also reached by the chart operators '@' and '!' */
      formula++;
    }
  }

  /* exactly one result is the value of the cell */
  if (count == 1) {
    sprintf(text, "%10.2f", stack[0]);
    return 0;
  }

error:
  if (highest)
    draw_histogram(stack, count, highest);
  else if (total)
    draw_pie(stack, count, total);
  strcpy(text, sheet[column][row]);
  return 1;
}

int main() {
  KeySym keysym;
  char *cell;

  read(0, sheet, SHEET_BYTES);
  display = XOpenDisplay(0);
  gc = XCreateGC(display, window = RootWindow(display, 0), 0, 0);
  window = XCreateSimpleWindow(display, window, 0, 0, WINDOW_WIDTH,
                               WINDOW_HEIGHT, 0, 0, WhitePixel(display, 0));
  XSelectInput(display, window, ButtonPressMask | KeyPressMask | ExposureMask);
  XMapWindow(display, window);

  for (;;) {
    cell = sheet[cursor_column][cursor_row];

    XNextEvent(display, &event);
    XLookupString(&event.xkey, text, 1, &keysym, 0);
    key = keysym;

    /* edit the formula of the selected cell or move the cursor */
    if ((key & 96) && key < 128)
      cell[formula_length++] = key;
    else if (key == XK_Up)
      cursor_row--;
    else if (key == XK_Down)
      cursor_row++;
    else if (key == XK_Right)
      cursor_column++;
    else if (key == XK_BackSpace)
      cell[--formula_length] = 0;
    else if (key == XK_Left && cursor_column)
      cursor_column--;

    if (event.type == ButtonPress) {
      cursor_column = first_column + (event.xbutton.x - MARGIN) / CELL_WIDTH;
      cursor_row = first_row + event.xbutton.y / CELL_HEIGHT - 1;
    }

    /* wrap around and scroll the view horizontally */
    cursor_column %= COLUMN_LIMIT;
    if (cursor_column > first_column + VISIBLE_COLUMNS)
      first_column++;
    while (cursor_column < first_column)
      first_column--;

    cursor_row %= SHEET_SIZE;
    if (cursor_row < 1)
      cursor_row = 1;

    /* quit and save the spreadsheet to standard output */
    if (key == XK_Escape)
      exit(write(1, sheet, SHEET_BYTES));

    /* scroll the view vertically */
    if (cursor_row > first_row + VISIBLE_ROWS)
      first_row++;
    while (cursor_row <= first_row)
      first_row--;

    XClearWindow(display, window);
    for (draw_row = first_row + 1; draw_row <= first_row + VISIBLE_ROWS;
         draw_row++) {
      /* row number in the left margin */
      sprintf(text, "%3d", draw_row);
      XDrawString(display, window, gc, 0, (DRAW_Y + 2) * CELL_HEIGHT - TEXT_Y,
                  text, 3);

      for (draw_column = first_column;
           draw_column <= first_column + VISIBLE_COLUMNS; draw_column++) {
        XDrawRectangle(display, window, gc, DRAW_X * CELL_WIDTH + MARGIN,
                       DRAW_Y * CELL_HEIGHT + CELL_HEIGHT, CELL_WIDTH,
                       CELL_HEIGHT);
        evaluate_cell(draw_column, draw_row, 0);
        XDrawString(display, window, gc, DRAW_X * CELL_WIDTH + MARGIN + TEXT_X,
                    (DRAW_Y + 2) * CELL_HEIGHT - TEXT_Y, text, strlen(text));

        /* column letter in the top margin */
        *text = 'A' + draw_column;
        XDrawString(display, window, gc, DRAW_X * CELL_WIDTH + MARGIN + TEXT_X,
                    CELL_HEIGHT * 2 - TEXT_Y, text, 1);
      }
    }

    /* highlight the selected cell */
    XDrawRectangle(display, window, gc,
                   (cursor_column - first_column) * CELL_WIDTH + MARGIN + 1,
                   (cursor_row - first_row) * CELL_HEIGHT + CELL_HEIGHT + 1,
                   CELL_WIDTH - 2, CELL_HEIGHT - 2);

    /* formula of the selected cell above the column letters */
    cell = sheet[cursor_column][cursor_row];
    formula_length = strlen(cell);
    XDrawString(display, window, gc, MARGIN, MARGIN, cell, formula_length);
  }
}
