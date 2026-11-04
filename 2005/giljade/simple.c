/*
 * 2005/giljade - sliding block puzzle solver that prints every step of the
 * solution as a copy of its own source code laid out in the shape of the board.
 *
 * No #include lines: the Makefile force-includes stdio.h, string.h, unistd.h
 * and stdlib.h, just like for the original.
 */

/* The original opens __FILE__ (its own source).  The preprocessed version
 * names the original file so the output stays the same. */
#define SOURCE_FILE "giljade.c"

/* ------------------------------------------------------------------------ */
/* Puzzle                                                                   */
/* ------------------------------------------------------------------------ */

/*
 * The board is 4 columns wide and 5 rows tall.  Cell (row, col) is bit
 * row * 4 + col of an int.  Row 4 is printed at the top of the screen.
 * A piece is stored as the bit of its lowest row / leftmost column.
 */
enum { BIG, TALL, WIDE, SMALL }; /* 2x2, 1 wide x 2 high, 2 wide x 1 high, 1x1 */

#define ROWS_1_TO_4 0xFFFF0 /* cells that have a row below them   */
#define ROWS_0_TO_3 0x0FFFF /* cells that have a row above them   */
#define COLS_0_TO_2 0x77777 /* cells that have a column right     */
#define COLS_1_TO_3 0xEEEEE /* cells that have a column left      */

#define GOAL 0x2000 /* big piece at row 3, column 1 (top center on screen) */

#define BUCKETS (1 << 20)   /* one hash bucket per possible SMALL mask */
#define MAX_POSITIONS 200000

typedef struct {
  int piece[4]; /* bit masks of BIG, TALL, WIDE and SMALL pieces */
  int next;     /* next position in the same hash bucket (0 = none) */
  int parent;   /* position this one was reached from (0 = none) */
} Position;

Position positions[MAX_POSITIONS]; /* breadth first search queue, [0] unused */
int bucket[BUCKETS];
int position_count; /* positions[position_count] is the next free entry */

/* Keep the candidate in positions[position_count] unless it was seen before */
void add_candidate(void) {
  Position *candidate = &positions[position_count];
  int *link = &bucket[candidate->piece[SMALL]];

  while (*link && memcmp(candidate->piece, positions[*link].piece, 16))
    link = &positions[*link].next;
  if (!*link) {
    *link = position_count;
    position_count++;
  }
}

/* Every piece of one kind whose bit is set in "movers" is moved by "shift"
 * bits (positive = left shift), each move making a new candidate position. */
void slide(int from, int kind, int movers, int shift) {
  int bit, i;
  Position *candidate;

  while (movers) {
    bit = movers & movers - 1 ^ movers; /* lowest set bit */
    movers = movers ^ bit;

    candidate = &positions[position_count];
    for (i = 0; i < 4; i++)
      candidate->piece[i] = positions[from].piece[i];
    candidate->next = 0;
    candidate->parent = from;
    candidate->piece[kind] ^= bit ^ (shift > 0 ? bit << shift : bit >> -shift);
    add_candidate();
  }
}

/* Bit mask of the empty cells (bits above the board are also set) */
int empty_cells(Position *p) {
  int big = p->piece[BIG] << 4 | p->piece[BIG];

  return ~(big | big << 1 | p->piece[SMALL] | p->piece[TALL] |
           p->piece[TALL] << 4 | p->piece[WIDE] | p->piece[WIDE] << 1);
}

void expand(int from) {
  Position *p = &positions[from];
  int empty = empty_cells(p);
  int room;

  /* up: room has a bit for every cell whose upper neighbour is empty */
  room = (empty & ROWS_1_TO_4) >> 4;
  slide(from, SMALL, room & p->piece[SMALL], 4);
  slide(from, TALL, room >> 4 & p->piece[TALL], 4);
  room = room >> 1 & room; /* the cell to the right also has room */
  slide(from, WIDE, room & p->piece[WIDE], 4);
  slide(from, BIG, room >> 4 & p->piece[BIG], 4);

  /* left */
  room = (empty & COLS_0_TO_2) << 1;
  slide(from, SMALL, room & p->piece[SMALL], -1);
  slide(from, WIDE, room & p->piece[WIDE], -1);
  room = room >> 4 & room; /* the cell above also has room */
  slide(from, TALL, room & p->piece[TALL], -1);
  slide(from, BIG, room & p->piece[BIG], -1);

  /* right */
  room = (empty & COLS_1_TO_3) >> 1;
  slide(from, SMALL, room & p->piece[SMALL], 1);
  slide(from, WIDE, room >> 1 & p->piece[WIDE], 1);
  room = room >> 4 & room;
  slide(from, TALL, room & p->piece[TALL], 1);
  slide(from, BIG, room >> 1 & p->piece[BIG], 1);

  /* down */
  room = (empty & ROWS_0_TO_3) << 4;
  slide(from, SMALL, room & p->piece[SMALL], -4);
  slide(from, TALL, room & p->piece[TALL], -4);
  room = room >> 1 & room;
  slide(from, WIDE, room & p->piece[WIDE], -4);
  slide(from, BIG, room & p->piece[BIG], -4);
}

/* ------------------------------------------------------------------------ */
/* Printing                                                                 */
/* ------------------------------------------------------------------------ */

char tokens[9999]; /* the source code with all whitespace removed */
int is_word[9999]; /* tokens[i] is a letter or digit */

/*
 * read_source() puts a MARKER in tokens[] where each section ends:
 *
 *   #define...#define...(0))  MARKER  ;(comment with the self test command)
 *   MARKER  ;char*A=,*_,...rest of the program...
 *
 * The '#' of every "#define" also acts as a marker.
 */
#define MARKER '#'

/* What the next character slot of the board will hold.  A "slot" is a
 * character position that lies on a piece; empty cells and the borders
 * between pieces are printed as spaces (a "gap"). */
enum {
  STRING_PREFIX,   /* ";;char*A=" and then the opening quote */
  STRING,          /* the inside of the string literal holding the source */
  STRING_SPLIT,    /* string was closed at the end of a line, reopen it */
  CODE,            /* the next source token character */
  GAP_COMMENT,     /* a comment was opened before a gap, write its '*' */
  GAP_COMMENT_END, /* write the '/' that closes the gap comment */
  FILLER_OPEN,     /* a filler comment starts here, write '/' */
  FILLER_STAR,     /* '/' was written, write '*' */
  FILLER           /* write scrambled source characters */
};

/* What happens at the next newline / next slot */
enum {
  NOTHING,
  CLOSE_FILLER, /* close the filler comment at the end of this line */
  NEW_SECTION   /* the next slot starts a "#define" or the main code */
};

#define FRAME_LINES 74
#define FRAME_COLUMNS 72
#define CELL_HEIGHT 14
#define CELL_WIDTH 18
#define LAST_LINE 13499 /* 180 frames of 75 lines */

/* Compile every printed program found in "file", one per frame */
void self_test(char *command_format, char *file) {
  char command[9999];
  int line = 2, end;

  while (line < LAST_LINE) {
    end = line + 75;
    /* echo Line %d;sed -n -e %d,%dp %s |sed -e 's,intZ,int Z,g'>c.c;make c; rm -f c */
    sprintf(command, command_format, line, line, end, file);
    line = end;
    if (system(command))
      break;
  }
  exit(line < LAST_LINE);
}

void print_frame(Position *p, int argc, char **argv) {
  char frame[9999];
  char *out = frame;
  char *last_slot;     /* last character written into a slot of a piece */
  char *comment_start; /* where the current filler comment started */
  char *filler = tokens;
  char *scan, ch;
  int big = p->piece[BIG];
  int empty = empty_cells(p);
  /* draw the line above a cell unless it is the lower half of a piece */
  int top_edge = ~(p->piece[TALL] | big | big << 1);
  /* draw the column right of a cell unless it is the left half of a piece */
  int right_edge = ~(p->piece[WIDE] | big | big << 4);
  int line, column, cell, kind, back;
  int d = 1; /* index of the next character of tokens[] */
  int string_resume;
  int state = FILLER;
  int at_newline = NEW_SECTION;
  int in_macros = 0; /* lines need a backslash continuation */

  for (line = FRAME_LINES; line > 0; line--) {
    for (column = 0; column < FRAME_COLUMNS; column++) {
      cell = 1 << (line - 1) / CELL_HEIGHT * 4 + (column - 1) / CELL_WIDTH;
      if (column == 0)
        kind = '\n';
      else if ((column % CELL_WIDTH == 0 && right_edge & cell) ||
               empty & cell || (line % CELL_HEIGHT == 0 && top_edge & cell))
        kind = ' ';
      else
        kind = 0; /* a character of the program goes here */

      if (state <= STRING_SPLIT) {
        /* inside: char*A="..." */
        if (kind == 0) {
          if (state == STRING_PREFIX) {
            if (d < string_resume)
              *out++ = tokens[d++];
            else {
              state = STRING;
              d = 0;
              *out++ = '"';
            }
          } else if (state == STRING) {
            if (tokens[d])
              *out++ = tokens[d++];
            else {
              /* whole source is in the string, continue the code after A= */
              d = string_resume;
              state = CODE;
              *out++ = '"';
            }
          } else {
            state = STRING;
            *out++ = '"';
          }
        } else if (kind == '\n' && state != STRING_PREFIX) {
          /* replace the last character of the line by a closing quote */
          if (state != STRING_SPLIT)
            d--;
          *last_slot = '"';
          state = STRING_SPLIT;
        }
      } else if (state <= GAP_COMMENT_END) {
        /* inside the program code */
        if (kind) {
          if (kind == '\n' && in_macros) {
            /* the last character of the line becomes a backslash */
            out = last_slot;
            if (state != GAP_COMMENT)
              d--;
            else if (out[-1] == '/') {
              out[-1] = tokens[d++];
              state = CODE;
            }
          }
          /* a gap must not split a word or a two character operator, so
           * back up and hide those characters in a comment instead */
          back = 0;
          if (is_word[d] && is_word[d - 1])
            for (; is_word[d - 1]; ++back)
              d--;
          if (d && tokens[d] == tokens[d - 1] && !is_word[d] &&
              tokens[d] != '(' && tokens[d] != ')' && tokens[d] != '}') {
            back = 1;
            d--;
          }
          if (back == 1) { /* need two characters for the comment opener */
            back = 2;
            d--;
          }
          if (back) {
            out[-back] = '/';
            out[1 - back] = '*';
            state = GAP_COMMENT;
          }
          if (kind == '\n' && in_macros)
            *out++ = '\\';
        } else if (state == GAP_COMMENT) {
          *out++ = '*';
          state = GAP_COMMENT_END;
        } else if (state == GAP_COMMENT_END) {
          *out++ = '/';
          state = CODE;
        } else {
          ch = *out++ = tokens[d++];
          if (ch == MARKER || ch == '\0') {
            /* end of a #define or of the whole program: the rest of the
             * line (or frame) becomes a comment */
            if (ch == MARKER)
              at_newline = CLOSE_FILLER;
            state = FILLER_STAR;
            last_slot = comment_start = out - 1;
            *last_slot = '/';
          }
        }
      } else {
        /* inside a filler comment */
        if (kind) {
          if (state == FILLER_STAR) {
            /* no room for the '*', so turn the '/' into a ';' */
            state = FILLER_OPEN;
            out[-1] = ';';
          }
        } else if (at_newline == NEW_SECTION) {
          at_newline = NOTHING;
          if (tokens[d] == 'd') {
            /* tokens[d] starts "define" */
            in_macros = 1;
            state = CODE;
            *out++ = '#';
          } else {
            /* the self test command is between the two markers */
            in_macros = 0;
            state = STRING_PREFIX;
            *out++ = 0;
            filler = &tokens[d];
            for (scan = filler; *scan != MARKER; ++scan)
              if ((*scan == '/' || *scan == '*') && argc != 1)
                *scan = ' ';
            d = d + scan - filler;
            if (argc != 1) {
              *scan = 0;
              self_test(filler + 1, argv[1]);
            }
            string_resume = d + 9; /* index after "#;char*A=" */
            out[-1] = tokens[++d];
          }
        } else if (state == FILLER_OPEN) {
          *out++ = '/';
          state = FILLER_STAR;
        } else if (state == FILLER_STAR) {
          *out++ = '*';
          state = FILLER;
        } else {
          /* clearing the lowest bit turns every '/' into '.' so the comment
           * can never be closed early */
          *out++ = 126 & *++filler;
        }

        if (at_newline == CLOSE_FILLER && kind == '\n') {
          if (last_slot - comment_start < 3) {
            /* too short for a comment, use empty statements instead */
            *(comment_start < last_slot ? last_slot - 1 : last_slot) = ';';
            *comment_start = ';';
            *last_slot = ';';
          } else {
            *last_slot = '/';
            last_slot[-1] = '*';
          }
          at_newline = NEW_SECTION;
        }
      }

      if (kind)
        *out++ = kind;
      else
        last_slot = out - 1;
    }
  }
  last_slot[-1] = '*';
  *last_slot = '/';
  *out++ = '\n';

  for (scan = frame; putchar(*scan) && out > ++scan;)
    ;
}

/* Read the source and squeeze it into tokens[] */
void read_source(void) {
  char source[3334];
  char *p;
  int pairs = 0, count = 0;

  source[fread(source, 1, 3333, fopen(SOURCE_FILE, "r"))] = 0;

  /* There are four ";;" in the source.  The first three get a MARKER, the
   * second also loses the 0 in "char*A=0" and everything from the third up to
   * the fourth (the code that reads the file) is erased. */
  for (p = source + 1; *p; p++) {
    if (p[0] == ';' && p[1] == ';') {
      pairs++;
      if (pairs != 4)
        *p = MARKER;
      if (pairs == 2)
        p[9] = ' ';
    }
    if (pairs == 3)
      *p = ' ';
  }

  for (p = source; *p; p++)
    if (*p != '\n' && *p != ' ' && *p != '\\') {
      tokens[count] = *p;
      is_word[count] = isalnum(*p) != 0;
      count++;
    }
}

int main(int argc, char **argv) {
  static int path[MAX_POSITIONS];
  int current, length = 0;

  read_source();

  positions[1].piece[BIG] = 0x40;      /* row 1, column 2 */
  positions[1].piece[TALL] = 0x121;    /* rows 0, 1, 2 */
  positions[1].piece[WIDE] = 0x42000;  /* rows 3 and 4 */
  positions[1].piece[SMALL] = 0x800E;  /* row 0 and row 3 */
  position_count = 2;

  for (current = 1; current < position_count; current++) {
    expand(current);
    if (positions[current].piece[BIG] & GOAL) {
      for (; current; current = positions[current].parent)
        path[length++] = current;
      while (length > 0)
        print_frame(&positions[path[--length]], argc, argv);
      break;
    }
  }
  return 0;
}
