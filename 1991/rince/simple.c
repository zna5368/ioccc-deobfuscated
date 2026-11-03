#include <curses.h>

/* Number of moves the player may make before the game is lost. */
#define MOVES_ALLOWED 484

/* Number of diamonds that must be collected to win. */
#define DIAMONDS_NEEDED 39

/*
 * Every map square is a letter.  The letter minus 'A' is the index of
 * its picture in tile_art.
 */
#define WALL 'A'
#define EMPTY 'B'
#define HOLLOW_BOULDER 'C' /* can be crushed into diamonds */
#define SOLID_BOULDER 'D'
#define LEFT_KEY 'E' /* opens LEFT_DOOR when pushed left */
#define DOWN_KEY 'F' /* opens DOWN_DOOR when pushed down */
#define UP_KEY 'G'   /* opens UP_DOOR when pushed up */
#define PLAYER_RIGHT 'H'
#define LEFT_DOOR 'I'
#define DOWN_DOOR 'J'
#define UP_DOOR 'K'
#define DIAMOND 'L'
#define PLAYER_LEFT 'M'
#define PLAYER_DOWN 'N'
#define PLAYER_UP 'O'

/* Every tile is drawn as 3 lines of 6 characters. */
#define TILE_HEIGHT 3
#define TILE_WIDTH 6

/* The screen shows 12 x 7 tiles of the map at a time. */
#define VIEW_COLS 12
#define VIEW_ROWS 7

#define MAP_ROWS 12
#define MAP_COLS 22

char *tile_art[][TILE_HEIGHT] = {
    /* A wall           */ {"######", "######", "######"},
    /* B empty          */ {"      ", "      ", "      "},
    /* C hollow boulder */ {"  ___ ", " /   \\", " \\___/"},
    /* D solid boulder  */ {"  ___ ", " /@@@\\", " \\@@@/"},
    /* E key (left)     */ {"| | /\\", "----||", "    \\/"},
    /* F key (down)     */ {"  /--\\", "   \\/ ", " ==|  "},
    /* G key (up)       */ {" ==|  ", "   /\\ ", "  \\--/"},
    /* H player right   */ {" /O__o", " \\___/", " /_\\_ "},
    /* I door (left)    */ {"  <<  ", "  <<  ", "  <<  "},
    /* J door (down)    */ {"      ", "vvvvvv", "      "},
    /* K door (up)      */ {"      ", "^^^^^^", "      "},
    /* L diamond        */ {"  /\\  ", " <  > ", "  \\/  "},
    /* M player left    */ {"o__O\\ ", "\\___/ ", " _/_\\ "},
    /* N player down    */ {" O__O ", "/----\\", "_/  \\_"},
    /* O player up      */ {" ^__^ ", "/----\\", "_/  \\_"},
};

char map[MAP_ROWS][MAP_COLS] = {
    "AAAAAAAAAAAAAAAAAAAAAA", "ALCABBBBBBBABBCBBCABBA",
    "ACCABAKAAABAFBABABAEBA", "ABBBBABBIBBBBBEEABACLA",
    "ACCBNIBBAAAAGALBBBABCA", "AAAAAAAJACBABBCABAABCA",
    "ALLAACCBACBAAALLCABBBA", "ABDLAAABABCAAAALCABBLA",
    "ABCBIBBBBBBBBAAAAACBBA", "ABCCAABBADBBBBDBDBDBCA",
    "ALBBACCCCDBCABBBDBBBBA", "AAAAAAAAAAAAAAAAAAAAAA"};

/* Movement keys and their directions, in the same order. */
char *move_keys = "hjkl";
int dir_x[] = {-1, 0, 0, 1};
int dir_y[] = {0, 1, -1, 0};
char player_facing[] = {PLAYER_LEFT, PLAYER_DOWN, PLAYER_UP, PLAYER_RIGHT};

enum { LEFT, DOWN, UP, RIGHT };

int player_x, player_y;
int diamonds_left;
int moves_left = MOVES_ALLOWED;

/*
 * Draw the part of the map around the player.  The view jumps in steps
 * of 5 rows and 10 columns so that it overlaps the previous one.
 */
void draw_view(void) {
  int top = (player_y - 1) / 5 * 5;
  int left = (player_x - 1) / 10 * 10;
  int row, col, line;

  for (row = 0; row < VIEW_ROWS; row++)
    for (col = 0; col < VIEW_COLS; col++)
      for (line = 0; line < TILE_HEIGHT; line++)
        mvaddstr(row * TILE_HEIGHT + line, col * TILE_WIDTH,
                 tile_art[map[top + row][left + col] - 'A'][line]);
}

/* Move the player one square to (x, y). */
void step_to(int x, int y) {
  player_x = x;
  player_y = y;
  moves_left--;
}

int main(void) {
  char facing;
  char *key;

  initscr();
  clear();
  noecho();
  cbreak();

  player_x = player_y = 4; /* the map holds PLAYER_DOWN here */
  diamonds_left = DIAMONDS_NEEDED;
  facing = PLAYER_LEFT;

  while (diamonds_left && moves_left) {
    draw_view();
    refresh();

    /* The player is only stored in the map while drawing. */
    map[player_y][player_x] = EMPTY;

    key = (char *)strchr(move_keys, getch());
    if (key) {
      int dir = key - move_keys;
      /* the square next to the player and the one beyond it */
      int next_x = player_x + dir_x[dir];
      int next_y = player_y + dir_y[dir];
      int beyond_x = next_x + dir_x[dir];
      int beyond_y = next_y + dir_y[dir];
      char *next = &map[next_y][next_x];
      char *beyond = &map[beyond_y][beyond_x];

      facing = player_facing[dir];

      switch (*next) {
      case EMPTY:
        step_to(next_x, next_y);
        break;

      case DIAMOND:
        step_to(next_x, next_y);
        diamonds_left--;
        break;

      case HOLLOW_BOULDER:
        if (*beyond == EMPTY) {
          *beyond = *next;
          step_to(next_x, next_y);
        } else if (*beyond == HOLLOW_BOULDER) {
          /* crushing two hollow boulders makes two diamonds */
          *beyond = *next = DIAMOND;
        }
        break;

      case SOLID_BOULDER:
        if (*beyond == EMPTY) {
          *beyond = *next;
          step_to(next_x, next_y);
        }
        break;

      case LEFT_KEY:
      case DOWN_KEY:
      case UP_KEY:
        if (*beyond == EMPTY) {
          *beyond = *next;
          step_to(next_x, next_y);
        } else if ((*next == LEFT_KEY && *beyond == LEFT_DOOR && dir == LEFT) ||
                   (*next == DOWN_KEY && *beyond == DOWN_DOOR && dir == DOWN) ||
                   (*next == UP_KEY && *beyond == UP_DOOR && dir == UP)) {
          /* the key unlocks the door and both disappear */
          *beyond = *next = EMPTY;
        }
        break;
      }
    }

    map[player_y][player_x] = facing;
  }

  clear();
  mvaddstr(12, 31, moves_left ? "Well done!" : "Too many moves");
  move(23, 0);
  refresh();
  endwin();
  printf("%s\n", moves_left ? "Well done!" : "Too many moves");
  return 0;
}
