#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Suicide (losing) chess.
 *
 * The board uses the "0x88" layout: square = row * 16 + column, where
 * row 0 is rank 8 (black's back rank) and row 7 is rank 1 (white's back
 * rank).  Only columns 0-7 are real squares, so any square with a bit of
 * 0x88 set is off the board.
 *
 * Pieces are stored as their ASCII letter: positive for white, negative
 * for black, 0 for an empty square.
 */

#define PAWN 'P'
#define KNIGHT 'N'
#define BISHOP 'B'
#define ROOK 'R'
#define QUEEN 'Q'
#define KING 'K'

#define WIN_SCORE 0xFFF /* the side to move has no legal moves */
#define BOARD_SIZE 256
#define MAX_MOVES 500
#define CAPTURE_PENALTY 100
#define MAX_EXTENSION 6

#define OFF_BOARD(square) ((square) & 0x88)

typedef struct {
  char from, to, promotion;
} Move;

/* pieces a pawn may promote to, in move generation order */
char promotion_pieces[] = "KRNBQ";

char board[BOARD_SIZE];
char input[1024];

int knight_offsets[] = {-14, -31, -33, -18, 14, 31, 33, 18};
int king_offsets[] = {-1, -17, -16, -15, 1, 17, 16, 15};

int side = 1;      /* side to move: 1 = white, -1 = black */
int quiet_count;   /* number of non-capturing moves found */
int capture_count; /* number of capturing moves found */
int best_move;     /* index of the best move found by search() */

Move quiet_moves[MAX_MOVES], capture_moves[MAX_MOVES];
Move user_move;

static void add_quiet(int from, int to, int promotion) {
  quiet_moves[quiet_count++] = (Move){from, to, promotion};
}

static void add_capture(int from, int to, int promotion) {
  capture_moves[capture_count++] = (Move){from, to, promotion};
}

/* knight and king: jump to each offset if empty or holding an enemy */
static void add_steps(int from, int *offsets) {
  int n;
  for (n = 0; n < 8; n++) {
    int to = from + offsets[n];
    if (OFF_BOARD(to))
      continue;
    if (board[to] * board[from] < 0)
      add_capture(from, to, 0);
    else if (!board[to])
      add_quiet(from, to, 0);
  }
}

/* bishop, rook and queen: slide in one direction until blocked */
static void add_slides(int from, int step) {
  int to;
  for (to = from + step; !OFF_BOARD(to); to += step) {
    int product = board[to] * board[from];
    if (product < 0) {
      add_capture(from, to, 0);
      break;
    } else if (product > 0)
      break;
    add_quiet(from, to, 0);
  }
}

static void add_pawn_moves(int from) {
  int row = from & 0x70;
  int forward = -16 * side;
  int n;

  /* double step from the starting rank (does not check the square between) */
  if (row == 56 + side * 40 && !board[from + 2 * forward])
    add_quiet(from, from + 2 * forward, 0);

  if (row == 56 - side * 40) {
    /* about to promote: every capture and push comes with 5 piece choices */
    if (!OFF_BOARD(from - 17 * side) && board[from - 17 * side] * side < 0)
      for (n = 0; n < 5; n++)
        add_capture(from, from - 17 * side, promotion_pieces[n] * side);
    if (!OFF_BOARD(from - 15 * side) && board[from - 15 * side] * side < 0)
      for (n = 0; n < 5; n++)
        add_capture(from, from - 15 * side, promotion_pieces[n] * side);
    /* note: the promotion push does not check that the square is empty */
    for (n = 0; n < 5; n++)
      add_quiet(from, from + forward, promotion_pieces[n] * side);
  } else {
    if (!board[from + forward])
      add_quiet(from, from + forward, 0);
    if (!OFF_BOARD(from - 17 * side) && board[from - 17 * side] * side < 0)
      add_capture(from, from - 17 * side, 0);
    if (!OFF_BOARD(from - 15 * side) && board[from - 15 * side] * side < 0)
      add_capture(from, from - 15 * side, 0);
  }
}

/*
 * Generate all legal moves for the side to move into moves[].  Captures are
 * compulsory, so if any capture exists only the captures are returned.
 * Leaves capture_count holding the number of captures found.
 */
int generate_moves(Move *moves) {
  int row, column, n;

  quiet_count = capture_count = 0;
  for (row = 0; row < 8; row++) {
    for (column = 0; column < 8; column++) {
      int square = row << 4 | column;
      int piece = board[square];
      if (piece == 0 || piece * side < 0)
        continue;
      switch (piece < 0 ? -piece : piece) {
      case PAWN:
        add_pawn_moves(square);
        break;
      case KNIGHT:
        add_steps(square, knight_offsets);
        break;
      case QUEEN:
      case BISHOP:
        add_slides(square, 17);
        add_slides(square, -17);
        add_slides(square, 15);
        add_slides(square, -15);
        if (piece == BISHOP || piece == -BISHOP)
          break;
        /* a queen also moves like a rook */
      case ROOK:
        add_slides(square, 1);
        add_slides(square, -1);
        add_slides(square, 16);
        add_slides(square, -16);
        break;
      case KING:
        add_steps(square, king_offsets);
      }
    }
  }

  if (capture_count > 0) {
    for (n = 0; n < capture_count; n++)
      moves[n] = capture_moves[n];
    return capture_count;
  }
  for (n = 0; n < quiet_count; n++)
    moves[n] = quiet_moves[n];
  return quiet_count;
}

void make_move(Move move) {
  board[move.to] = move.promotion ? move.promotion : board[move.from];
  board[move.from] = 0;
}

void show_board(void) {
  int row, column;

  printf(" +---+---+---+---+---+---+---+---+\n");
  for (row = 0; row < 8; row++) {
    putchar('8' - row);
    for (column = 0; column < 8; column++) {
      int piece = board[row * 16 + column];
      if (piece < 0)
        printf("| *%c", -piece); /* black pieces are marked with '*' */
      else if (piece > 0)
        printf("| %c ", piece);
      else
        printf("|   ");
    }
    printf("|\n +---+---+---+---+---+---+---+---+\n");
  }
}

void setup_board(void) {
  static const char back_rank[] = "RNBQKBNR";
  int column;

  for (column = 0; column < 8; column++) {
    board[0 * 16 + column] = -back_rank[column];
    board[1 * 16 + column] = -PAWN;
    board[6 * 16 + column] = PAWN;
    board[7 * 16 + column] = back_rank[column];
  }
}

/*
 * Score the position for the side to move.  Fewer pieces is better, more
 * mobility is better, and being forced to capture is bad.
 */
int evaluate(void) {
  static Move scratch[MAX_MOVES];
  int score = 0, square;

  for (square = 0; square < 120; square++) {
    if (board[square] * side > 0)
      score--;
    if (board[square] * side < 0)
      score++;
  }

  score += generate_moves(scratch);
  score -= capture_count ? CAPTURE_PENALTY : 0;
  side = -side;
  score -= generate_moves(scratch);
  score += capture_count ? CAPTURE_PENALTY : 0;
  side = -side;

  return score;
}

/*
 * Negamax search.  When there are fewer than 5 replies the search goes
 * deeper: a forced (single) move costs no depth, otherwise up to
 * MAX_EXTENSION extra plies are added.
 */
int search(int depth, int extension) {
  int best_score = -WIN_SCORE, n, count, best_index, score;
  Move moves[MAX_MOVES];
  char saved[BOARD_SIZE];

  /*
   * In the original, best_move shares a variable with the column loop
   * counter of generate_moves(), so an early return leaves it at 8.
   */
  if (!(depth + extension)) {
    score = evaluate() + time(NULL) % 9 - 4; /* time() is the randomness */
    best_move = 8;
    return score;
  }
  count = generate_moves(moves);
  best_move = 8;
  if (!count)
    return WIN_SCORE;

  if (count < 5) {
    if (count == 1)
      depth++;
    else if (extension < MAX_EXTENSION)
      extension++;
  }

  memcpy(saved, board, BOARD_SIZE);
  for (n = 0; n < count; n++) {
    memcpy(board, saved, BOARD_SIZE);
    make_move(moves[n]);
    side = -side;
    /*
     * Strict '>' means best_index is never set when every move loses,
     * which is what makes the program crash when it loses.
     */
    if ((score = -search(depth - 1, extension)) > best_score) {
      best_index = n;
      best_score = score;
    }
    side = -side;
  }
  best_move = best_index;
  return best_score;
}

int main(int argc, char *argv[]) {
  Move legal_moves[MAX_MOVES];
  char saved[BOARD_SIZE];
  int count, n;

  setup_board();
  while (1) {
    show_board();
    count = generate_moves(legal_moves);
    scanf("%s", input);
    if (strcmp(input, "quit") == 0)
      return 0;
    else if (strcmp(input, "g") == 0) {
      /* computer move */
      memcpy(saved, board, BOARD_SIZE);
      printf("Value %d\n", search(argc > 1 ? atoi(argv[1]) : 3, 0));
      memcpy(board, saved, BOARD_SIZE);
      make_move(legal_moves[best_move]);
      side = -side;
    } else {
      /* human move such as "e2e4" or "e7e8=Q" */
      user_move.from = ('8' - input[1]) * 16 + input[0] - 'a';
      user_move.to = ('8' - input[3]) * 16 + input[2] - 'a';
      user_move.promotion = input[4] ? input[5] : 0;
      for (n = 0; n < count; n++)
        if (user_move.from == legal_moves[n].from &&
            user_move.to == legal_moves[n].to &&
            user_move.promotion == legal_moves[n].promotion)
          break;
      if (n == count)
        printf("Illegal move\n");
      else {
        make_move(user_move);
        side = -side;
      }
    }
  }
}
