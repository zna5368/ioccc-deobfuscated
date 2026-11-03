#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The maze has 15 caves numbered 1..15.  Cave 0 is the outside world, and
 * two numbers past the last cave mark the other ways the game can end.
 */
#define OUTSIDE 0
#define START_CAVE 1
#define EXIT_CAVE 15
#define LAST_CAVE 15
#define QUIT 16
#define EATEN 17

/*
 * Every cave has four doors (absolute directions 0..3).  A door is
 * identified by a "slot" number: cave * 4 + door.  passage[slot] holds the
 * slot at the other end of the passage, or 0 when the door is a wall.
 */
#define SLOT(cave, door) ((cave) * 4 | (door))
#define NUM_SLOTS 64

#define NUM_ITEMS 9
#define LAMP 0
#define CARRIED 0 /* item_cave[] value for an item in the inventory */

#define LAMP_SECONDS (60 + 60 * 4)

/* directions are relative to the way the player is facing */
static const char *direction_names[] = {"left", "forward", "right", "back"};

static const char *item_names[NUM_ITEMS] = {
    "lamp", "diadem", "harp", "kris", "chime",
    "ruby", "scepter", "talisman", "zither",
};

int lamp_deadline;      /* time the lamp runs out, 0 once it has died */
int cave = START_CAVE;  /* where the player is (or how the game ended) */
int facing;             /* absolute direction the player is facing */
int item_cave[NUM_ITEMS];
int need_look;          /* describe the cave at the start of the next turn */
char passage[NUM_SLOTS];

char line[99];
int verb; /* first letter of the first word typed */
int noun; /* first letter of the second word typed */

/* random number from 1 to n */
int roll(int n) { return rand() % n + 1; }

/* connect two doors with a passage if both are still walls */
int dig(int from, int to) {
  if (passage[from] || passage[to])
    return 0;
  passage[from] = to;
  passage[to] = from;
  return 1;
}

/* turn a direction relative to the player into an absolute door number */
int absolute_door(int relative) { return (relative + 3 + facing) & 3; }

/*
 * Nonzero when the cave is dark: the lamp has died, or it is neither carried
 * nor lying here.  (The arithmetic is kept from the original, including its
 * quirk that a dead lamp lying in cave 1 lights up cave 2.)
 */
int is_dark(void) {
  return !lamp_deadline + (item_cave[LAMP] - cave) * item_cave[LAMP];
}

/* "sun" during the day and "moon" at night */
const char *sky(void) {
  time_t now = time(0);
  int hour = localtime(&now)->tm_hour;
  return hour < 6 || hour > 19 ? "moon" : "sun";
}

/* skip spaces and control characters */
char *skip_blanks(char *p) {
  while (*p && *p < 33)
    p++;
  return p;
}

/* skip the letters of a word */
char *skip_word(char *p) {
  while (*p && *p != ' ')
    p++;
  return p;
}

/* read a line and remember the first letters of its first two words */
void read_command(void) {
  char *p;
  /* like the original this crashes at end of input (fgets returns NULL) */
  if (!*fgets(line, +98, stdin))
    exit(0);
  p = skip_blanks(line);
  verb = *p;
  p = skip_blanks(skip_word(p));
  noun = *p;
}

/* print the inventory, returning nonzero when something is carried */
int inventory(void) {
  int item, count = 0;
  printf("you have\n");
  for (item = 0; item < NUM_ITEMS; item++)
    if (item_cave[item] == CARRIED) {
      printf("   a %s\n", item_names[item]);
      count++;
    }
  if (!count)
    printf("   nothing\n");
  return count != 0;
}

/* move an item named by noun between the cave and the inventory */
void transfer(int getting) {
  int item;
  int from = getting ? cave : CARRIED;
  if (!(getting && is_dark()))
    for (item = NUM_ITEMS - 1; item >= 0; item--)
      if (item_cave[item] == from && noun == item_names[item][0]) {
        printf("done\n");
        item_cave[item] = getting ? CARRIED : cave;
        return;
      }
  printf("you do not %s that\n", getting ? "see" : "have");
}

void get_or_drop(int dropping) {
  if (!noun) {
    /* "drop" with nothing to drop just shows the empty inventory */
    if (dropping && !inventory())
      return;
    printf("%s what? ", dropping ? "drop" : "get");
    read_command();
    noun = verb;
  }
  transfer(!dropping);
}

void walk(int relative) {
  int other_end = passage[SLOT(cave, absolute_door(relative))];
  if (other_end) {
    printf("you climb...\n");
    cave = other_end / 4 & 63;
    facing = (other_end & 3) ^ 2; /* turn away from the door we came in */
    need_look = 1;
  } else {
    printf("eh?\n");
  }
  if (is_dark() && roll(6) / 6) {
    printf("chomp crunch grind slurp\noops.  you fed the grue\n");
    cave = EATEN;
  }
}

void look(void) {
  int relative, item, seen = 0;
  if (is_dark()) {
    printf("it is pitch black here\n");
  } else {
    printf("you are in a maze of twisty little passages\ncaves lead: ");
    for (relative = 0; relative < 4; relative++)
      if (passage[SLOT(cave, absolute_door(relative))])
        printf(" %s", direction_names[relative]);
    putchar('\n');
    for (item = 0; item < NUM_ITEMS; item++)
      if (item_cave[item] == cave) {
        if (!seen++)
          printf("you see\n");
        printf("   a %s\n", item_names[item]);
      }
  }
  if (cave == EXIT_CAVE)
    printf("%slight streams in ahead!\n", sky());
}

void turn(void) {
  int relative;

  if (lamp_deadline && lamp_deadline < time(0)) {
    if (!is_dark()) {
      printf("your lamp dies!\n");
      need_look = 1;
    }
    lamp_deadline = 0;
  }
  if (need_look) {
    need_look = 0;
    look();
  }

  printf("> ");
  read_command();

  if (verb == 'i') {
    inventory();
  } else if (verb == 'q') {
    cave = QUIT;
  } else if (verb == 'g' || verb == 'd') {
    get_or_drop(verb == 'd');
  } else {
    for (relative = 0; relative < 4; relative++)
      if (verb == direction_names[relative][0]) {
        walk(relative);
        return;
      }
    printf("eh?\n");
  }
}

/* try to dig a few random passages from each door in a range of slots */
void dig_shortcuts(int first, int limit, int tries) {
  int slot, attempt, dug;
  for (slot = first; slot < limit; slot++) {
    dug = 0;
    for (attempt = 0; attempt < tries; attempt++)
      dug = dig(slot, slot + roll(limit - slot + dug));
    tries ^= dug;
  }
}

void build_maze(void) {
  int c, entrance = 0;

  /* a chain of passages from cave 1 to the exit cave */
  for (c = START_CAVE; c < LAST_CAVE; c++) {
    entrance = SLOT(c + 1, 0) + roll(3);
    dig(SLOT(c, 0), entrance);
  }
  /* the door opposite the entrance of the last cave leads outside */
  passage[entrance ^ 2] = SLOT(OUTSIDE, 1);

  /* shortcuts that stay local so the exit is always far from the start */
  dig_shortcuts(4, 16, 2);
  dig_shortcuts(17, 35, 3);
  dig_shortcuts(36, 59, 6);
}

void scatter_items(void) {
  int item;
  for (item = NUM_ITEMS - 1; item > LAMP; item--)
    item_cave[item] = roll(LAST_CAVE);
  item_cave[LAMP] = START_CAVE;
}

int main() {
  int item, score, time_left;

  srand(lamp_deadline = LAMP_SECONDS + time(0));
  build_maze();
  need_look = 1;
  scatter_items();
  facing = 1;

  while (cave >= START_CAVE && cave <= LAST_CAVE)
    turn();

  if (cave == OUTSIDE) {
    printf("into bright %slight!\nscore: ", sky());
    /* 50 points for your hide plus 50 per item, and a bonus for speed */
    score = 50;
    for (item = 0; item < NUM_ITEMS; item++)
      if (item_cave[item] == CARRIED)
        score += 50;
    time_left = lamp_deadline - (int)time(0);
    if (lamp_deadline)
      score += time_left;
    printf("%d \n", score);
  }
  return 0;
}
