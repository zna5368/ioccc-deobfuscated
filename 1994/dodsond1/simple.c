/*
 * Othello / Reversi: the human plays 'O', the computer plays 'X'.
 *
 * The 8x8 board lives in rows/columns 1..8 of a 10x10 array so that the
 * outer ring of zero cells acts as a sentinel border.
 */

#define SIZE 8
#define EMPTY 0
#define HUMAN 1
#define COMPUTER (-1)
#define NUM_TURNS 60

/* flags for try_move() */
#define EVALUATE 1 /* only compute the value of the move */
#define PLAY (-1)  /* place the piece and flip the captured pieces */

int board[10][10] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, HUMAN, COMPUTER, 0, 0, 0, 0},
    {0, 0, 0, 0, COMPUTER, HUMAN, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

/* how valuable each square is: corners best, edges good */
int weight[10][10] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 9, 6, 6, 6, 6, 6, 6, 9, 0},
    {0, 6, 1, 2, 2, 2, 2, 1, 6, 0},
    {0, 6, 2, 3, 3, 3, 3, 2, 6, 0},
    {0, 6, 2, 3, 3, 3, 3, 2, 6, 0},
    {0, 6, 2, 3, 3, 3, 3, 2, 6, 0},
    {0, 6, 2, 3, 3, 3, 3, 2, 6, 0},
    {0, 6, 1, 2, 2, 2, 2, 1, 6, 0},
    {0, 9, 6, 6, 6, 6, 6, 6, 9, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

/* the eight compass directions as (row, column) steps */
int row_step[9] = {-1, -1, -1, 0, 0, 1, 1, 1, 0};
int col_step[9] = {-1, 0, 1, -1, 1, -1, 0, 1, 0};

/*
 * One function for four jobs: check if a move is legal, check if any move
 * exists (by summing), make a move, and score a move.
 *
 * Returns 0 for an illegal move, otherwise the weight of the target square
 * plus the weights of every captured piece.  When mode is PLAY the piece is
 * placed and the captured pieces are flipped (multiplied by -1).
 */
int try_move(int row, int col, int player, int mode)
{
    int value, dist, dir;

    if (board[row][col] != EMPTY)
        return 0;
    if (mode != EVALUATE)
        board[row][col] = player;

    value = weight[row][col];
    for (dir = 0; dir < 8; dir++) {
        int dr = row_step[dir], dc = col_step[dir];

        /* the neighbour must be an opponent piece */
        if (board[row + dr][col + dc] != -player)
            continue;

        /* walk over the line of opponent pieces */
        for (dist = 1; board[row + dist * dr][col + dist * dc] == -player; dist++)
            ;

        /* the line must end with one of our own pieces */
        if (board[row + dist * dr][col + dist * dc] == player) {
            for (dist--; dist; dist--) {
                board[row + dist * dr][col + dist * dc] *= mode;
                value += weight[row + dist * dr][col + dist * dc];
            }
        }
    }

    /* nothing captured: not a legal move */
    if (value == weight[row][col]) {
        board[row][col] = EMPTY;
        return 0;
    }
    return value;
}

print_board()
{
    int col, row;

    printf("\n");
    for (row = 1; row <= SIZE; row++) {
        printf("%c", 'A' + row - 1);
        for (col = 1; col <= SIZE; col++)
            printf("%c:", "X O<"[board[row][col] + 1]);
        printf("\n");
    }
    printf(" ");
    for (col = 1; col <= SIZE; col++)
        printf("%d ", col);
    printf("\n");
}

main()
{
    int turn, row, col, best, best_row, best_col, score;

    print_board();
    for (turn = 0; turn < NUM_TURNS; turn++) {
        /* human's turn, if any legal move exists */
        best = 0;
        for (row = 1; row <= SIZE; row++)
            for (col = 1; col <= SIZE; col++)
                best += try_move(row, col, HUMAN, EVALUATE);

        if (best) {
            do {
                printf("\n?");
                do {
                    row = getchar() - ('A' - 1);
                } while (row < 1 || row > SIZE);
                do {
                    col = getchar() - '0';
                } while (col < 1 || col > SIZE);
            } while (!try_move(row, col, HUMAN, PLAY));
            print_board();
        }

        /* computer's turn: best scoring square plus a little randomness */
        best = 0;
        for (row = 1; row <= SIZE; row++)
            for (col = 1; col <= SIZE; col++)
                if ((score = try_move(row, col, COMPUTER, EVALUATE) + random() % 6) > best) {
                    best = score;
                    best_row = row;
                    best_col = col;
                }

        if (best && try_move(best_row, best_col, COMPUTER, PLAY))
            print_board();
    }
}
