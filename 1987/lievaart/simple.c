/*
 * Reversi (Othello) played against the computer.
 *
 * The board is a 10x10 grid stored in 100 ints.  The playing squares are
 * numbered row*10+column with rows and columns 1..8, so the squares run from
 * 11 (top left) to 88 (bottom right).  Row 0, row 9, column 0 and column 9 are
 * a border of BORDER cells that stops every scan along a line.
 *
 * The game search needs a fresh copy of the board for every ply, so the
 * program keeps BOARD_COUNT boards one after the other in a single array.
 * The board for search depth d starts at boards + d * BOARD_SIZE and a move
 * made at depth d is written into the board directly after it.
 */

#define BOARD_SIZE 100
#define BOARD_COUNT 11

#define FIRST_SQUARE 11
#define LAST_SQUARE 88

/* contents of a cell; the two players are 1 and 2 so 3 - p is the opponent */
#define EMPTY 0
#define COMPUTER 1
#define HUMAN 2
#define BORDER 3

#define PASS 99         /* the human types this to pass */
#define NO_MOVE 0       /* the computer reports this when it passes */
#define MAX_SCORE 9000   /* bigger than any position score */
#define WIN_SCORE 8003  /* stop searching once a move scores this much */
#define MAX_LEVEL 10

#define CORNER_SCORE 300 /* owning a corner */
#define X_SQUARE_SCORE 50 /* owning the square diagonally next to an empty corner */
#define MOBILITY_SCORE 8  /* each legal move near the bottom of the search */

char input[100];

int boards[BOARD_SIZE * BOARD_COUNT];
int search_depth;

/* step to move one square along each of the eight directions */
int directions[] = {-1, -11, -10, -9, 1, 11, 10, 9};
int corners[] = {11, 18, 81, 88};
/* the square diagonally inside each corner, in the same order as corners[] */
int x_squares[] = {22, 27, 72, 77};

int best_move;         /* the move chosen by the last search() call */
int empty_squares = 60; /* the game ends when this reaches zero */

int search(int depth, int *board, int player, int alpha, int beta);
int play_move(int *board, int square, int player, int target_offset);

/* copy the playing area of board into the board one search level deeper */
void copy_to_next_board(int *board)
{
  int *from, *to = board + BOARD_SIZE + FIRST_SQUARE;

  for (from = board + FIRST_SQUARE; from <= board + LAST_SQUARE; from++)
    *to++ = *from;
}

/*
 * Score a position at the bottom of the search from the point of view of
 * player.  Only the corners count: a corner is worth a lot, and taking the
 * square diagonally next to an empty corner is bad because it gives the
 * corner away.
 */
int evaluate(int *board, int player)
{
  int opponent = 3 - player;
  int score = 0;
  int i, corner, x_square;

  for (i = 0; i < 4; i++) {
    corner = board[corners[i]];
    if (corner == player)
      score += CORNER_SCORE;
    else if (corner == opponent)
      score -= CORNER_SCORE;
    else {
      x_square = board[x_squares[i]];
      if (x_square == player)
        score -= X_SQUARE_SCORE;
      else if (x_square == opponent)
        score += X_SQUARE_SCORE;
    }
  }
  return score;
}

/*
 * Negamax search with alpha-beta pruning.  Returns the score of board for
 * player and leaves the best move found in best_move.
 */
int search(int depth, int *board, int player, int alpha, int beta)
{
  int *next_board = board + BOARD_SIZE;
  int opponent = 3 - player;
  int near_bottom = depth >= search_depth - 1;
  int move_count = 0;
  int best, score, square, best_square;

  if (depth > search_depth)
    return evaluate(board, player);

  /*
   * Near the bottom of the search a mobility bonus is added afterwards, so
   * the best score cannot start at alpha there.
   */
  best = near_bottom ? -MAX_SCORE : alpha;

  for (square = FIRST_SQUARE; square <= LAST_SQUARE; square++) {
    /* try the move, writing the new position into next_board */
    if (play_move(board, square, player, BOARD_SIZE)) {
      move_count++;
      score = -search(depth + 1, next_board, opponent, -beta, -best);
      if (score > best) {
        best_square = square;
        best = score;
        if (score >= beta || score >= WIN_SCORE)
          return score; /* cut off: the opponent will avoid this line */
      }
    }
  }

  if (move_count == 0) {
    /* no legal move: pass and let the opponent move on an unchanged board */
    best_move = NO_MOVE;
    copy_to_next_board(board);
    return -search(depth + 1, next_board, opponent, -beta, -best);
  }

  best_move = best_square;
  if (near_bottom)
    return best + move_count * MOBILITY_SCORE; /* reward mobility */
  return best;
}

/*
 * Place a stone of player on square of board.  If the move is legal the
 * board is first copied into the next board level, then the placed stone and
 * every captured stone are written into board + target_offset (0 changes
 * board itself, BOARD_SIZE leaves the result in the next board level).
 * Returns 1 for a legal move and 0 otherwise.
 */
int play_move(int *board, int square, int player, int target_offset)
{
  int opponent = 3 - player;
  int legal = 0;
  int *start = board + square;
  int *scan;
  int i, step;

  if (*start != EMPTY)
    return 0;

  for (i = 7; i >= 0; i--) {
    step = directions[i];

    /* walk over the opponent's stones in this direction */
    scan = start + step;
    while (*scan == opponent)
      scan += step;

    /* they are captured if one of our stones closes the line */
    if (*scan == player && scan - step != start) {
      if (!legal) {
        legal = 1;
        copy_to_next_board(board);
      }
      /* flip back to and including the starting square */
      while (scan != start) {
        scan -= step;
        scan[target_offset] = player;
      }
    }
  }
  return legal;
}

int read_word(void)
{
  return scanf("%2s", input);
}

void setup_boards(void)
{
  int board, cell;

  for (board = 0; board < BOARD_SIZE * BOARD_COUNT; board += BOARD_SIZE)
    for (cell = 0; cell < BOARD_SIZE; cell++)
      if (cell < FIRST_SQUARE || cell > LAST_SQUARE || cell % 10 == 0 ||
          cell % 10 == 9)
        boards[board + cell] = BORDER;
      else
        boards[board + cell] = EMPTY;

  boards[44] = boards[55] = COMPUTER;
  boards[45] = boards[54] = HUMAN;
}

int main()
{
  int move;

  setup_boards();

  do {
    *input = '\0';
    printf("Level:");
    read_word();
  } while (!isdigit(*input) || (search_depth = atoi(input)) > MAX_LEVEL ||
           search_depth < 0);

  while (empty_squares > 0) {
    /* ask until the human gives a legal move or passes */
    do {
      printf("You:");
      read_word();
      move = atoi(input);
      *input = '\0';
    } while (!play_move(boards, move, HUMAN, 0) && move != PASS);

    if (move != PASS)
      empty_squares--;

    /* search deeper during the end game */
    if (empty_squares < 15 && search_depth < MAX_LEVEL)
      search_depth += 2;

    printf("Wait\n");
    printf("Value:%d\n", search(0, boards, COMPUTER, -MAX_SCORE, MAX_SCORE));

    /* a pass (NO_MOVE) is never legal so it changes nothing */
    empty_squares -= play_move(boards, best_move, COMPUTER, 0);
    printf("move: %d\n", best_move);
  }
}
