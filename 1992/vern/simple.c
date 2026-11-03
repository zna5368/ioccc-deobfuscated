#include <stdio.h>

/*
 * The board is 8x8 squares numbered 0..63 (octal 00..77). The square
 * number is row * 8 + column, row 0 is the computer's (black) back rank and
 * row 7 is the human's (white) back rank.
 */
#define SQUARES 64
#define WIDTH 8
#define COL_MASK 7  /* square & 7  -> column */
#define ROW_SHIFT 3 /* square >> 3 -> row    */
#define ROW(sq) ((sq) >> ROW_SHIFT)
#define COL(sq) ((sq) & COL_MASK)

/* distance of a row/column from the nearer edge: 0,1,2,3,3,2,1,0 */
#define EDGE_DIST(x) ((x) <= 3 ? (x) : 7 - (x))

#define SIGN(x) ((x) < 0 ? -1 : !!(x))
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define INFINITE_SCORE 9999

/* the side whose turn it is; also the direction its pawns move in */
#define WHITE -1 /* human, bottom of the board */
#define BLACK 1  /* computer, top of the board */

/*
 * Each square holds a pointer to the move checker of the piece standing on
 * it (or 0 when empty). A move checker returns non-zero when the move is
 * illegal for that piece.
 */
typedef int (*Piece)(int from, int to, int dx, int dy);

Piece board[SQUARES];
int color[SQUARES]; /* WHITE, BLACK or 0 for an empty square */

int side = WHITE;        /* side that is moving */
int search_depth;        /* plies left to search */
int best_move;           /* from * 64 + to chosen by search() */
int positions_evaluated; /* number of calls to evaluate() */
int move_number;         /* number of moves the human has made */

int rook(int from, int to, int dx, int dy);

/* is anything standing between from and to on a straight/diagonal line? */
int path_blocked(int from, int to, int dx, int dy) {
  int step = SIGN(dx) + SIGN(dy) * WIDTH;
  int sq;

  if (!step)
    return 0;
  for (sq = from + step; sq != to && !board[sq]; sq += step)
    ;
  return sq != to;
}

int pawn(int from, int to, int dx, int dy) {
  if (dx) /* diagonal capture */
    return ABS(dx) > 1 || dy != side || !board[to];
  if (board[to]) /* pawns can't capture straight ahead */
    return 1;
  if (dy == side) /* single step */
    return 0;
  if (dy == 2 * side) /* double step must land on the 4th rank */
    return board[from + side * WIDTH] ||
           ROW(to) != (side == WHITE ? 4 : 3);
  return 1;
}

int rook(int from, int to, int dx, int dy) {
  return dx * dy || path_blocked(from, to, dx, dy);
}

int king(int from, int to, int dx, int dy) {
  if (ABS(dx) <= 1 && ABS(dy) <= 1)
    return 0;
  /* king side castling (no check or has-moved tests) */
  return to - from != 2 || COL(from) != 4 ||
         ROW(from) != (side == BLACK ? 0 : 7) || board[from + 1] ||
         board[from + 2] || board[from + 3] != rook ||
         color[from + 3] * side < 0;
}

int queen(int from, int to, int dx, int dy) {
  return (dx * dy && ABS(dx) != ABS(dy)) || path_blocked(from, to, dx, dy);
}

int bishop(int from, int to, int dx, int dy) {
  return ABS(dx) != ABS(dy) || path_blocked(from, to, dx, dy);
}

int knight(int from, int to, int dx, int dy) {
  (void)from;
  (void)to;
  return ABS(dx * dy) != 2;
}

char piece_letter(Piece p) {
  return p == rook     ? 'r'
         : p == knight ? 'n'
         : p == bishop ? 'b'
         : p == queen  ? 'q'
         : p == king   ? 'k'
                       : 'p';
}

/* move a piece (castling moves the rook too, pawns promote to queens) */
void make_move(int from, int to) {
  if ((board[to] = board[from]) == pawn && EDGE_DIST(ROW(to)) == 0)
    board[to] = queen;
  if (board[from] == king) {
    if (to - from == 2) /* castle: move the rook over the king */
      make_move(to + 1, to - 1);
    else if (from - to == 2) /* uncastle: put the rook back */
      make_move(from - 1, from + 1);
  }
  color[to] = color[from];
  board[from] = 0;
  color[from] = 0;
}

typedef struct {
  Piece moved, captured;
  int captured_color;
} Undo;

Undo do_move(int from, int to) {
  Undo u;
  u.moved = board[from];
  u.captured = board[to];
  u.captured_color = color[to];
  make_move(from, to);
  return u;
}

void undo_move(int from, int to, Undo u) {
  make_move(to, from);
  board[to] = u.captured;
  board[from] = u.moved;
  color[to] = u.captured_color;
}

int in_check(void);

/*
 * Non-zero when side can not move the piece on from to to. When
 * check_king is set, moves that leave the own king in check are illegal.
 */
int illegal_move(int from, int to, int check_king) {
  int illegal;
  Undo u;

  if (color[from] != side || color[to] == side)
    return 1;
  if (board[from](from, to, COL(to) - COL(from), ROW(to) - ROW(from)))
    return 1;
  if (!check_king)
    return 0;
  u = do_move(from, to);
  illegal = in_check();
  undo_move(from, to, u);
  return illegal;
}

/* is the king of side attacked by any piece of the other side? */
int in_check(void) {
  int king_sq, sq;

  for (king_sq = 0; king_sq < SQUARES; ++king_sq)
    if (color[king_sq] == side && board[king_sq] == king)
      break;
  if (king_sq == SQUARES)
    return 0;

  side = -side;
  for (sq = 0; sq < SQUARES; ++sq)
    if (!illegal_move(sq, king_sq, 0)) {
      side = -side;
      return 1;
    }
  side = -side;
  return 0;
}

void setup_board(void) {
  int sq;

  for (sq = 0; sq < SQUARES; ++sq) {
    int row = EDGE_DIST(ROW(sq));
    int col = EDGE_DIST(COL(sq));

    if (row == 0)
      board[sq] = col == 0   ? rook
                  : col == 1 ? knight
                  : col == 2 ? bishop
                  : COL(sq) == 3 ? queen
                                 : king;
    else if (row == 1)
      board[sq] = pawn;
    else
      board[sq] = 0;
    color[sq] = !board[sq] ? 0 : sq < 32 ? BLACK : WHITE;
  }
}

void print_board(void) {
  int sq;

  for (sq = 0; sq < SQUARES; ++sq) {
    if (sq % WIDTH == 0)
      printf("\n%4o ", sq);
    if (board[sq]) {
      char c = piece_letter(board[sq]);
      printf("%c ", color[sq] == WHITE ? c - 'a' + 'A' : c);
    } else
      printf("- ");
  }
  printf("\n    ");
  for (sq = 0; sq < WIDTH; ++sq)
    printf("%2d", sq);
  printf("\n");
}

/* static evaluation from the point of view of side */
int evaluate(void) {
  int sq, to, total = 0;

  for (sq = 0; sq < SQUARES; ++sq) {
    Piece p = board[sq];
    int centre, value, owner;

    if (!p)
      continue;
    centre = EDGE_DIST(ROW(sq)) + EDGE_DIST(COL(sq));
    owner = color[sq];

    if (p == rook)
      value = 88;
    else if (p == pawn) /* reward advancing pawns */
      value = 11 + centre + (owner == WHITE ? 7 - ROW(sq) : ROW(sq));
    else if (p == queen) { /* discourage moving the queen early */
      value = 124;
      if (move_number < 8 &&
          (COL(sq) != 3 || ROW(sq) != (owner == BLACK ? 0 : 7)))
        value -= SQUARES;
    } else if (p == bishop)
      value = 41 + centre;
    else if (p == king) /* keep the king near the edge */
      value = INFINITE_SCORE - 2 * centre;
    else /* knight */
      value = 36 + 2 * centre;

    /* mobility: 5 points per capture, 1 point per empty square */
    {
      int saved = side;
      side = owner;
      for (to = 0; to < SQUARES; ++to)
        if (!illegal_move(sq, to, 0))
          value += color[to] ? 5 : 1;
      side = saved;
    }
    total += owner == side ? value : -value;
  }

  if ((++positions_evaluated & (SQUARES - 1)) == 0)
    write(1, ".", 1);
  return total;
}

/*
 * Negamax search with alpha-beta cut off. Every piece of side is tried on
 * every square of the board. Returns the score for side and stores the best
 * move found in best_move.
 */
int search(int cutoff) {
  int move, from, to, score, moves = 0, best = -INFINITE_SCORE, choice = 0;
  Undo u;

  if (search_depth-- == 0) {
    ++search_depth;
    return evaluate();
  }

  for (move = 0; move < SQUARES * SQUARES; ++move) {
    from = move / SQUARES;
    to = move % SQUARES;
    if (illegal_move(from, to, 1))
      continue;

    side = -side;
    u = do_move(from, to);
    score = -search(-best);
    undo_move(from, to, u);
    side = -side;

    if (score > best) {
      ++moves;
      choice = move;
      best = score;
      if (best >= cutoff) {
        ++search_depth;
        return best;
      }
    }
  }

  if (!moves) /* checkmate or stalemate */
    best = in_check() ? -INFINITE_SCORE + 1 : 0;
  best_move = choice;
  ++search_depth;
  return best;
}

void play(void) {
  int from, to, computer_score = 0;
  char from_str[5], to_str[5];

  for (;;) {
    print_board();
    do {
      int check, score;

      from_str[0] = '\0';
      to_str[0] = '\0';
      from = to = 0;
      check = in_check();
      score = evaluate();
      printf("\n%d %d %d %s ", positions_evaluated, computer_score, score,
             check ? "!" : ">");
      fflush(stdout);
    } while (scanf("%4s%4s", from_str, to_str) != 2 ||
             (from = strtol(from_str, NULL, 8)) < 0 || from > 077 ||
             (to = strtol(to_str, NULL, 8)) < 0 || to > 077 ||
             illegal_move(from, to, 1));
    make_move(from, to);
    print_board();

    /* computer's turn */
    positions_evaluated = 0;
    ++move_number;
    side = -side;
    computer_score = search(INFINITE_SCORE);
    from = best_move / SQUARES;
    to = best_move % SQUARES;
    if (illegal_move(from, to, 1)) {
      printf("Rats!\n");
      return;
    }
    make_move(from, to);
    side = -side;
    if (computer_score > SQUARES * SQUARES)
      printf("\nHar har.\n");
  }
}

int main(int argc, char **argv) {
  search_depth = argc > 1 ? atoi(argv[1]) : 2;
  setup_board();
  play();
  return 0;
}
