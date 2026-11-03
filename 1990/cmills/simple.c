#include <stdio.h>
#include <time.h>

/*
 * Settings supplied by the Makefile with -D:
 *   M - the maximum bet (also used as the size of the input line buffer)
 *   N - the number of cards in the shoe (52 * number of decks)
 *   X - the statement used to seed rand()
 */
#define MAX_BET M
#define LINE_SIZE M
#define SHOE_SIZE N
#define SEED_RANDOM() X

void exit();

/* Card ranks: Ace, 2-9, Ten, Jack, Queen, King */
char ranks[] = "A23456789TJQK";

char *dealer_end;           /* end of the dealer's hand string */
char shoe[SHOE_SIZE];       /* the shuffled cards */
char dealer_hand[SHOE_SIZE];
/*
 * All of the player's hands are kept in one string. After splits it looks
 * like "W W H1 H2 CUR": a "W " for every split card still waiting for its
 * own hand, the hands already played, and the hand being played at the end.
 */
char player_hands[SHOE_SIZE];
char *player_end;           /* end of the player's hand string */
char *next_card = shoe + SHOE_SIZE; /* starts "empty" so the shoe is shuffled */
char line[LINE_SIZE];
char dealer_shown[] = "??"; /* dealer's up card and the hidden hole card */

/*
 * Hand totals: a positive total is a "hard" hand. A negative total is a
 * "soft" hand which holds an ace counted as 11, so -17 is soft 17.
 */
int cash = MAX_BET * 2;
int player_total;
int dealer_total;
int splits_waiting;
int doubled;  /* 1 if the current hand was doubled down */
int winnings; /* money returned to the player at the end of the round */

/* Ask a yes/no question until the answer starts with y, Y, n or N */
int ask(char *question)
{
    for (;;) {
        printf(question);
        if (!fgets(line, LINE_SIZE, stdin))
            exit(0);
        if (line[0] == 'y' || line[0] == 'Y')
            return 1;
        if (line[0] == 'n' || line[0] == 'N')
            return 0;
    }
}

/* Value of a card: ace is 1, 2-9 are their number, T J Q K are 10 */
int card_value(int card)
{
    char *rank;

    for (rank = ranks; *rank != 'J';)
        if (card == *rank++)
            break;
    return rank - ranks;
}

/* Add a card to a hand total, keeping track of soft hands */
int add_card(int total, int card)
{
    int value = card_value(card);

    if (total < 0) {
        /* soft hand: total is minus the score with the ace counted as 11 */
        total = value - total;
        if (total < 22)
            total = -total;     /* still soft */
        else
            total -= 10;        /* count the ace as 1 instead */
    } else if (value == 1) {
        /* first ace in the hand: try counting it as 11 */
        total += 11;
        if (total < 22)
            total = -total;
        else
            total -= 10;
    } else {
        total += value;
    }
    return total;
}

void dealer_hit(void)
{
    dealer_total = add_card(dealer_total, *dealer_end++ = *next_card++);
    *dealer_end = '\0';
}

void player_hit(void)
{
    player_total = add_card(player_total, *player_end++ = *next_card++);
    *player_end = '\0';
}

void show(char *dealer)
{
    printf("Dealer: %s\tPlayer: %s\n", dealer, player_hands);
}

void shuffle(void)
{
    int i, j, swap;

    printf("Shuffle...\n");
    for (i = 0; i < SHOE_SIZE; i++)
        shoe[i] = ranks[i % 13];
    for (i = 0; i < SHOE_SIZE; i++) {
        j = rand() % SHOE_SIZE;
        swap = shoe[j];
        shoe[j] = shoe[i];
        shoe[i] = swap;
    }
    next_card = shoe;
}

int read_bet(void)
{
    int bet;

    for (;;) {
        printf("Total $%d.  Wager?", cash);
        if (!fgets(line, LINE_SIZE, stdin) || (bet = atoi(line)) < 1)
            exit(0);
        if (bet & 1 || bet > MAX_BET || bet > cash)
            printf("Even bets <= $%d only.\n", cash < MAX_BET ? cash : MAX_BET);
        else
            return bet;
    }
}

/*
 * Split the current hand: its last card is moved to the front of the string
 * (followed by a space) to wait for its own hand later.
 */
void split_hand(void)
{
    int i;
    int end = player_end - player_hands;

    for (i = end; i >= 0; i--)
        player_hands[i + 2] = player_hands[i];
    player_hands[0] = player_hands[end + 1];
    player_hands[1] = ' ';
    player_end = player_hands + end + 1;
}

/*
 * Start the next split hand: move the first waiting card from the front of
 * the string to the end, where it becomes the first card of the new hand.
 */
void next_split_hand(void)
{
    *player_end++ = ' ';
    *player_end++ = player_hands[0];
    *player_end = '\0';
    for (player_end = player_hands; (*player_end = player_end[2]) != '\0';)
        player_end++;
}

/*
 * Deal the second card to a split hand. Every split hand starts with the
 * same rank as the first card in the string. Returns true for split aces,
 * which only get one card. (Like the split test, -player_end[1] is the
 * original's -1[e], so the "unless another ace" part is always true.)
 */
int deal_split_card(void)
{
    player_total = add_card(add_card(0, player_hands[0]),
                            *player_end++ = *next_card++);
    *player_end = '\0';
    show(dealer_shown);
    return player_hands[0] == 'A' && -player_end[1] != player_hands[0];
}

/* Add the payout of the current hand to the winnings */
void settle_hand(int bet)
{
    if (dealer_total < 0)
        dealer_total = -dealer_total;
    if (player_total < 0)
        player_total = -player_total;
    if (player_total < 22) {
        if (player_total == dealer_total)
            winnings += bet * ++doubled;        /* push: bet returned */
        else if (dealer_total > 21 || player_total > dealer_total)
            winnings += bet * 2 * ++doubled;    /* win: paid even money */
    }
}

int main(int argc, char **argv)
{
    int bet;
    int one_card_only;

    SEED_RANDOM();
    if (--argc)
        cash = atoi(argv[1]);

    while (cash > 1) {
        if (next_card - shoe > SHOE_SIZE * 3 / 4)
            shuffle();

        bet = read_bet();
        cash -= bet;
        splits_waiting = doubled = winnings = 0;
        dealer_total = player_total = 0;
        dealer_end = dealer_hand;
        player_end = player_hands;

        dealer_hit();
        dealer_hit();
        player_hit();
        player_hit();
        dealer_shown[0] = dealer_hand[0];
        show(dealer_shown);

        /* Insurance pays 2 to 1 if the dealer has blackjack */
        if (dealer_hand[0] == 'A' && cash >= bet / 2 && ask("Insurance?")) {
            if (card_value(dealer_hand[1]) == 10)
                cash += bet;
            else
                cash -= bet / 2;
        }

        if (dealer_total == -21) {
            /* dealer blackjack */
            settle_hand(bet);
        } else if (player_total == -21) {
            /* player blackjack pays 3 to 2 */
            cash += bet / 2;
            settle_hand(bet);
        } else {
            /* dealer plays the hidden hand first, standing on soft 17 */
            while (dealer_total > -17 && dealer_total < 17)
                dealer_hit();

            one_card_only = 0;
            for (;;) {
                /*
                 * A hard 20 (two ten-valued cards) can be split. The second
                 * test looks like "is this a pair", but the original wrote
                 * it as -1[e], which means -(e[1]) and not e[-1], so it
                 * never matches a card and only a hard 20 can be split.
                 */
                while (!one_card_only &&
                       (player_total == 20 ||
                        player_hands[0] == -player_end[1]) &&
                       cash >= bet && ask("Split?")) {
                    cash -= bet;
                    ++splits_waiting;
                    split_hand();
                    one_card_only = deal_split_card();
                }

                if (!one_card_only) {
                    doubled = cash >= bet && ask("Double?");
                    if (doubled) {
                        cash -= bet;
                        player_hit();
                    }
                    while (!doubled && player_total < 22 && ask("Hit?")) {
                        player_hit();
                        if (player_total < 22)
                            show(dealer_shown);
                    }
                }

                settle_hand(bet);
                if (!splits_waiting--)
                    break;
                next_split_hand();
                one_card_only = deal_split_card();
            }
        }

        cash += winnings;
        *player_end = '\0';
        show(dealer_hand);
    }
}
