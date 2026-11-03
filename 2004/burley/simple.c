#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * A card is stored as (suit << 4) | rank, where rank is 0..12 for 2..A and
 * suit is 1..4 for clubs, diamonds, hearts and spades.
 */
#define HAND_SIZE 5
#define RANK_JACK 9
#define RANK_QUEEN 10
#define RANK_KING 11
#define RANK_ACE 12

static const char rank_names[] = "23456789TJQKA";
static const char suit_names[] = "BCDHS"; /* index 0 is never used */

char line[80];
int used[4];          /* per suit: bit mask of ranks already dealt this round */
int hand[HAND_SIZE];  /* hand[4] is shown first, hand[0] last */
int balance = 100;
int bet;

static int rank_of(int card) { return card & 15; }
static int suit_of(int card) { return card >> 4; }

/* deal a random card that has not been dealt yet this round */
static int deal_card(void) {
  int suit = (rand() >> 9) % 4;
  int rank;

  do {
    rank = (rand() >> 3) % 13;
  } while (used[suit] & (1 << rank));
  used[suit] |= 1 << rank;
  return rank | (suit + 1) << 4;
}

/*
 * Turn the user's list of positions to keep (digits 1-5) into a bit mask of
 * hand indexes.  Position 1 is hand[4], position 5 is hand[0].
 */
static int parse_keep(const char *keep) {
  int mask = 0;

  for (; *keep; keep++) {
    int position = *keep - '1';
    if (position >= 0 && position < HAND_SIZE)
      mask |= 1 << (HAND_SIZE - 1 - position);
  }
  return mask;
}

static void show_hand(void) {
  int n;

  for (n = HAND_SIZE - 1; n >= 0; n--) {
    putchar(rank_names[rank_of(hand[n])]);
    putchar(' ');
  }
  putchar('\n');
  for (n = HAND_SIZE - 1; n >= 0; n--) {
    putchar(suit_names[suit_of(hand[n])]);
    putchar(' ');
  }
  putchar('\n');
}

/* sort the hand by rank, highest first */
static void sort_hand(void) {
  int i, j, tmp;

  for (i = 0; i < HAND_SIZE - 1; i++)
    for (j = i + 1; j < HAND_SIZE; j++)
      if (rank_of(hand[j]) > rank_of(hand[i])) {
        tmp = hand[i];
        hand[i] = hand[j];
        hand[j] = tmp;
      }
}

/* payout multiplier for a hand sorted by sort_hand() */
static int payout(void) {
  int count[13] = {0};
  int pairs = 0, three = 0, four = 0, high_pair = 0;
  int flush = 1, straight = 1;
  int n, r;

  for (n = 0; n < HAND_SIZE; n++) {
    count[rank_of(hand[n])]++;
    if (suit_of(hand[n]) != suit_of(hand[0]))
      flush = 0;
  }

  /* each card one rank below the previous, or the wheel A 5 4 3 2 */
  for (n = 1; n < HAND_SIZE; n++) {
    int step = rank_of(hand[n - 1]) - rank_of(hand[n]);
    int wheel = n == 1 && rank_of(hand[0]) == RANK_ACE && rank_of(hand[1]) == 3;
    if (step != 1 && !wheel)
      straight = 0;
  }

  for (r = 0; r < 13; r++) {
    if (count[r] == 2) {
      pairs++;
      if (r >= RANK_JACK)
        high_pair = 1;
    }
    if (count[r] == 3)
      three = 1;
    if (count[r] == 4)
      four = 1;
  }

  if (straight && flush)
    return rank_of(hand[2]) == RANK_QUEEN ? 250 : 50; /* royal flush */
  if (four)
    return 25;
  if (three && pairs)
    return 9;
  if (flush)
    return 6;
  if (straight)
    return 4;
  if (three)
    return 3;
  if (pairs == 2)
    return 2;
  if (pairs == 1 && high_pair)
    return 1;
  return 0;
}

int main(int argc, char **argv) {
  srand(time(NULL));

  for (;;) {
    char *keep;
    int keep_mask, n, winnings;

    /* a blank or zero bet repeats the previous bet; the first bet is 1 */
    fgets(line, sizeof line, stdin);
    n = atoi(line + strspn(line, " -"));
    if (n)
      bet = n;
    if (!bet)
      bet = 1;

    /* first deal: all five cards */
    for (n = 0; n < HAND_SIZE; n++)
      hand[n] = deal_card();
    show_hand();

    /* draw: replace every card that is not kept */
    keep = fgets(line, sizeof line, stdin);
    keep_mask = parse_keep(keep);
    for (n = 0; n < HAND_SIZE; n++)
      if (!(keep_mask & (1 << n)))
        hand[n] = deal_card();
    show_hand();

    sort_hand();
    winnings = payout() * bet;
    balance += winnings - bet;
    printf("$%d (%d) \n", balance, winnings);

    /* put every card back in the deck */
    memset(used, 0, sizeof used);
  }
}
