#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<time.h>
#include<stdint.h>

/*
 * Compile time configuration. The macro names M, S and N are kept so that
 * the Makefile variables MAX_ATTEMPTS, MAX_SIZE and OFFSPRING still work.
 */

/* M: maximum number of generations before giving up (at least 100) */
#ifndef M
#define M SIZE_MAX-1
#elif M < 100
#undef M
#define M SIZE_MAX-1
#endif
#define MAX_ATTEMPTS (M)

/* S: size of the target buffer (at least 38, at most M) */
#ifndef S
#define S 38
#elif S < 38
#undef S
#define S 38
#elif S > M
#undef S
#define S M
#endif
#define TARGET_SIZE (S)
#define MAX_TARGET_LEN (TARGET_SIZE - 1)

/* N: number of offspring per generation (below 4 becomes 3, at most M) */
#ifndef N
#define N 25
#elif N < 4
#undef N
#define N 3
#elif N > M
#undef N
#define N M
#endif
#define OFFSPRING (N)

/* Easter egg target used when compiled with N=97: 97# (the primorial of 97) */
#define PRIMORIAL_97 "2305567963945518424753102147331756070"

/* the string being evolved towards */
static char target[TARGET_SIZE] = "METHINKS IT IS LIKE A WEASEL";

/* every character the monkey's typewriter can type */
static const char keyboard[] =
    " !\"#$<%:>&'()*+,-./0123456789;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`{|}~";

typedef struct offspring {
    char genes[sizeof target];
    size_t fitness;            /* number of characters matching the target */
} offspring;

static offspring population[OFFSPRING];
static char parents[2][sizeof target];
static size_t generation = 0;
static long mutation_rate = 1;   /* percent chance for a character to mutate */
static unsigned done = 0;

/* random integer in the inclusive range [low, high] */
static int random_between(int low, int high)
{
    return rand() % (high - low + 1) + low;
}

/* a random key press on the typewriter */
static char random_key()
{
    return keyboard[random_between(0, (int)strlen(keyboard) - 1)];
}

int main(int argc, char **argv)
{
    char *end;
    const char *p;
    int end_of_options = 0, quiet = 0, bad_rate = 0, have_target = 0;
    int monkey_flag = 0, monkey_mode, sorted;
    int easter_egg = OFFSPRING == 97;
    size_t i, j, len, fitness;
    int arg;

    /* parse the command line; stops at the first empty argument */
    for (arg = 1; arg < argc && *argv[arg]; ++arg) {
        char *opt = argv[arg];

        if (*opt == '-' && !end_of_options) {
            while (*++opt) {
                switch (*opt) {
                case '-':
                    end_of_options = 1;
                    break;
                case 'r':
                    mutation_rate = strtol(opt + 1, &end, 0);
                    if (mutation_rate < 1 || mutation_rate > 100) {
                        printf("mutation rate out of range\n");
                        bad_rate = 1;
                    } else
                        bad_rate = 0;
                    break;
                case 'q':
                    quiet = 1;
                    printf("quiet output\n");
                    break;
                case 'm':
                    monkey_flag = 1;
                    break;
                default:
                    break;
                }
            }
            continue;
        }

        /* any other argument becomes the target string */
        have_target = 1;
        len = strlen(opt);
        if (len > MAX_TARGET_LEN)
            len = MAX_TARGET_LEN;
        strncpy(target, opt, len);
        target[len] = '\0';
    }

    if (easter_egg && !have_target)
        strcpy(target, PRIMORIAL_97);

    /* every target character must be on the keyboard */
    for (p = target; *p && strchr(keyboard, toupper(*p)); ++p)
        ;
    if (*p) {
        printf("'%c' not in keyboard \"%s\"\n", *p, keyboard);
        return 1;
    }

    srand((unsigned)time(0));
    memset(population, 0, sizeof population);

    printf("target '%s'\n", target);
    printf("mutation rate %ld\n", mutation_rate);

    /* a bad mutation rate, -m or too few offspring: just random typing */
    if ((monkey_mode = (bad_rate || monkey_flag || OFFSPRING < 4)))
        printf("monkey at typewriter\n");

    len = strlen(target);
    for (j = 0; j < OFFSPRING; ++j)
        for (i = 0; i < len; ++i)
            population[j].genes[i] = random_key();

    while (!done) {
        if (++generation == MAX_ATTEMPTS) {
            printf("Too many attempts, blaming the monkey Eric even if "
                   "he isn't typing or doesn't exist. Bye.\n");
            return 1;
        }

        /* score each offspring against the target */
        for (j = 0; j < OFFSPRING && !done; ++j) {
            fitness = 0;
            for (i = 0; i < len; ++i)
                if (toupper(population[j].genes[i]) == toupper(target[i]))
                    ++fitness;
            population[j].fitness = fitness;
            done = fitness == len;
            if ((done || generation == 1) && (!quiet || done))
                printf("Generation %4zu\tOffspring %2zu: %s\n",
                       generation - 1, j + 1, population[j].genes);
        }
        if (done)
            break;

        /*
         * Bubble the fittest towards the front. The original "swap" assigns
         * the next element over the current one and puts it back, so the
         * fitter offspring is simply copied into the earlier slot.
         */
        if (!monkey_mode) {
            do {
                sorted = 1;
                for (i = 0; i < OFFSPRING - 1; ++i) {
                    if (population[i].fitness < population[i + 1].fitness) {
                        population[i] = population[i + 1];
                        sorted = 0;
                    }
                }
            } while (!sorted);
        }

        /* the two fittest become the parents of the next generation */
        strcpy(parents[0], population[0].genes);
        strcpy(parents[1], population[1].genes);

        for (j = 0; j < OFFSPRING; ++j) {
            /* crossover: each character comes from one of the parents */
            for (i = 0; !monkey_mode && i < len; ++i) {
                if (random_between(0, 9) < 5)
                    population[j].genes[i] = parents[0][i];
                else
                    population[j].genes[i] = parents[1][i];
            }
            if (!quiet)
                printf("Generation %4zu\tOffspring %2zu: %s -> ",
                       generation, j + 1, population[j].genes);

            /* mutation (or, for the monkey, retype every character) */
            for (i = 0; i < len; ++i)
                if (random_between(1, 100) <= mutation_rate || monkey_mode)
                    population[j].genes[i] = random_key();

            if (!quiet)
                printf("%s (%s)\n", population[j].genes,
                       monkey_mode ? "monkey typing" : "mutation");
        }
        if (!quiet)
            printf("**\n");
    }
    return 0;
}
