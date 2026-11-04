#include <setjmp.h>

/*
 * The board is a 10 x 12 "mailbox" array.  The playing squares are 21..98:
 * the file is square % 10 (1 = A .. 8 = H) and the rank is 10 - square / 10
 * (square 21 is A8, square 98 is H1).  Every other entry is a border square
 * so pieces walking off the board always land on a BORDER value.
 *
 * Black pieces are stored as their type (1..6).  White pieces are stored as
 * the bitwise complement of their type (~1..~6, i.e. -2..-7).
 */
enum { EMPTY = 0, BORDER = -1, PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING };

#define KING_CAPTURED 100000      /* score for capturing a king           */
#define MATE_LIMIT 10000          /* scores beyond this mean a king is lost */
#define NO_MOVE_SCORE -1000000000 /* best score before any move is tried    */

/* bits of game_flags: which kings and rooks have moved (for castling) */
#define BLACK_KING_MOVED 1
#define A8_ROOK_MOVED 2
#define H8_ROOK_MOVED 4
#define WHITE_KING_MOVED 8
#define A1_ROOK_MOVED 16
#define H1_ROOK_MOVED 32
#define CASTLING_BITS 63
/* game_flags >> EN_PASSANT_SHIFT is the square of a pawn that just moved
   two squares forward and so may be captured en passant */
#define EN_PASSANT_SHIFT 6

char piece_letters[] = "KQRBNP .pnbrqk"; /* indexed by piece + 7 */
char back_rank[] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
/* material value by piece type (kings are never valued, they are captured) */
int material[] = {0, 99, 306, 297, 495, 855};
/* bonus for being close to the centre, by rank or file */
int centre_bonus[] = {0, 1, 2, 3, 3, 2, 1, 0};
int knight_steps[] = {21, 19, 12, 8, -8, -12, -19, -21};
/* the first four are diagonal steps, the last four are straight steps */
int king_steps[] = {11, 9, -9, -11, 10, -10, -1, 1};

int board[149];
/*
 * Inside search() the pieces of the side to move are (type ^ color), so
 * color is ~0 while white moves and 0 while black moves.  search() flips
 * color when it starts, therefore between searches color is 0 when white
 * is to move and ~0 when black is to move.
 */
int color;
int game_flags;
/* the move to play: the piece that ends on the target square (it differs
   from the moving piece on a promotion), its start and its target square */
int move_piece, move_from, move_to;
jmp_buf move_played;

/* the state of one search() call that try_move() needs */
struct node {
  int parent_gain; /* score of the parent move leading to this position */
  int ply;         /* distance from the root of the search              */
  int parent_best; /* best score of the parent so far; at the root it
                      is nonzero when the chosen move must be played    */
  int max_ply;     /* the search stops expanding at this ply            */
  int best;        /* best score found so far in this position          */
};

int search(int parent_gain, int ply, int parent_best, int max_ply);

int value_of(int piece) { return material[piece < 0 ? ~piece : piece]; }

int rank_centre(int square) { return centre_bonus[square / 10 - 2]; }

int file_centre(int square) { return centre_bonus[square % 10 - 1]; }

/*
 * Try moving the piece on `from` to `to` (for every promotion piece when it
 * is a pawn reaching the last rank), score the move and search the replies.
 * Returns 1 when search() must stop and return *result: a king can be
 * captured or the opponent will never allow this line (alpha-beta cutoff).
 */
int try_move(struct node *nd, int from, int to, int *result) {
  int captured = board[to];
  int piece, type, new_piece, first, last, saved_flags, pawn_step, gain;

  /* only an empty square or an opponent piece can be the target */
  if (captured != EMPTY && (captured ^ color) >= -1)
    return 0;

  piece = board[from];
  if (captured == ~KING || captured == KING) {
    *result = KING_CAPTURED - nd->ply;
    return 1;
  }

  board[from] = EMPTY;
  first = last = piece;
  if (piece == PAWN && to > 89) { /* black pawn promotes */
    first = KNIGHT;
    last = QUEEN;
  }
  if (piece == ~PAWN && to < 30) { /* white pawn promotes */
    first = ~QUEEN;
    last = ~KNIGHT;
  }

  for (new_piece = first; new_piece <= last; new_piece++) {
    saved_flags = game_flags;
    pawn_step = 0;
    game_flags &= CASTLING_BITS; /* any en passant chance is gone */
    type = piece ^ color;

    if (type == PAWN) {
      pawn_step = piece == PAWN ? to - from : from - to;
      gain = pawn_step / 8; /* 1 for a single step, 2 for a double step */
      if ((to - from) % 10 != 0 && captured == EMPTY)
        gain += 99; /* en passant capture */
      if (piece == PAWN ? to > 90 : to < 29)
        gain += value_of(new_piece); /* promotion */
    } else if (type == KING) {
      game_flags |= piece != KING ? WHITE_KING_MOVED : BLACK_KING_MOVED;
      gain = (from - to == 2) * 9 + 9 * (to - from == 2); /* castling */
    } else {
      /* prefer moving toward the centre and moving cheaper pieces */
      gain = rank_centre(to) - rank_centre(from) - (type - 2);
    }
    gain += value_of(captured) + file_centre(to) - nd->ply - file_centre(from);

    board[to] = new_piece;
    game_flags |= (from == 21 || to == 21) * A8_ROOK_MOVED +
                  (from == 28 || to == 28) * H8_ROOK_MOVED +
                  (from == 91 || to == 91) * A1_ROOK_MOVED +
                  (from == 98 || to == 98) * H1_ROOK_MOVED +
                  (pawn_step == 20) * (to << EN_PASSANT_SHIFT);

    /* the score of a move is what it gains minus the best reply */
    if (nd->max_ply > nd->ply)
      gain -= search(gain, nd->ply + 1, nd->best, nd->max_ply);

    /* the move to play was found and does not lose the king: keep it on
       the board and jump straight back to the game loop */
    if (nd->ply == 0 && nd->parent_best && new_piece == move_piece &&
        from == move_from && to == move_to && gain > -MATE_LIMIT)
      longjmp(move_played, 1);

    game_flags = saved_flags;
    if ((!nd->parent_best || nd->ply) &&
        (gain > nd->best ||
         (nd->ply == 0 && gain == nd->best && (rand() & 1)))) {
      if (nd->ply == 0) {
        if (nd->max_ply) {
          move_piece = new_piece;
          move_from = from;
          move_to = to;
        }
      } else if (nd->parent_gain - gain < nd->parent_best) {
        board[from] = piece;
        board[to] = captured;
        *result = gain;
        return 1;
      }
      nd->best = gain;
    }
  }
  board[to] = captured;
  board[from] = piece;
  return 0;
}

/* put the king of the side to move on `square` and check it is not attacked */
int square_is_safe(int king_square, int square) {
  int safe;

  board[king_square] = EMPTY;
  board[square] = KING ^ color;
  safe = MATE_LIMIT > search(0, 0, 0, 0);
  board[square] = EMPTY;
  board[king_square] = KING ^ color;
  return safe;
}

/*
 * Check whether the king may castle over `passed_square`.  The test of the
 * king's own square is made too, but its answer is thrown away: the original
 * code stores both answers in one variable, so only the last one counts.
 */
int castling_path_safe(int king_square, int passed_square) {
  square_is_safe(king_square, king_square);
  return square_is_safe(king_square, passed_square);
}

/*
 * Search every move of the side to move, max_ply - ply moves deep, and
 * return the best score.  At the root (ply 0) the best move is stored in
 * move_piece/move_from/move_to, or with parent_best set the move already
 * stored there is played if it is legal.
 */
int search(int parent_gain, int ply, int parent_best, int max_ply) {
  struct node nd = {parent_gain, ply, parent_best, max_ply, NO_MOVE_SCORE};
  int from, to, type, step, i, first_step, last_step, stop, result;
  int forward, en_passant;

  color = ~color;
  for (from = 98; from >= 21; from--) {
    type = board[from] ^ color;

    if (type == PAWN) {
      forward = color ? -10 : 10;
      en_passant = game_flags >> EN_PASSANT_SHIFT;
      to = from + forward;
      if (board[to] == EMPTY) {
        if (try_move(&nd, from, to, &result))
          goto stop_search;
        to += forward;
        if ((color ? from > 80 : from < 39) && board[to] == EMPTY)
          if (try_move(&nd, from, to, &result))
            goto stop_search;
      }
      /* capture to the left (as seen by white) */
      to = from + forward - 1;
      if (board[to] != EMPTY) {
        if (try_move(&nd, from, to, &result))
          goto stop_search;
      } else if (from - 1 == en_passant) {
        board[from - 1] = EMPTY;
        stop = try_move(&nd, from, to, &result);
        board[from - 1] = ~PAWN ^ color;
        if (stop)
          goto stop_search;
      }
      /* capture to the right */
      to = from + forward + 1;
      if (board[to] != EMPTY) {
        if (try_move(&nd, from, to, &result))
          goto stop_search;
      } else if (from + 1 == en_passant) {
        board[from + 1] = EMPTY;
        stop = try_move(&nd, from, to, &result);
        board[from + 1] = ~PAWN ^ color;
        if (stop)
          goto stop_search;
      }
    }

    if (type == KNIGHT)
      for (i = 0; i < 8; i++)
        if (try_move(&nd, from, from + knight_steps[i], &result))
          goto stop_search;

    if (type == BISHOP || type == ROOK || type == QUEEN) {
      first_step = type == ROOK ? 4 : 0;
      last_step = type == BISHOP ? 4 : 8;
      for (i = first_step; i < last_step; i++) {
        step = king_steps[i];
        to = from;
        do {
          to += step;
          if (try_move(&nd, from, to, &result))
            goto stop_search;
        } while (board[to] == EMPTY);
      }
    }

    if (type == KING) {
      for (i = 0; i < 8; i++)
        if (try_move(&nd, from, from + king_steps[i], &result))
          goto stop_search;

      /* castle queen side: the king may not pass over an attacked square */
      if ((color ? !(game_flags & (WHITE_KING_MOVED | A1_ROOK_MOVED))
                 : !(game_flags & (BLACK_KING_MOVED | A8_ROOK_MOVED))) &&
          max_ply && board[from - 1] == EMPTY && board[from - 2] == EMPTY &&
          board[from - 3] == EMPTY && castling_path_safe(from, from - 1)) {
        board[from - 1] = ROOK ^ color;
        board[from - 4] = EMPTY;
        stop = try_move(&nd, from, from - 2, &result);
        board[from - 1] = EMPTY;
        board[from - 4] = ROOK ^ color;
        if (stop)
          goto stop_search;
      }

      /* castle king side */
      if ((color ? !(game_flags & (WHITE_KING_MOVED | H1_ROOK_MOVED))
                 : !(game_flags & (BLACK_KING_MOVED | H8_ROOK_MOVED))) &&
          max_ply && board[from + 1] == EMPTY && board[from + 2] == EMPTY &&
          castling_path_safe(from, from + 1)) {
        board[from + 1] = ROOK ^ color;
        board[from + 3] = EMPTY;
        stop = try_move(&nd, from, from + 2, &result);
        board[from + 1] = EMPTY;
        board[from + 3] = ROOK ^ color;
        if (stop)
          goto stop_search;
      }
    }
  }
  color = ~color;
  return nd.best;

stop_search:
  color = ~color;
  return result;
}

void print_board(void) {
  int row, square, rank;

  puts("\n      A B C D E F G H\n");
  for (row = 20; row < 100; row += 10) {
    rank = '0' + 10 - row / 10;
    printf("   %c ", rank);
    for (square = row + 1; square <= row + 8; square++)
      printf(" %c", piece_letters[board[square] + 7]);
    printf("  %c \n", rank);
  }
  puts("\n      A B C D E F G H\n");
}

int main(int argc, char **argv) {
  int square, file, score, letter, type;

  (void)argv; /* only the number of arguments matters */
  for (square = 0; square < 130; square++)
    board[square] =
        square < 21 || square > 98 || square % 10 == 0 || square % 10 == 9
            ? BORDER
            : EMPTY;
  for (file = 0; file < 8; file++) {
    board[21 + file] = back_rank[file];
    board[31 + file] = PAWN;
    board[81 + file] = ~PAWN;
    board[91 + file] = ~back_rank[file];
  }

  for (;;) {
    print_board();

    /* no move keeps the king safe: checkmate or stalemate */
    if (-MATE_LIMIT > search(0, 0, 0, 1)) {
      color = ~color;
      puts(search(0, 0, 0, 0) > MATE_LIMIT ? color ? "I win" : "You win"
                                           : "Stalemate");
      break;
    }

    if (argc > 1 && color) {
      /* the computer plays black, one more ply for every argument */
      score = search(0, 0, 0, argc + 2);
      printf("*%c%c-%c%c (%d)\n", move_from % 10 + 'A' - 1,
             '0' + 10 - move_from / 10, move_to % 10 + 'A' - 1,
             '0' + 10 - move_to / 10, score);
    } else {
      /* read a move such as e2e4, or f7f8n to promote to a knight */
      putchar('>');
      move_from = (getchar() & 95) - 'A' + 21;
      move_piece = board[move_from += ('8' - getchar()) * 10];
      move_to = (getchar() & 95) - 'A' + 21;
      move_to += ('8' - getchar()) * 10;
      while ((letter = getchar() & 95) != '\n') {
        type = letter == 'N'   ? KNIGHT
               : letter == 'B' ? BISHOP
               : letter == 'R' ? ROOK
                               : QUEEN;
        move_piece = ~type ^ color;
      }
    }

    /* play the move if it is legal */
    if (!setjmp(move_played)) {
      search(0, 0, 1, 1);
      puts("Illegal");
    }
  }
  while (getchar() != '\n')
    ;
}
