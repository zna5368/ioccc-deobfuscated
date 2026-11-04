#include <X11/Xlib.h>

/*
 * Graphical X11 chess program.
 *
 * Board
 * -----
 * The board is a "mailbox" of 10 columns by 12 rows (120 squares). The real
 * 8x8 board sits in rows 2..9 and columns 1..8, the rest is a border that is
 * marked OFF_BOARD so pieces can never move onto it. Square 21 is the top
 * left corner (a8) and square 98 is the bottom right corner (h1). Moving one
 * row down adds 10, moving one column right adds 1.
 *
 * Black (computer by default) starts at the top, White at the bottom.
 *
 * Pieces
 * ------
 * A black piece is stored as its type (PAWN..KING = 1..6). A white piece is
 * stored as the bitwise complement of its type (~type = -type - 1, so -2..-7).
 * A side is BLACK (0) or WHITE (-1, all bits set), which means
 *
 *     piece ^ side
 *
 * turns a piece of that side into its plain type (1..6). A piece of the other
 * side turns into a value of -2 or less, an empty square into 0 or -1 and the
 * border into -1 or 0. So "piece ^ side > 0" is "my piece" and
 * "piece ^ side < -1" is "enemy piece".
 *
 * Side to move
 * ------------
 * search() flips the global "side" on entry, so inside search() it is the
 * side that is moving. Outside search() it is the side that moved last (it
 * starts as BLACK, so White moves first). When search() actually plays a
 * move it returns without flipping back, which hands the turn over.
 *
 * Note: the original keeps the board in a "long" array but moves the rook
 * (castling) and removes en passant pawns through "int" pointers. On 64-bit
 * systems that only writes half of a square, which corrupts the board as
 * soon as castling is considered (a phantom king shows up on g1/g8). This
 * version uses an int board, which is how the program was written to work.
 */

#define BLACK 0
#define WHITE -1

#define EMPTY 0
#define OFF_BOARD -1
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

/* game flags: which kings and rooks have moved, plus the en passant square */
#define BLACK_KING_MOVED 1
#define ROOK_A8_MOVED 2
#define ROOK_H8_MOVED 4
#define WHITE_KING_MOVED 8
#define ROOK_A1_MOVED 16
#define ROOK_H1_MOVED 32
#define NO_CASTLING 63     /* all of the above */
#define EN_PASSANT_SHIFT 6 /* square of a pawn that just moved two squares */

#define MATE_SCORE 10000000 /* score for capturing the king */
#define WIN_LIMIT 1000000   /* scores beyond this mean a king was lost */
#define NO_MOVE -100000000  /* best score before any move was found */

#define SQUARE_PIXELS 64

/* piece types in the order they stand on the back rank */
const int back_rank[8] = {ROOK, KNIGHT, BISHOP, QUEEN,
                          KING, BISHOP, KNIGHT, ROOK};

/*
 * Bonus for being close to the centre, by row (1..8) and by column (1..8).
 */
const int centre_bonus[10] = {0, 0, 1, 2, 3, 3, 2, 1, 0, 0};

/*
 * Material value of the piece on a square, indexed by board value + 7, so
 * white pieces come first (king down to pawn), then border, empty and the
 * black pieces (pawn up to king). Kings are worth 0 here because capturing
 * a king is handled separately. The scores are multiplied by 9 when used.
 */
const int piece_value[14] = {
    0,  94, 55, 33, 34, 11, /* white king, queen, rook, bishop, knight, pawn */
    0,  0,                  /* border, empty */
    11, 34, 33, 55, 94, 0,  /* black pawn, knight, bishop, rook, queen, king */
};
#define VALUE(square_value) (piece_value[(square_value) + 7] * 9)

/*
 * Move directions. Each piece type uses the slice first_direction[type] up to
 * (but not including) end_direction[type]. Slot 0 is used for white pawns,
 * which move up the board, slot PAWN for black pawns, which move down. The
 * first two pawn directions are the diagonal captures, the last two the
 * single and double step. The first two king directions are left and right,
 * the only ones that can turn into castling.
 */
const int directions[22] = {
    -21, -19, -8,  -12, 8, 12, 19, 21, /* knight */
    -1,  1,   -10, 10,                 /* rook (and queen, king) */
    -11, -9,                           /* bishop (and queen, king) */
    9,   11,                           /* also black pawn captures */
    10,  20,                           /* black pawn steps */
    -11, -9,  -10, -20,                /* white pawn */
};
/* slots: white pawn, black pawn, knight, bishop, rook, queen, king */
const int first_direction[7] = {18, 14, 0, 12, 8, 8, 8};
const int end_direction[7] = {22, 18, 8, 16, 12, 16, 16};

/* the glyphs of the pieces, 8 by 8 "big pixels" of 8 by 8 screen pixels */
const char *glyphs[7][8] = {
    {"........", "........", "........", "........", /* empty */
     "........", "........", "........", "........"},
    {"........", "........", "...##...", "..####..", /* pawn */
     "..####..", "...##...", "..####..", "........"},
    {"........", ".###....", ".#.##...", ".#####..", /* knight */
     ".##.###.", "...####.", "..#####.", "........"},
    {"........", "...##...", "..####..", "..####..", /* bishop */
     "..####..", "...##...", ".##..##.", "........"},
    {"........", ".#.##.#.", ".######.", "..####..", /* rook */
     "..####..", ".######.", ".######.", "........"},
    {"........", ".#.##.#.", ".#.##.#.", "..#..#..", /* queen */
     "..####..", "..####..", "..####..", "........"},
    {"........", "..####..", ".##.#.#.", ".#.#.##.", /* king */
     "..####..", "..####..", "..####..", "........"},
};

/*
 * Background bit patterns of the four kinds of square, one byte for the even
 * and one for the odd pixel lines: light, dark, and selected (twice).
 */
const unsigned char square_pattern[4][2] = {
    {0x55, 0xAA}, {0x55, 0x00}, {0x44, 0x44}, {0x44, 0x44}};

int board[120];
int side;       /* see the comment at the top */
int game_flags; /* castling and en passant state of the real game */

/* the move to play: set by the mouse for the human, by search() otherwise */
int move_from, move_to, move_piece;

Display *display;
Window window;
GC gc;
Pixmap images[4][14]; /* [kind of square][board value + 7] */

int search(int only_square, int parent_score, int ply, int flags, int alpha,
           int max_ply);

/*
 * Search every move of the side to move and return the best score.
 *
 * only_square   when not 0, only moves that capture on this square are
 *               searched (used to look at recaptures beyond max_ply)
 * parent_score  score the opponent got for the move that led here
 * ply           how deep we are, 0 for the real position
 * flags         castling and en passant state (see game_flags)
 * alpha         best score of the parent so far, for alpha-beta cutoffs.
 *               At ply 0 it is a mode: 1 means "find the best move and store
 *               it in move_from/move_to/move_piece", 0 means "play the move
 *               in move_from/move_to/move_piece if it is legal".
 * max_ply       search depth
 */
int search(int only_square, int parent_score, int ply, int flags, int alpha,
           int max_ply) {
  int from, to, type, dir, end, piece, captured, new_piece, last_piece;
  int distance, score, new_flags, best = NO_MOVE;
  int *en_passant_pawn, *rook;

  side = ~side;

  for (from = 99; from > 20; from--) {
    piece = board[from];
    type = piece ^ side;
    if (type <= 0)
      continue; /* not one of my pieces */

    if (type == PAWN && side == WHITE) {
      dir = first_direction[0];
      end = end_direction[0];
    } else {
      dir = first_direction[type];
      end = end_direction[type];
    }

    to = from;
    do {
      rook = 0;
      to += directions[dir];
      captured = board[to];

      if ((!only_square || to == only_square) &&
          (type != PAWN || dir < end - 2 || captured == EMPTY)) {
        distance = abs(from - to);

        /* a pawn landing behind an enemy pawn that just moved two squares */
        if (type == PAWN &&
            flags >> EN_PASSANT_SHIFT == to + (side == BLACK ? -10 : 10))
          en_passant_pawn = &board[flags >> EN_PASSANT_SHIFT];
        else
          en_passant_pawn = 0;

        if ((captured == EMPTY &&
             (type != PAWN || distance % 10 == 0 || en_passant_pawn)) ||
            (captured ^ side) < -1) {

          /* pawns reaching the last row promote to knight..queen in turn */
          if (type == PAWN && (to > 89 || to < 30)) {
            new_piece = KNIGHT ^ side;
            last_piece = QUEEN ^ side;
          } else {
            new_piece = piece;
            last_piece = piece;
          }

          if ((captured ^ side) < -6) {
            side = ~side;
            return MATE_SCORE - 811 * ply; /* king captured */
          }

          for (;;) {
            /* make the move */
            board[from] = EMPTY;
            board[to] = new_piece;
            if (rook) {
              *en_passant_pawn = *rook; /* castling: the rook jumps over */
              *rook = EMPTY;
            } else if (en_passant_pawn) {
              *en_passant_pawn = EMPTY;
            }

            new_flags = flags & NO_CASTLING;
            if (from == 91 || to == 91)
              new_flags |= ROOK_A1_MOVED;
            if (from == 28 || to == 28)
              new_flags |= ROOK_H8_MOVED;
            if (from == 21 || to == 21)
              new_flags |= ROOK_A8_MOVED;
            if (from == 98 || to == 98)
              new_flags |= ROOK_H1_MOVED;

            /* score of the move */
            if (type == PAWN) {
              if (distance == 20)
                new_flags |= to << EN_PASSANT_SHIFT;
              if (new_piece != piece)
                score = VALUE(new_piece);
              else
                score = distance / 8 + (en_passant_pawn ? 99 : 0);
            } else if (type == KING) {
              new_flags |= side == WHITE ? WHITE_KING_MOVED : BLACK_KING_MOVED;
              score = rook ? 9 : 0;
            } else {
              score = centre_bonus[to / 10 - 1] - centre_bonus[from / 10 - 1] -
                      type + 2;
            }
            score += VALUE(captured) + centre_bonus[to % 10] - ply -
                     centre_bonus[from % 10];

            /* subtract the best answer of the opponent */
            if (max_ply > ply || (max_ply == ply && score > 49 && max_ply > 1))
              score -= search(max_ply > ply ? 0 : to, score, ply + 1, new_flags,
                              best, max_ply);

            /* is it the move we were asked to play, and is it legal? */
            if (from == move_from && new_piece == move_piece && ply == 0 &&
                to == move_to && alpha == 0 && score >= -WIN_LIMIT)
              return game_flags = new_flags;

            /* take the move back */
            board[from] = piece;
            board[to] = captured;
            if (rook) {
              *rook = *en_passant_pawn;
              *en_passant_pawn = EMPTY;
            } else if (en_passant_pawn) {
              *en_passant_pawn = PAWN ^ ~side;
            }

            if ((alpha != 0 || ply != 0) &&
                (score > best || (ply == 0 && best == score && (rand() & 4)))) {
              best = score;
              if (ply == 0 && max_ply) {
                move_piece = new_piece;
                move_from = from;
                move_to = to;
              }
              if (ply && parent_score - score < alpha) {
                side = ~side;
                return score; /* the parent will not choose this line */
              }
            }

            /*
             * A king step to the left or right can be repeated as castling:
             * reuse en_passant_pawn as the square the rook lands on.
             */
            if (type == KING && distance < 2 &&
                dir < first_direction[KING] + 2) {
              int needed;

              en_passant_pawn = &board[to];
              to += to - from;
              captured = board[to];
              rook = to < from ? en_passant_pawn - 3 : en_passant_pawn + 2;
              needed = to < from ? BLACK_KING_MOVED | ROOK_A8_MOVED
                                 : BLACK_KING_MOVED | ROOK_H8_MOVED;
              if (side == WHITE)
                needed <<= 3;
              if (!(flags & needed || *en_passant_pawn || captured ||
                    rook[to < from ? 1 : -1]) &&
                  score > -WIN_LIMIT &&
                  search(0, 0, 0, NO_CASTLING, 1, 0) < WIN_LIMIT) {
                distance++;
                continue; /* not in check, path free: castle */
              }
            }

            if (new_piece == last_piece)
              break;
            new_piece += side | 1; /* next promotion piece */
          }
        }
      }

      /* the double step only from the start row and over an empty square */
      if (type == PAWN && dir == end - 2 &&
          ((side == WHITE ? from < 80 : from > 39) || captured))
        dir++;

      /* bishops, rooks and queens slide on until they hit something */
      if (captured != EMPTY || type < BISHOP || type > QUEEN) {
        to = from;
        dir++;
      }
    } while (dir < end);
  }

  side = ~side;
  return best != NO_MOVE ? best : 0;
}

/* put the pieces on their starting squares */
void setup_board(void) {
  int square, column;

  for (square = 0; square < 120; square++) {
    if (square > 98 || square < 21 || square % 10 == 9 || square % 10 == 0)
      board[square] = OFF_BOARD;
    else
      board[square] = EMPTY;
  }
  for (column = 1; column <= 8; column++) {
    board[80 + column] = PAWN ^ WHITE;
    board[20 + column] = back_rank[column - 1];
    board[90 + column] = back_rank[column - 1] ^ WHITE;
    board[30 + column] = PAWN;
  }
}

void draw_board(void) {
  int row, column, square, kind;

  for (row = 0; row < 8; row++) {
    for (column = 0; column < 8; column++) {
      square = row * 10 + column + 21;
      kind = ((row + column) & 1) + 2 * (square == move_from);
      XCopyPlane(display, images[kind][board[square] + 7], window, gc, 0, 0,
                 SQUARE_PIXELS, SQUARE_PIXELS, column * SQUARE_PIXELS,
                 row * SQUARE_PIXELS, 1);
    }
  }
  XFlush(display);
}

/* turn the glyphs into 56 bitmaps: 4 kinds of square x 14 board values */
void make_images(void) {
  unsigned char bits[SQUARE_PIXELS * SQUARE_PIXELS / 8], *byte;
  int kind, value, type, row, line, column;
  unsigned char ink;

  for (kind = 0; kind < 4; kind++) {
    for (value = 0; value < 14; value++) {
      /* value is board value + 7: 0..5 white king..pawn, 8..13 black */
      if (value < 7) {
        type = value == 6 ? EMPTY : KING - value;
        ink = 0xFF; /* white pieces are drawn with set bits */
      } else {
        type = value - 7;
        ink = 0x00; /* black pieces are drawn with clear bits */
      }
      byte = bits;
      for (row = 0; row < 8; row++)
        for (line = 0; line < 8; line++)
          for (column = 0; column < 8; column++)
            *byte++ = glyphs[type][row][column] == '#'
                          ? ink
                          : square_pattern[kind][line & 1];
      images[kind][value] = XCreateBitmapFromData(display, window, (char *)bits,
                                                  SQUARE_PIXELS, SQUARE_PIXELS);
    }
  }
}

void computer_move(int depth) {
  search(0, 0, 0, game_flags, 1, depth); /* find the best move */
  search(0, 0, 0, game_flags, 0, 1);     /* and play it */
}

main(argc, argv) char **argv;
{
  XGCValues values;
  XEvent event;
  int depth, square, last_side;

  display = XOpenDisplay("");
  window = XCreateSimpleWindow(display, DefaultRootWindow(display), 64, 64,
                               8 * SQUARE_PIXELS, 8 * SQUARE_PIXELS, 2,
                               BlackPixel(display, DefaultScreen(display)),
                               BlackPixel(display, DefaultScreen(display)));
  XSelectInput(display, window, ExposureMask | ButtonPressMask);
  XMapWindow(display, window);
  values.foreground = WhitePixel(display, DefaultScreen(display));
  gc = XCreateGC(display, window, GCForeground, &values);

  make_images();

  srand(time(0));
  move_from = game_flags = 0;
  setup_board();

  /* ./toledo3 [level [b]]: search depth is 3 + level */
  if (argc > 1)
    depth = argv[1][0] - '0' + 3;
  if (argc > 2) {
    /* human plays black: the computer makes the first move */
    computer_move(depth);
    move_from = 0;
  }

  for (;;) {
    XNextEvent(display, &event);

    if (event.type == Expose)
      draw_board();

    if (event.type == ButtonPress) {
      square = event.xbutton.y / SQUARE_PIXELS * 10 +
               event.xbutton.x / SQUARE_PIXELS + 21;

      if ((board[square] ^ side) < -1) {
        /* clicked one of the pieces of the side to move: select it */
        move_from = square;
        draw_board();
      } else {
        /* clicked somewhere else: try to move the selected piece there */
        move_to = square;
        move_piece = board[move_from];
        if ((move_to < 29 || move_to > 90) && move_piece == (PAWN ^ ~side))
          move_piece = QUEEN ^ ~side; /* humans always promote to a queen */
        last_side = side;
        search(0, 0, 0, game_flags, 0, 1);
        move_from = 0;
        draw_board();

        if (argc > 1 && side != last_side) /* the move was legal */
          computer_move(depth);
        move_from = 0;
        draw_board();
      }
    }
  }
}
