/*
 * The original has no #include lines. The Makefile force includes stdio.h,
 * unistd.h, stdlib.h, signal.h and sys/time.h on the command line.
 */

/*
 * The board is a 12 x 23 grid stored in one array. Columns 0 and 11 are
 * walls, rows 21 and 22 are the floor and row 0 is above the screen. Moving
 * one cell left/right is -1/+1 and one cell up/down is -12/+12.
 */
#define WIDTH 12
#define HEIGHT 23
#define BOARD_SIZE (WIDTH * HEIGHT) /* 276 */
#define FLOOR (21 * WIDTH)          /* 252: first floor cell */
#define FIRST_DRAWN WIDTH           /* row 0 is never drawn */
#define DRAWN_END (FLOOR + WIDTH)   /* 264: rows 1 to 21 are drawn */
#define LAST_DRAWN (DRAWN_END - 1)  /* 263: bottom right wall cell */
#define LAST_COLUMN 10              /* last column inside the walls */
#define SPAWN (WIDTH + 5)           /* 17: row 1, column 5 */

/* cell values are also the video attribute used to draw them */
#define EMPTY 0 /* normal video */
#define SOLID 7 /* reverse video */

#define SCREEN_LEFT 28 /* terminal column of board column 0 */

#define LEFT (-1)
#define RIGHT 1
#define UP (-WIDTH)
#define DOWN WIDTH

/* sigmask() for the old BSD sigblock() and sigsetmask() calls */
#define ALARM_MASK (1 << (SIGALRM - 1))

/* the six characters of the command string */
enum { KEY_LEFT, KEY_ROTATE, KEY_RIGHT, KEY_DROP, KEY_PAUSE, KEY_QUIT };

/*
 * A shape is the centre cell plus three cells at offsets from it, and the
 * shape it turns into when rotated. The first seven are the start shapes.
 */
/* clang-format off */
enum {
  Z, S, T, O, L, J, I, /* start shapes */
  Z_90, S_90, T_90, T_180, T_270, L_90, L_180, L_270,
  J_90, J_180, J_270, I_90
};
/* clang-format on */

struct shape {
  long next; /* shape after rotating */
  long offset[3];
};

struct shape shapes[] = {
    /* Z     */ {Z_90, {UP + LEFT, UP, RIGHT}},
    /* S     */ {S_90, {UP + RIGHT, UP, LEFT}},
    /* T     */ {T_90, {LEFT, RIGHT, DOWN}},
    /* O     */ {O, {UP + LEFT, UP, LEFT}},
    /* L     */ {L_90, {LEFT, DOWN + LEFT, RIGHT}},
    /* J     */ {J_90, {LEFT, DOWN + RIGHT, RIGHT}},
    /* I     */ {I_90, {LEFT, RIGHT, 2 * RIGHT}},
    /* Z_90  */ {Z, {UP, LEFT, DOWN + LEFT}},
    /* S_90  */ {S, {UP, RIGHT, DOWN + RIGHT}},
    /* T_90  */ {T_180, {UP, RIGHT, DOWN}},
    /* T_180 */ {T_270, {UP, LEFT, RIGHT}},
    /* T_270 */ {T, {UP, LEFT, DOWN}},
    /* L_90  */ {L_180, {UP, DOWN, DOWN + RIGHT}},
    /* L_180 */ {L_270, {UP + RIGHT, LEFT, RIGHT}},
    /* L_270 */ {L, {UP + LEFT, UP, DOWN}},
    /* J_90  */ {J_180, {UP + RIGHT, UP, DOWN}},
    /* J_180 */ {J_270, {UP + LEFT, RIGHT, LEFT}},
    /* J_270 */ {J, {UP, DOWN, DOWN + LEFT}},
    /* I_90  */ {I, {UP, DOWN, 2 * DOWN}},
};

struct itimerval timer; /* one shot timer, re-armed on every tick */
FILE *high_scores;
long key; /* last getchar(), EOF when a tick interrupted it */
long score;
long cursor;    /* last cell drawn, so the cursor is just after it */
long attribute; /* video attribute the terminal is using */
long board[BOARD_SIZE];
long screen[BOARD_SIZE]; /* what the terminal is showing */
struct shape *piece;
long position = SPAWN; /* board cell of the centre of the piece */

/* SIGALRM handler: speed up a little and set the timer for the next drop */
void tick() {
  timer.it_value.tv_usec -= timer.it_value.tv_usec / 3000;
  setitimer(ITIMER_REAL, &timer, 0);
}

/* draw the cells that changed, then wait for a key press or a tick */
void refresh(void) {
  long cell, value;

  for (cell = FIRST_DRAWN; cell < DRAWN_END; cell++) {
    value = board[cell];
    if (value != screen[cell]) {
      screen[cell] = value;
      cursor++;
      if (cell != cursor || cell % WIDTH == 0)
        printf("\033[%ld;%ldH", cell / WIDTH, cell % WIDTH * 2 + SCREEN_LEFT);
      cursor = cell;
      if (value != attribute)
        printf("\033[%ldm", value);
      printf("  ");
      attribute = value;
    }
  }
  /* storing the key in the bottom right wall cell makes it differ from the
   * board, so it is redrawn every time. That parks the cursor at the bottom
   * right and always leaves the terminal in reverse video */
  screen[LAST_DRAWN] = key = getchar();
}

/* can the current piece be placed with its centre at a cell */
int fits(long center) {
  int i;

  if (board[center])
    return 0;
  for (i = 0; i < 3; i++)
    if (board[center + piece->offset[i]])
      return 0;
  return 1;
}

/* write the current piece onto the board */
void paint(long value) {
  int i;

  board[position] = value;
  for (i = 0; i < 3; i++)
    board[position + piece->offset[i]] = value;
}

int row_is_full(long row) {
  long cell;

  for (cell = row + 1; cell <= row + LAST_COLUMN; cell++)
    if (!board[cell])
      return 0;
  return 1;
}

/* blank a full row, show it, then move everything above it down */
void remove_row(long row) {
  long cell;

  for (cell = row + LAST_COLUMN; cell > row; cell--)
    board[cell] = EMPTY;
  refresh();
  for (cell = row - 1; cell != 0; cell--)
    board[cell + DOWN] = board[cell];
  refresh();
}

void remove_full_rows(void) {
  long row = 0;

  while (row < FLOOR) {
    if (row_is_full(row)) {
      remove_row(row);
      row = 0; /* start again from the top */
    } else
      row += WIDTH;
  }
}

void new_piece(void) { piece = &shapes[rand() % 7]; }

int main(int argc, char **argv) {
  struct sigaction action = {0};
  struct shape *before;
  char *keys;
  long cell;
  long saved_mask;

  action.sa_handler = tick; /* no SA_RESTART: a tick interrupts getchar() */
  timer.it_value.tv_usec = 1000000 / (argc > 1 ? atoi(argv[1]) : 2);
  keys = argc > 2 ? argv[2] : "jkl pq";

  for (cell = 0; cell < BOARD_SIZE; cell++) {
    int column = cell % WIDTH;
    board[cell] =
        cell >= FLOOR || column == 0 || column == WIDTH - 1 ? SOLID : EMPTY;
  }

  srand(getpid());
  system("stty cbreak -echo");
  sigaction(SIGALRM, &action, 0);
  tick();
  puts("\033[H\033[J");

  for (new_piece();;) {
    if (key < 0) {
      /* EOF from a tick: move down, or land and get a new piece */
      if (fits(position + DOWN))
        position += DOWN;
      else {
        paint(SOLID);
        score++;
        remove_full_rows();
        new_piece();
        position = SPAWN;
        if (!fits(position))
          key = keys[KEY_QUIT]; /* game over */
      }
    }

    if (key == keys[KEY_LEFT] && !fits(--position))
      position++;

    if (key == keys[KEY_ROTATE]) {
      before = piece;
      piece = &shapes[piece->next];
      if (!fits(position))
        piece = before;
    }

    if (key == keys[KEY_RIGHT] && !fits(++position))
      position--;

    if (key == keys[KEY_DROP])
      while (fits(position + DOWN)) {
        position += DOWN;
        score++;
      }

    if (key == keys[KEY_PAUSE] || key == keys[KEY_QUIT]) {
      saved_mask = sigblock(ALARM_MASK);
      printf("\033[H\033[J\033[0m%ld\n", score);
      if (key == keys[KEY_QUIT])
        break;
      for (cell = 0; cell < DRAWN_END; cell++)
        screen[cell] = EMPTY; /* forget the screen so it is all redrawn */
      while (getchar() != keys[KEY_PAUSE])
        ;
      puts("\033[H\033[J\033[7m");
      sigsetmask(saved_mask);
    }

    paint(SOLID);
    refresh();
    paint(EMPTY);
  }

  system("stty cbreak echo");
  high_scores = popen("cat - HI 2>/dev/null|sort -rn|head -20>/tmp/$$;"
                      "mv /tmp/$$ HI;cat HI",
                      "w");
  fprintf(high_scores, "%4ld by %s\n", score, getlogin());
  pclose(high_scores);
}
