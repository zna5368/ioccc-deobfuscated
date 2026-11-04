#include <stdlib.h>
#include <stdio.h>

/*
 * Competitive sudoku: the human and the computer take turns placing a
 * number on an empty sudoku board. Whoever makes the last legal move wins.
 */

#define SIZE 9

typedef struct {
  int cell[SIZE][SIZE]; /* cell[row][col], 0 means empty */
} Board;

typedef struct Player Player;
struct Player {
  Board *board;
  const char *win_message; /* printed when this player makes the last move */
  /* fill in a move; returns 0 if the player gives up */
  int (*choose_move)(Player *self, int *col, int *row, int *value);
};

static void board_clear(Board *board) {
  int row, col;
  for (row = 0; row < SIZE; row++)
    for (col = 0; col < SIZE; col++)
      board->cell[row][col] = 0;
}

static int block_start(int index) {
  return (index < 3) ? 0 : (index < 6) ? 3 : 6;
}

static int board_is_valid_move(Board *board, int col, int row, int value) {
  int i, r, c, row0, col0;

  if (value <= 0 || value >= 10 || col < 0 || col >= SIZE || row < 0 ||
      row >= SIZE || board->cell[row][col] != 0)
    return 0;

  for (i = 0; i < SIZE; i++)
    if (board->cell[row][i] == value || board->cell[i][col] == value)
      return 0;

  row0 = block_start(row);
  col0 = block_start(col);
  for (r = row0; r < row0 + 3; r++)
    for (c = col0; c < col0 + 3; c++)
      if (board->cell[r][c] == value)
        return 0;

  return 1;
}

static int board_has_valid_move(Board *board) {
  int row, col, value;
  for (value = 1; value <= 9; value++)
    for (row = 0; row < SIZE; row++)
      for (col = 0; col < SIZE; col++)
        if (board_is_valid_move(board, col, row, value))
          return 1;
  return 0;
}

static void board_set(Board *board, int col, int row, int value) {
  board->cell[row][col] = value;
}

static void board_print(Board *board) {
  int row, col;
  printf("\n");
  for (row = 0; row < SIZE; row++) {
    for (col = 0; col < SIZE; col++) {
      int v = board->cell[row][col];
      printf("%c%s", v == 0 ? '.' : '0' + v,
             (col == 2 || col == 5) ? "  " : " ");
    }
    printf("  %c%s", row + 'a', (row == 2 || row == 5) ? "\n\n" : "\n");
  }
  printf("\n0 1 2  3 4 5  6 7 8\n\n");
}

/* Ask the human for "row col value" until a legal move is entered. */
static int human_choose_move(Player *self, int *col, int *row, int *value) {
  do {
    char letter[3];
    printf("you: ");
    fflush(stdout);
    scanf("%1s", letter);
    if (*letter == 'q')
      return 0;
    *row = (*letter >= 'a' && *letter <= 'i') ? *letter - 'a' : 9;
    scanf("%d %d", col, value);
  } while (!board_is_valid_move(self->board, *col, *row, *value));
  return 1;
}

/* Pick a random legal move. */
static int cpu_choose_move(Player *self, int *col, int *row, int *value) {
  int moves[SIZE * SIZE * SIZE][3];
  int count = 0, pick, c, r, v;

  for (v = 1; v <= 9; v++)
    for (r = 0; r < SIZE; r++)
      for (c = 0; c < SIZE; c++)
        if (board_is_valid_move(self->board, c, r, v)) {
          moves[count][0] = c;
          moves[count][1] = r;
          moves[count][2] = v;
          count++;
        }

  pick = rand() % count;
  *col = moves[pick][0];
  *row = moves[pick][1];
  *value = moves[pick][2];
  printf("cpu: %c %d %d\n", *row + 'a', *col, *value);
  return 1;
}

static int player_take_turn(Player *player) {
  int col, row, value;
  if (player->choose_move(player, &col, &row, &value)) {
    board_set(player->board, col, row, value);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  Board *board;
  Player *players[2];
  int turn;

  if (argc > 1)
    srand(atoi(argv[1]));

  board = malloc(sizeof(Board));
  board_clear(board);

  players[0] = malloc(sizeof(Player));
  players[0]->board = board;
  players[0]->win_message = "you win";
  players[0]->choose_move = human_choose_move;

  players[1] = malloc(sizeof(Player));
  players[1]->board = board;
  players[1]->win_message = "you lose";
  players[1]->choose_move = cpu_choose_move;

  for (turn = 0;; turn = !turn) {
    board_print(board);
    if (!player_take_turn(players[turn]))
      break; /* human quit */
    if (!board_has_valid_move(board)) {
      board_print(board);
      printf("%s\n", players[turn]->win_message);
      break;
    }
  }

  free(board);
  free(players[0]);
  free(players[1]);

  return 0;
}
