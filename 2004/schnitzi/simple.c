#include <stdio.h>

/*
 * Reads a sprite animation description on stdin and writes a string of vi
 * keystrokes on stdout.  Typed (or piped) into vi, the keystrokes draw every
 * frame of the animation by overwriting only the characters that changed.
 */

#define SCREEN_WIDTH 50
#define SCREEN_HEIGHT 20
#define MAX_SPRITES 9
#define MAX_VIEWS 4
#define MAX_SIZE 100
#define LINE_LENGTH 100

#define BORDER_LINE "=================================================="
#define BLANK_LINE "                                                  "

/* Is the point (x, y) inside the visible screen? */
int on_screen(float x, float y) {
  return x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT;
}

int main() {
  /* two screen buffers: the frame being drawn and the previous frame */
  char screen[2][SCREEN_HEIGHT][SCREEN_WIDTH + 1];
  char views[MAX_SPRITES][MAX_VIEWS][MAX_SIZE][MAX_SIZE];
  char line[LINE_LENGTH];

  int start[MAX_SPRITES], end[MAX_SPRITES];
  float pos_x[MAX_SPRITES], pos_y[MAX_SPRITES];
  float delta_x[MAX_SPRITES], delta_y[MAX_SPRITES];
  int cols[MAX_SPRITES], rows[MAX_SPRITES];
  int view_count[MAX_SPRITES], current_view[MAX_SPRITES];
  int sprite_count = 0;

  int duration, time_factor, time;
  int current = 0;
  int cursor_row, cursor_col, chars_written;
  int sprite, view, row, col;

  /*
   * Fill vi with a 20 line screen of '=' characters:
   * insert one line, ESC, yank it, paste it 19 times, go to line 1 column 0.
   */
  printf("i%s\033Y%s1G0", BORDER_LINE, "ppppppppppppppppppp");

  /* the first frame is blank, the "previous" frame is what vi now shows */
  for (row = 0; row < SCREEN_HEIGHT; row++) {
    sprintf(screen[1][row], "%s", BORDER_LINE);
    sprintf(screen[0][row], "%s", BLANK_LINE);
  }

  /* header: [duration] [timefactor] */
  scanf("%d %d\n", &duration, &time_factor);

  /* each sprite: a parameter line followed by its views */
  fgets(line, LINE_LENGTH, stdin);
  do {
    sscanf(line, "%d %d %f %f %f %f %d %d %d", &start[sprite_count],
           &end[sprite_count], &pos_x[sprite_count], &pos_y[sprite_count],
           &delta_x[sprite_count], &delta_y[sprite_count], &cols[sprite_count],
           &rows[sprite_count], &view_count[sprite_count]);
    current_view[sprite_count] = 0;

    view = 0;
    do {
      row = 0;
      do {
        char *text = views[sprite_count][view][row];
        int got_line = fgets(text, LINE_LENGTH, stdin) != NULL;
        /* cut the row to the sprite width (removes the newline) */
        text[cols[sprite_count]] = !got_line;
      } while (++row < rows[sprite_count]);
    } while (++view < view_count[sprite_count]);
    sprite_count++;
  } while (fgets(line, LINE_LENGTH, stdin));

  cursor_row = cursor_col = 0;
  time = 0;
  do {
    char(*frame)[SCREEN_WIDTH + 1] = screen[current];
    char(*previous)[SCREEN_WIDTH + 1] = screen[1 - current];

    /* clear the frame */
    for (row = 0; row < SCREEN_HEIGHT; row++)
      strncpy(frame[row], BLANK_LINE, SCREEN_WIDTH);

    /* draw every visible sprite; later sprites are drawn in front */
    for (sprite = 0; sprite < sprite_count; sprite++) {
      if (time < start[sprite] || time > end[sprite])
        continue;

      for (row = 0; row < rows[sprite]; row++)
        for (col = 0; col < cols[sprite]; col++)
          if (on_screen(pos_x[sprite] + col, pos_y[sprite] + row))
            frame[(int)pos_y[sprite] + row][(int)pos_x[sprite] + col] =
                views[sprite][current_view[sprite]][row][col];

      /* next view, next position */
      if (++current_view[sprite] == view_count[sprite])
        current_view[sprite] = 0;
      pos_x[sprite] += delta_x[sprite];
      pos_y[sprite] += delta_y[sprite];
    }

    /* emit vi commands that replace each changed character */
    chars_written = 0;
    for (row = 0; row < SCREEN_HEIGHT; row++) {
      for (col = 0; col < SCREEN_WIDTH; col++) {
        if (previous[row][col] == frame[row][col])
          continue;

        if (cursor_row != row) {
          /* go to line row+1, column 0 */
          printf("%dG0", row + 1);
          cursor_col = 0;
          chars_written += 4;
        }
        if (col && cursor_col > col) {
          /* go to column 0 then move right */
          printf("0%d ", col);
          chars_written += 4;
        }
        if (col && cursor_col < col && col - cursor_col > 1) {
          /* move right several columns */
          printf("%d ", col - cursor_col);
          chars_written += 3;
        }
        if (col && cursor_col < col && col - cursor_col == 1) {
          /* move right one column */
          printf(" ");
          chars_written++;
        }
        /* replace the character under the cursor */
        printf("r%c", frame[row][col]);
        chars_written += 2;
        cursor_col = col;
        cursor_row = row;
      }
    }

    /* return home, repeating it to waste time until time_factor is reached */
    do {
      printf("1G0");
      chars_written += 3;
      cursor_col = cursor_row = 0;
    } while (chars_written < time_factor);

    current = 1 - current;
  } while (++time <= duration);

  return !puts("");
}
