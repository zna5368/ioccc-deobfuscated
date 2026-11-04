/*
 * Knight's tour using Warnsdorff's rule.
 *
 * The board is kept directly as the text that gets printed: 12 lines of
 * 30 characters. Every square takes 3 characters (a space followed by two
 * digits), so one line holds 10 squares: the 8 real squares plus a 2 square
 * border. The playable squares hold '\0' until the knight visits them, then
 * they hold the two digit move number. Border squares hold spaces, so they
 * always look "used" and the knight never jumps onto them.
 *
 * Starting points outside the board make the knight read (and write) memory
 * around the board, so for those the result depends on the variables that
 * happen to be stored next to it. The statements below keep the same order
 * of updates as the original so that those cases behave alike.
 */

#define BOARD_CHARS 366 /* 12 lines of 30 characters plus 6 */
#define LINE_CHARS 30   /* 10 squares of 3 characters */
#define SQUARE_CHARS 3

/*
 * The 8 knight jumps measured in characters of the text board. Moving one
 * square sideways is 3 characters, one line up or down is 30 characters:
 *   -63 = up 2, left 1      -57 = up 2, right 1
 *   -24 = up 1, right 2      36 = down 1, right 2
 *    63 = down 2, right 1    57 = down 2, left 1
 *    24 = down 1, left 2    -36 = up 1, left 2
 * The order matters: on a tie the first jump in the list wins.
 * The list ends with 0.
 */
const signed char knight_jumps[] = {-63, -57, -24, 36, 63, 57, 24, -36, 0};

char *current, board[BOARD_CHARS];
const signed char *neighbor_jump;
char *best_square, *square;
const signed char *jump;
int pos, move_number;

main(argc, argv) char **argv;
{
  int fewest;

  (void)argc;

  /* draw the empty board */
  for (pos = 0; pos < BOARD_CHARS; pos++) {
    int column = pos % LINE_CHARS;

    if (pos % SQUARE_CHARS != 0) {
      /* a digit of a square: playable squares are lines 2..9, squares 2..9 */
      if (60 < pos && pos < 300 && column > 6)
        board[pos] = 0;
      else
        board[pos] = ' ';
    } else if (column != 0) {
      board[pos] = ' '; /* gap in front of every square */
    } else {
      board[pos] = '\n'; /* start of every line */
    }
  }

  /*
   * The argument "RC" picks row R and column C (1..8), the first digit of
   * that square is found at line R+1, square C+1 of the text board.
   */
  best_square = board + SQUARE_CHARS * atoi(argv[1]) + LINE_CHARS + 4;

  for (;;) {
    jump = neighbor_jump = knight_jumps;
    current = best_square;
    if (!current)
      break; /* the knight is stuck: the tour is over */

    /* jump to the chosen square and write the move number into it */
    move_number++;
    *current++ = '0' + move_number / 10;
    best_square = 0;
    fewest = 0;
    *current = '0' + move_number % 10; /* current now is the ones digit */

    /*
     * Warnsdorff's rule: pick the free square reachable from here that has
     * the fewest free squares to continue from.
     */
    for (;;) {
      square = current - 1 + *jump;
      pos = *square;
      if (*jump++ == 0)
        break;

      /*
       * score = character on the square minus the number of its knight
       * neighbors that are already used (visited squares and border).
       * A free square starts at 0 and ends up negative, used squares hold
       * a digit or a space and always stay positive.
       */
      do
        pos -= square[*neighbor_jump++] != 0;
      while (*neighbor_jump);
      neighbor_jump = knight_jumps;

      if (pos < fewest) {
        fewest = pos;
        best_square = square;
      }
    }
  }

  puts(board);
}
