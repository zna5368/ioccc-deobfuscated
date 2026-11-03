#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * The insult words live on an 11x11 grid whose centre is (0,0).  Three rings
 * around the centre each hold one kind of word:
 *
 *   inner disc   0     < r < 3      containers   ("bucket", "jar", ...)
 *   middle ring  3     < r < 4.125  contents     ("tripe", "vermin", ...)
 *   outer ring   4.125 < r          adjectives   ("diseased", "vile", ...)
 *
 * Cells that fall exactly on a ring border (r == 3) are never chosen; they
 * just hold filler words.
 */
#define GRID_SIZE 11
#define GRID_HALF 5

static const char *grid[GRID_SIZE][GRID_SIZE] = {
    /* y = -5 */
    {"torporous", "translucent", "uneducated", "ungodly", "glorified",
     "grecian", "indigent", "inebriated", "obscene", "pilated", "worm-ridden"},
    /* y = -4 */
    {"affectionate", "unpleasant", "draconian", "flaccid", "programmers",
     "angst", "cadavers", "barbaric", "bestial", "over-ripe", "domesticated"},
    /* y = -3 */
    {"layered", "lewd", "boil-covered", "amoebas", "effluvium", "bladder",
     "lycanthropes", "mendicants", "rotting", "well-endowed", "promiscuous"},
    /* y = -2 */
    {"puss-ridden", "putrescent", "prostitutes", "bowel", "bib", "colon",
     "jar", "mouthful", "curmudgeons", "satanic", "pathetic"},
    /* y = -1 */
    {"pestilent", "obscurity", "parasites", "bowl", "bucket", "drainage",
     "dumpster", "carton", "supplicants", "sycophants", "vile"},
    /* y =  0 */
    {"platonic", "thieves", "vial", "box", "bucket", "cartload", "carton",
     "tub", "emanation", "tripe", "parasitic"},
    /* y =  1 */
    {"world-class", "whores", "misery", "exhalation", "congress", "discharge",
     "bag", "cartload", "pollywogs", "miasma", "violated"},
    /* y =  2 */
    {"virulent", "rank", "corpses", "sack", "scrotum-full", "sewer", "teapot",
     "tube", "filth", "well-lubricated", "doleful"},
    /* y =  3 */
    {"pre-pubescent", "steaming", "stratified", "grief", "infestation", "urn",
     "poxes", "pustules", "supine", "scum-encrusted", "slack-jawed"},
    /* y =  4 */
    {"sloppy", "thieving", "bulbous", "colorful", "refuse", "vermin", "waste",
     "crass", "cruel", "rigid", "riotous"},
    /* y =  5 */
    {"scatological", "diseased", "odorous", "insignificant", "laminated",
     "whelpish", "opaque", "ornamental", "limp", "luminous", "decrepit"},
};

/* Ring borders (distance from the centre of the grid). */
#define CONTAINER_INNER 0.0
#define CONTAINER_OUTER 3.0
#define CONTENTS_INNER 3.0
#define CONTENTS_OUTER 4.125
#define ADJECTIVE_INNER 4.125
#define ADJECTIVE_OUTER 9.0

/*
 * A random number from 0 to about 1.727 when RAND_MAX is 32767.  (With a
 * larger RAND_MAX it is much bigger, but points that miss the grid are
 * simply thrown away, so the program still works, only slower.)
 */
double random_unit(void) { return (double)rand() / 18972; }

/*
 * Pick a random point using polar coordinates until it lands on the grid
 * inside the ring inner < r < outer, and return the word in that cell.
 * The coordinates are kept as char exactly like the original so that the
 * same random numbers give the same word.
 */
const char *random_word(double inner, double outer) {
  double angle, radius, distance;
  char x, y;

  for (;;) {
    angle = random_unit() * 6.283184;     /* about 2 * pi */
    radius = sqrt(random_unit() * 72);
    y = sin(angle) * radius;
    x = cos(angle) * radius;

    if (fabs(x) > GRID_HALF || fabs(y) > GRID_HALF)
      continue; /* missed the grid */

    distance = sqrt(x * x + y * y);
    if (distance > inner && distance < outer)
      break;
  }
  return grid[y + GRID_HALF][x + GRID_HALF];
}

int main(void) {
  const char *adjective, *container, *contents;

  srand(time(0));

  /*
   * The original picked the words inside the printf() argument list, where
   * this compiler evaluates them from right to left.
   */
  contents = random_word(CONTENTS_INNER, CONTENTS_OUTER);
  container = random_word(CONTAINER_INNER, CONTAINER_OUTER);
  adjective = random_word(ADJECTIVE_INNER, ADJECTIVE_OUTER);

  printf("you are a %s %s of %s\n", adjective, container, contents);
  return 0;
}
