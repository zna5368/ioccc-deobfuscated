/*
 * Checkers: the human plays Black (x) and the computer plays Red (o).
 *
 * The board has 32 playable squares, numbered 0..31 internally and 1..32 on
 * screen.  Square s is in row s / 4 (row 0 is at the bottom, next to Black).
 * Each side is kept as a 32 bit mask.  The board is always stored from the
 * point of view of the side to move: before the computer thinks, the board is
 * rotated 180 degrees (bit reversed) and the two sides are swapped.
 *
 * Directions (as seen by the side to move):
 *
 *        1   0
 *         \ /
 *          .
 *         / \
 *        3   2
 *
 * A move is encoded as: bits 0-4 = starting square, bits 5-6 = direction,
 * bit 7 = set if the move is a capture (jump).
 */

typedef struct {
  int mine;   /* squares holding pieces of the side to move */
  int theirs; /* squares holding pieces of the other side */
  int kings;  /* squares holding kings (of either side) */
  int result; /* best move found by a search, or landing square of a move */
  int best;   /* best score found so far (alpha) */
  int limit;  /* score above which the search may stop (beta) */
} Board;

#define MOVE_IS_JUMP 128

/* search_piece() modes for the first move of a turn */
#define NO_CAPTURES -1  /* no capture is possible: only simple moves */
#define MUST_CAPTURE -2 /* a capture is possible, so it must be taken */

#define INFINITY_SCORE (1 << 30)

#define HUMAN_TURN 4    /* state bit: Black (the human) is to move */
#define CONTINUE_JUMP 1 /* state bit: the same piece must jump again */

/* set by apply_move(): the piece that just captured must keep jumping */
unsigned must_jump_again;

/* reverse the order of the 32 bits (turns the board around) */
unsigned reverse_bits(unsigned x) {
  x = (x >> 1 & 0x55555555) + (x & 0x55555555) * 2;
  x = (x >> 2 & 0x33333333) + (x & 0x33333333) * 4;
  x = (x >> 4 & 0x0F0F0F0F) + (x & 0x0F0F0F0F) * 16;
  x = (x >> 8 & 0x00FF00FF) + (x & 0x00FF00FF) * 256;
  x = (x >> 16 & 0x0000FFFF) + (x & 0x0000FFFF) * 65536;
  return x;
}

/* number of bits that are set */
unsigned count_bits(unsigned x) {
  x = (x >> 1 & 0x55555555) + (x & 0x55555555);
  x = (x >> 2 & 0x33333333) + (x & 0x33333333);
  x = (x >> 4 & 0x0F0F0F0F) + (x & 0x0F0F0F0F);
  x = (x >> 8 & 0x00FF00FF) + (x & 0x00FF00FF);
  x = (x >> 16 & 0x0000FFFF) + (x & 0x0000FFFF);
  return x;
}

/* give the move to the other side: rotate the board and swap the pieces */
Board flip_board(Board old) {
  Board board;
  board.mine = reverse_bits(old.theirs);
  board.theirs = reverse_bits(old.mine);
  board.kings = reverse_bits(old.kings);
  board.best = -INFINITY_SCORE;
  board.limit = -old.best;
  return board;
}

/*
 * The square next to `square` in direction `dir`, or -1 if that is off the
 * board or one of the `occupied` squares.  For directions 0..3 the step is
 * +3, +4, -5, -4, plus one on odd rows.  Directions 0 and 1 go up a row,
 * directions 2 and 3 go down a row.
 */
int neighbor(int square, int dir, unsigned occupied) {
  int next = square + (dir + 8) % 10 - 5 + square / 4 % 2 & 31;
  int expected_row = square / 4 + 1 - (dir & 2);

  if (occupied >> next & 1 || next / 4 != expected_row)
    return -1;
  return next;
}

/* the encoded move of the piece on `from` in direction `dir`, or -1 */
int try_move(Board board, int from, int dir) {
  int next = neighbor(from, dir, board.mine);

  if (next == -1)
    return -1;
  if (board.theirs >> next & 1) {
    /* an enemy piece: it can be jumped if the square behind it is empty */
    if (neighbor(next, dir, board.theirs | board.mine) != -1)
      return MOVE_IS_JUMP | dir * 32 | from;
    return -1;
  }
  return dir * 32 | from;
}

/*
 * Can any piece of the side to move on the squares in `which` move?
 * With jumps_only set, only captures count.
 */
int has_move(Board board, unsigned which, int jumps_only) {
  int square, dir;

  for (square = 32; square--;)
    if ((board.mine & which) >> square & 1)
      for (dir = 2 << (board.kings >> square & 1); dir--;)
        if (try_move(board, square, dir) >= jumps_only << 7)
          return 1;
  return 0;
}

/*
 * Make a move.  A jump is done as two steps: first onto the enemy piece and
 * then on to the empty square behind it.  Clearing the square left behind in
 * the second step removes the captured piece.
 */
Board apply_move(Board board, int move) {
  unsigned from = move & 31;
  unsigned dir = move / 32 & 3;
  unsigned to = board.result = neighbor(from, dir, 0);
  unsigned to_bit = 1 << to;
  unsigned clear_mask;

  board.mine |= to_bit;
  board.kings &= ~to_bit;
  board.kings |= (board.kings >> from & 1) << to;

  /* leaving an enemy square means a capture was just made */
  must_jump_again = board.theirs >> from & 1 && has_move(board, to_bit, 1);

  /* crown a piece that reaches the far row (squares 28-31) */
  board.kings |= to_bit & 0xF0000000;

  clear_mask = ~(1 << from);
  board.mine &= clear_mask;
  board.kings &= clear_mask;
  board.theirs &= clear_mask;

  if (move >> 7) /* jump: second step */
    return apply_move(board, dir << 5 | to);
  return board;
}

/* number of positions looked at during this turn */
unsigned nodes;

Board search_all(Board board, int depth);

/*
 * Material balance from the point of view of the side that just moved (the
 * pieces in board.theirs).  A king is worth 4 men.  A little random noise
 * makes the computer play differently each game.
 */
int evaluate(Board board) {
  int opponent = count_bits(board.mine) + 3 * count_bits(board.mine & board.kings);
  int mover = count_bits(board.theirs) + 3 * count_bits(board.theirs & board.kings);

  return ((mover - opponent) << 24) / (mover + opponent) + (short)rand();
}

/*
 * Alpha-beta (negamax) search of all moves of the piece on `from`.
 * `first_move` is NO_CAPTURES, MUST_CAPTURE, or, while a multiple jump is
 * being continued, the move that started it (which is the move reported).
 */
Board search_piece(Board board, int from, int depth, int first_move) {
  int dir, move, reported, score, extended;
  int want_jump = first_move != NO_CAPTURES;

  for (dir = 2 << (board.kings >> from & 1); dir--;) {
    move = try_move(board, from, dir);
    if (move != -1 && move >> 7 == want_jump && board.best <= board.limit) {
      Board next = apply_move(board, move);

      reported = first_move < 0 ? move : first_move;
      ++nodes;
      if (must_jump_again) {
        /* same player keeps jumping with the same piece */
        score = search_piece(next, next.result, depth, reported).best;
      } else {
        next = flip_board(next);
        /* look one move further if the opponent has to capture */
        extended = depth + has_move(next, -1, 1);
        if (extended)
          score = -search_all(next, extended - 1).best;
        else
          score = evaluate(next);
      }
      if (score >= board.best) {
        board.best = score;
        board.result = reported;
      }
    }
  }
  return board;
}

/* search all moves of the side to move */
Board search_all(Board board, int depth) {
  int square;
  int first_move = has_move(board, -1, 1) ? MUST_CAPTURE : NO_CAPTURES;

  for (square = 32; square-- && board.best <= board.limit;)
    if (board.mine >> square & 1) {
      board = search_piece(board, square, depth, first_move);
    }
  return board;
}

int main() {
  int row, col, square, from, dir, state = HUMAN_TURN, prev_state, retry,
      depth, move;
  Board board;

  srand(clock());
  board.mine = 4095;          /* Black on squares 1-12 */
  board.theirs = 4095 << 20;  /* Red on squares 21-32 */
  board.kings = 0;

  for (;;) {
    /* draw the board, top row first, with the number of each row's
       rightmost square on the right */
    for (row = 8; row--;) {
      for (col = 8; col--;) {
        square = row * 4 + col / 2;
        printf((row ^ col ^ 1) % 2 ? "%c" : " ",
               ".xoXO"[(board.kings >> square & 1) * 2 +
                       (board.theirs >> square & 1) * 2 +
                       (board.mine >> square & 1)]);
      }
      printf(" %2d\n", square + 1);
    }

    for (retry = 1; retry--;) {
      printf(state & HUMAN_TURN ? "Black" : "Red");
      nodes = 0;
      if (!(state & HUMAN_TURN))
        board = flip_board(board);
      if (!has_move(board, -1, 0)) {
        printf(" loses!\n");
        return 0;
      }
      printf(state & CONTINUE_JUMP ? " %sdirection: "
                                   : " position and %sdirection: ",
             has_move(board, -1, 1) ? "jump " : "");

      if (state & HUMAN_TURN) {
        char first_word[5] = {'\0'}, second_word[5] = {'\0'};

        if (state & CONTINUE_JUMP) {
          /* only a direction is read; move is left as it was */
          scanf("%4s", first_word);
          dir = atoi(first_word);
        } else {
          scanf("%4s%4s", first_word, second_word);
          from = atoi(first_word);
          dir = atoi(second_word);
          --from;
          move = board.mine >> from & 1 ? try_move(board, from, dir) : -1;
          /* a capture must be made whenever one is possible */
          if (retry = move >> 7 != has_move(board, -1, 1)) {
            printf("illegal\n");
          }
        }
      } else {
        /* computer: deepen the search until enough positions were seen */
        board.best = -(board.limit = INFINITY_SCORE);
        for (depth = 4; nodes < 65536 && depth < 20; depth++) {
          move = (state & CONTINUE_JUMP
                      ? search_piece(board, from, depth, MUST_CAPTURE)
                      : search_all(board, depth))
                     .result;
        }
        /* convert back to the human's view of the board */
        if (state & CONTINUE_JUMP)
          printf("%d\n", 3 - (move >> 5 & 3));
        else
          printf("%d %d\n", 32 - (move & 31), 3 - (move >> 5 & 3));
      }
    }

    board = apply_move(board, move);
    prev_state = state;
    if (must_jump_again) {
      state |= CONTINUE_JUMP;
      from = board.result;
    } else {
      /* other player's turn */
      state = (state & ~CONTINUE_JUMP) ^ HUMAN_TURN;
    }
    if (!(prev_state & HUMAN_TURN))
      board = flip_board(board);
  }
}
