#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*
 * The Makefile builds ./prog with -DN=11, so that is the default here.
 * Build with -DN=5 for the alternate version (./prog.alt).
 */
#ifndef N
#define N 11
#endif

#define ROWS       (N - 1)        /* rows of the arena (0 .. N-2) */
#define EXIT_ROW   (N - 1)        /* reaching this row means escaping */

/* cell states */
#define UNKNOWN 0                 /* drawn as '.' */
#define SAFE    1                 /* drawn as '*' */
#define TRAP    2                 /* drawn as 'o' */

/* results returned by move_player() */
#define RESULT_NOTHING   0        /* key was not a move */
#define RESULT_MOVED     1        /* moved, nothing happened */
#define RESULT_TRAPPED   2        /* hit a trap, back to the start */
#define RESULT_LOST      3        /* trapped on the last attempt */
#define RESULT_WON_2     4        /* escaped on the 2nd attempt */
#define RESULT_WON_3     5        /* escaped on the 3rd attempt */
#define RESULT_QUIT      6

/*
 * NOTE: the order and the types of these globals are the same as in the
 * original.  With N = 11 the search below overflows a char and reads a
 * few bytes in front of queue[], so the memory layout has to be kept.
 */
typedef char grid_t[N * N - N];
grid_t grid,                      /* ROWS x N cells */
       queue;                     /* breadth first search queue */
signed char pivot_col,            /* column the computer keeps for itself */
            attempt = 5,          /* attempt number 1..3, bit 4 = game over */
            row, col,             /* player position, row -1 = outside */
            search_flag,          /* > 0 once the computer starts searching */
            grid_size;            /* number of cells, N * (N - 1) */
unsigned rng_state;

/* start a new game; the seed comes from the platform */
void reset_game(unsigned seed)
{
  for (char i = 0; i < (grid_size = ROWS * N); i++)
    grid[i] = UNKNOWN;

  search_flag = 0;
  row = -1;
  attempt = 1;
  rng_state = 1 + (rng_state - seed) * 69069;  /* linear congruential RNG */
  col = (rng_state / 2) % N;
  pivot_col = (rng_state + (rng_state & grid_size)) % N;
}

/* the state of the breadth first search */
struct search {
  char *here;                     /* the cell the player stepped on */
  char *probe;                    /* last cell looked at */
  char head, tail;                /* read and write index into queue[] */
};

/*
 * Add cell (x, y) to the search when it is walkable: a known safe cell,
 * or (when the player's cell is assumed to be a trap) an unknown cell in
 * the same row or column as the player, because a row or column never
 * holds two traps.  Reaching row 0 ends the search by pushing head past
 * grid_size.  x is unsigned so that x = -1 is rejected by x < N.
 */
static void visit(struct search *s, unsigned x, int y)
{
  if (y == -1 || !((s->head < grid_size) & (x < N)))
    return;
  s->probe = x + N * y + grid;
  if (*s->probe >= 2)             /* trap or already visited */
    return;
  if (*s->probe || (*s->here == TRAP && (y == row || x == col))) {
    *s->probe -= search_flag + search_flag;   /* mark as visited */
    queue[s->tail++] = s->probe - grid;
    s->head += grid_size * (y == 0);
  }
}

/* sum of the N cells of the row starting at cells */
static char row_sum(char *cells)
{
  char sum = 0;
  for (char i = 0; i < N; i++)
    sum += cells[i];
  return sum;
}

/*
 * A trap was placed at cells[trap]: every other cell of its row and its
 * column is now known to be safe.
 */
static void place_trap(char *cells, char trap)
{
  for (char i = 0; i < N; i++)
    cells[i] = grid[(i - (i > N - 2)) * N + trap] = SAFE;
  cells[trap]++;
}

/*
 * Handle one key: move the player and, when an unknown cell is entered,
 * let the computer decide whether it is a trap.
 */
char move_player(unsigned key)
{
  struct search s;
  char sum, ofs, differ;
  char t = 0;
  int quit = 0, moved = 1;

  /* arrow keys send ESC [ A..D, vi keys are h j k l */
  switch (key) {
  case 'A': case 'k':           row--; break;
  case 'B': case 'j':           row++; break;
  case 'C': case 'l':           col++; break;
  case 'D': case 'h': case 'i': col--; break;
  case 'I': case 'q':           quit = 1; /* fall through */
  default:                      moved = 0;
  }

  /* walls on the left and right; row -1 (outside) is the top wall */
  if (col == -1) col = 0;
  if (col == N)  col = N - 1;
  if (row == -2) row = -1;

  s.here = NULL;
  s.probe = grid;
  s.head = 0;

  if (row != EXIT_ROW && row != -1 && !*(s.here = grid + row * N + col)) {
    if (search_flag *= 2 * attempt - 6) {
      /*
       * Attempts 1 and 2 after the computer started searching.  Try the
       * cell as safe (n = 0) and as a trap (n = 1) and see whether the
       * player could then find a known way from the bottom to the top.
       */
      for (char n = 0; n < 2 + attempt - *s.here; n++) {
        *s.here = n + 1;
        s.tail = 0;
        for (char x = 0; x < N; x++)
          visit(&s, x, N - 2);
        for (s.head = 0; s.head < s.tail; s.head++) {
          char cell = queue[s.head];
          unsigned x = cell % N;
          char y = cell / N;
          visit(&s, x + 1, y);
          visit(&s, x - 1, y);
          visit(&s, x, y - 1);
        }
        for (char i = 0; i < grid_size; i++)
          grid[i] &= 3;           /* remove the visited marks */
        /* trap when "safe" gives a way through or "trap" gives none */
        search_flag |= 0 > (s.head - grid_size + 1) * (n - !n);
      }
      if (!row) {
        if (row_sum(s.probe) == N)
          search_flag |= 1;
        /* both neighbours of the pivot column known to be safe */
        if (pivot_col && pivot_col + 1 < N &&
            grid[pivot_col - 1] & grid[pivot_col + 1] & 1)
          search_flag |= 1;
      }
      *s.here = (search_flag & 1) + 1;
      search_flag = 4 - *s.here - attempt;
    } else {
      /* beginning of attempt 1, and every step of attempts 2 and 3 */
      if (row_sum(grid) == N - 2)
        pivot_col = col;
      if (attempt & 2 || col == pivot_col)
        *s.here = TRAP;
      else if (row && rng_state & 4)
        *s.here = TRAP, pivot_col = col;
      else
        *s.here = SAFE;
      search_flag ^= row | (col == pivot_col);
    }

    /* tell the player what can be deduced from the known cells */
    t = 1;
    if (*s.here == TRAP)
      place_trap(s.here - col, col);
    while (t--)
      for (char *cells = grid; cells < grid_size + grid; cells += N) {
        sum = row_sum(cells);
        /* N - 1 safe cells: the last unknown cell is the trap */
        if (sum == N - 1)
          for (char x = 0; x < N; x++)
            if ((t = !cells[x])) {
              place_trap(cells, x);
              break;
            }
        /*
         * Two rows next to each other with the same unknown cells: the
         * two traps are in these columns, so all other rows are safe there.
         */
        if ((ofs = cells - grid) && sum + 2 == N) {
          differ = 0;
          for (char x = 0; x < N; x++)
            differ |= !cells[x - N] ^ !cells[x];
          if (!differ)
            for (char other = 0; other < grid_size; other += N)
              if (ofs != other && other + N != ofs)
                for (char x = 0; x < N; x++)
                  grid[other + x] += !cells[x] && (t = !grid[other + x]);
        }
      }
  }

  if (quit)
    return RESULT_QUIT;
  if (!moved)
    return RESULT_NOTHING;
  if (row > N - 2)                  /* escaped */
    return attempt += 2;
  if (!s.here || *s.here != TRAP)
    return RESULT_MOVED;
  if (attempt < 3)                  /* back to the start */
    row = -1;
  attempt++;
  return (2 + attempt) / 2;         /* RESULT_TRAPPED or RESULT_LOST */
}

#ifndef NO_LOOP
/* platform adapters */
int io_char(int ch);              /* print ch, or read a key when ch == 0 */
void play_sound(int result);
unsigned entropy(void);

static void put_str(const char *str)
{
  while (*str)
    io_char(*str++);
}

/* move the cursor up over the whole board: ESC [ nn A */
static void cursor_up(void)
{
  io_char('\033');
  io_char('[');
  io_char('0' + (N + 2) / 10);
  io_char('0' + (N + 2) % 10);
  io_char('A');
}

static void draw_board(int show_header)
{
  if (show_header) {
    put_str("Move ");
    io_char('0' + attempt);
    put_str("\033[K\n");
  }
  /* row -1 is the entrance, row N-1 the exit */
  for (signed char i = -N; i != N * N; i++) {
    int cursor = i == row * N + col;
    io_char(cursor ? '[' : ' ');
    io_char(i < 0 || i >= grid_size ? '=' : ".*o"[grid[i]]);
    io_char(cursor ? ']' : ' ');
    if (i == -1 || (i >= 0 && i % N == N - 1))
      io_char('\n');
  }
}

/*
 * A key pressed on the end screen.  Returns the state to continue with,
 * or 0 to keep waiting.  q, Ctrl-C and Ctrl-D quit.
 */
static int end_screen_key(int key)
{
  int next;

  if (key - 10 >= attempt << 5)
    return 0;
  switch (key) {
  case 'q': case 3: case 4:         next = 7;  break;  /* quit */
  case 216:                         next = 15; break;  /* quit */
  case 5: case 6:                   next = 5;  break;  /* redraw */
  case EOF: case 0: case 7: case 8: next = 3;  break;  /* play on */
  default:                          return 0;
  }
  return attempt < next - 1 ? next : 0;
}

void game_loop(void)
{
  static const char *const message[] = {
    "",                     /* RESULT_NOTHING */
    "\033[K",               /* RESULT_MOVED */
    "(O O)\r",              /* RESULT_TRAPPED */
    "(> <)\n",              /* RESULT_LOST */
    "\\(>-<)/\n",           /* RESULT_WON_2 */
    "\\(^ ^)/\n",           /* RESULT_WON_3 */
    "~\\(^-^*)\n",          /* RESULT_QUIT */
  };
  int state, key, result;

  for (;;) {
    state = 2;
    if (attempt & 4)
      reset_game(entropy());
    else
      cursor_up();

    for (;;) {
      /* states 2 and 3 play, states 4 and above show the end screen */
      draw_board(!(state & 4));
      state++;

      if (state > 4) {
        put_str("\nPress Enter to continue\n");
        while ((key = io_char(0)) != '\n')
          if ((state = end_screen_key(key)))
            break;
        if (key == '\n')
          break;                    /* next game or next attempt */
      } else {
        result = move_player(io_char(0));
        put_str(message[result]);
        play_sound(result);
        if (result <= RESULT_TRAPPED)
          break;
        state = result + 1;
      }

      /* redraw the board once more, or quit */
      if (state == 7)
        state = 8;
      else
        cursor_up();
      if (state & 8) {
        attempt = 5;
        return;
      }
    }
  }
}

#if !defined(NO_MAIN) && !defined(ARDUINO)
struct termios saved_term, game_term;
FILE *audio;                      /* signed 16 bit, 48000 Hz, mono */

void restore_terminal(void)
{
  if (audio)
    fclose(audio);
  tcsetattr(0, TCSAFLUSH, &saved_term);
}

int io_char(int ch) { return ch ? putchar(ch) : getchar(); }

/*
 * Every sound starts with a 440 Hz beep (1090 samples), then plays a tone
 * or noise and ends with 2400 samples of silence.
 */
void play_sound(int result)
{
  static const struct {
    int16_t length;               /* samples, 0 = no sound */
    int16_t half_period;          /* of the tone, 0 = noise */
  } sound[] = {
    { 0,    0 },                  /* nothing */
    { 3490, 0 },                  /* moved: only the beep */
    { 6050, 0 },                  /* trapped: noise */
    { 8610, 109 },                /* lost: 220 Hz */
    { 8610, 27 },                 /* won: 889 Hz */
    { 8610, 27 },
    { 8610, 27 },                 /* quit */
  };
  int16_t sample, noise = -112;

  if (!audio || !sound[result].length)
    return;
  for (int i = 0; fflush(audio) || i != sound[result].length; ) {
    if (i < 1090)
      sample = 54 > i % 109;
    else if (i < sound[result].length - 2400)
      sample = sound[result].half_period
             ? i / sound[result].half_period & 1
             : !((noise *= 997) % 3);
    else
      sample = 0;
    sample <<= 11;
    i += fwrite(&sample, 2, 1, audio);
  }
}

unsigned entropy(void)
{
  return time(0) ^ clock() << 3 ^ (long long)grid >> 12;
}

int main(int argc, char *argv[])
{
  /* turn off echo and line buffering */
  tcgetattr(0, &saved_term);
  game_term = saved_term;
  game_term.c_lflag &= ~ECHO & ~ICANON;
  atexit(restore_terminal);
  tcsetattr(0, TCSANOW, &game_term);

  if (argc && (audio = fopen(argv[1], "wb")))
    setbuf(audio, 0);
  else
    printf("No audio pipe supplied, game will be silent (_ _)\n");

  game_loop();
}
#endif
#endif
