#include<stdio.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>
#include<unistd.h>

/*
 * The data file holds every message of the game. Each message lives in a
 * fixed size block of the file so message number n starts at byte
 * n * BLOCK_SIZE and is ended by a NUL byte. Some of the messages are the
 * printf() and scanf() format strings used by the game.
 */
#ifndef DATA
#define DATA "data"
#endif
#define BLOCK_SIZE 1663

/* message numbers (blocks) in the data file */
enum {
    MSG_INTRO = 1,
    FMT_UNSIGNED = 2,           /* "%u" */
    FMT_LONG = 3,               /* "%ld" */
    MSG_GAME_OVER = 4,          /* tombstone printed after every death */
    MSG_SPAGHETTI_MONSTER = 5,
    MSG_BUY_FOOD = 6,           /* 6..10: one purchase prompt per item */
    FMT_STATUS = 11,
    FMT_MONEY_LEFT = 12,
    MSG_MILD_ILLNESS = 13,
    MSG_BAD_AMOUNT = 14,
    MSG_NO_OXEN = 15,
    MSG_LOW_ON_BULLETS = 16,
    MSG_SPAGHETTI_STRANGLES = 17,
    MSG_ATE_LAST_OX = 18,
    MSG_ATE_FUNNY_OX = 19,
    MSG_DAUGHTER_ARM = 20,
    MSG_WAGON_DAMAGED = 21,
    MSG_CLOCKS_STOP = 22,
    MSG_SOUTH_PASS = 23,
    MSG_STARVED = 24,
    MSG_OX_GHOST = 25,
    MSG_CASPER = 26,
    MSG_AMMO_EXPLODES = 27,
    MSG_BLACK_CAT = 28,
    MSG_ROBBED_RIDERS = 29,
    MSG_SPAGHETTI_TREE = 30,
    MSG_DEVIL_IN_MIRROR = 31,
    MSG_OX_KICKED_BUCKET = 32,
    MSG_TYRANNOSAURUSES = 33,
    MSG_RUNAWAY_BED = 34,
    MSG_BLIZZARD = 35,
    MSG_DIED_OF_BOREDOM = 36,
    MSG_LOW_FOOD = 37,
    MSG_CANNOT_AFFORD_DOCTOR = 38,
    MSG_SON_GHOST = 39,
    MSG_DIED_OF_DYSENTERY = 40,
    MSG_DIED_OF_INJURIES = 41,
    MSG_CHOOSE_ACTION = 42,
    MSG_OVERSPENT = 46,
    MSG_REACHED_OREGON = 47,
    MSG_PHANTOM_BLIZZARD = 48,
    MSG_ELF = 49,
    MSG_SHOT_EAGLE = 51,
    MSG_GREAT_SHOT = 52,
    MSG_TERRIBLE_SHOT = 53,
    MSG_SEVERE_ILLNESS = 54,
    MSG_RIDERS_AHEAD = 56,
    MSG_KNIFED_BY_RIDERS = 62,
    MSG_VAMPIRES = 68,
    MSG_WEREWOLF = 69,
    MSG_FLAT_OXEN = 71,
    MSG_OX_IN_HOLE = 72,
    MSG_BARGHEST = 74,
    MSG_SON_FREEZES = 75,
    MSG_BAD_WATER = 76,
    MSG_HEAVY_RAINS = 77,
    MSG_BLACKBEARD = 78,
    MSG_SHOT_BY_BLACKBEARD = 79,
    MSG_SCARED_PIRATES = 81,
    MSG_WAGON_FIRE = 82,
    MSG_FOG = 83,
    MSG_DRAGON = 84,
    MSG_SWAMPED_FORDING = 86,
    MSG_SON_GHOST_SICK = 91,
    MSG_DOCTOR_BILL = 93,
    MSG_HAIL_STORM = 94,
    MSG_HELPFUL_INDIANS = 95,
    MSG_RUGGED_MOUNTAINS = 96,
    MSG_LOST_CROSSROADS = 97,
    MSG_NO_MEDICAL_SUPPLIES = 99
};

/* the kinds of items that can be bought (index into supplies[]) */
enum { ITEM_FOOD = 1, ITEM_AMMO = 2, ITEM_CLOTHING = 3, ITEM_OXEN = 4,
       ITEM_MISC = 5, ITEM_COUNT = 6 };

#define START_MONEY 1666
#define MAX_PURCHASE 666
#define DOCTOR_BILL 666
#define BULLETS_PER_DOLLAR 5
#define TRAIL_LENGTH 2170
#define LAST_TURN 21

/* what to do after a random event */
enum { AFTER_EVENT_MOUNTAINS, AFTER_EVENT_DOCTOR, AFTER_EVENT_DELAY };

long random_range(long low, long high) { return rand() % (high - low + 1) + low; }

unsigned south_pass_done, treated_illness, oxen_warning_shown, hunting,
         injured, at_fort, cannot_afford_doctor, death_by_poison,
         treated_injury, choice, son_froze, ate_ox;

long oxen, food, ammo, clothing, misc, money, turn, fort_money_left;
long *supplies[] = { &oxen, &food, &ammo, &clothing, &oxen, &misc };
long ill, fort_purchase[7], event, miles, health = 666, blizzard,
     cleared_blue_mountains;

char *data;
FILE *data_file;
size_t data_size;

char *message(long number) { return &data[number * BLOCK_SIZE]; }

void print_message(long number) { printf("%s", message(number)); }

/* scanf() leaves bad input in the buffer, so skip the rest of the line */
#define read_input(format, var) \
    while (scanf(message(format), &var) != 1) while (getchar() != '\n');

/* every death prints its message, then the tombstone, then goes to hell */
void die(long number)
{
    print_message(number);
    print_message(MSG_GAME_OVER);
    exit(666);
}

/* half the time a shot hits, but only if there are bullets */
long shot_hits(void) { return ammo > 0 ? random_range(0, 99) > 49 : 0; }

/* initial purchase (at_fort == 0) or purchase at a fort (at_fort != 0) */
void buy_supplies(void)
{
    long item, i;
    int overspent;

    do {
        overspent = 0;
        memset(&fort_purchase, 0, sizeof fort_purchase);
        for (item = ITEM_FOOD; item < ITEM_COUNT && !overspent; ++item) {
            for (;;) {
                print_message(MSG_BUY_FOOD + item - ITEM_FOOD);
                if (at_fort) {
                    read_input(FMT_LONG, fort_purchase[item])
                    if (fort_purchase[item] < 0 || fort_purchase[item] > MAX_PURCHASE) {
                        print_message(MSG_BAD_AMOUNT);
                        continue;
                    }
                    fort_money_left = money;
                    for (i = 0; i < ITEM_COUNT; ++i)
                        fort_money_left -= fort_purchase[i];
                    if (fort_money_left < 0) {
                        print_message(MSG_OVERSPENT);
                        overspent = 1;
                    }
                } else {
                    read_input(FMT_LONG, *supplies[item])
                    if (*supplies[item] < 0 || *supplies[item] > MAX_PURCHASE) {
                        print_message(MSG_BAD_AMOUNT);
                        continue;
                    }
                    money = START_MONEY - oxen - food - ammo - clothing - misc;
                    if (money < 0) {
                        print_message(MSG_OVERSPENT);
                        oxen = food = ammo = clothing = misc = 0;
                        overspent = 1;
                    }
                }
                break;
            }
        }
    } while (overspent);
}

void visit_fort(void)
{
    long item;

    at_fort = 1;
    miles -= 45;
    buy_supplies();
    for (item = ITEM_FOOD; item < ITEM_COUNT; ++item) {
        *supplies[item] += item == ITEM_AMMO
            ? fort_purchase[item] * BULLETS_PER_DOLLAR : fort_purchase[item];
        money -= fort_purchase[item];
    }
    printf(message(FMT_MONEY_LEFT), money);
    at_fort = 0;
}

void hunt(void)
{
    hunting = 1;
    if (ammo < 9) {
        print_message(MSG_LOW_ON_BULLETS);
    } else {
        if (shot_hits()) {
            if (random_range(0, 9) > 4) {
                print_message(MSG_GREAT_SHOT);
                food += 23;
                random_range(0, 9);     /* result is ignored */
            } else {
                print_message(MSG_SHOT_EAGLE);
                food += 18;
            }
        } else {
            print_message(MSG_TERRIBLE_SHOT);
        }
        ammo -= 9;
        miles -= 45;
    }
    hunting = 0;
    at_fort = 0;
}

void eat(void)
{
    if (food < 1) {
        if (oxen > 39) {
            print_message(ate_ox = random_range(0, 1) ? MSG_ATE_LAST_OX : MSG_ATE_FUNNY_OX);
            oxen -= 40;
            food += 40;
        } else {
            die(MSG_STARVED);
        }
    } else {
        food -= food > 23 ? 23 : food > 18 ? 18 : food > 13 ? 13 : food;
    }
}

int random_event(void)
{
    switch (event = random_range(0, 22)) {
    case 0:
        print_message(MSG_DAUGHTER_ARM);
        break;
    case 1:
        print_message(MSG_HELPFUL_INDIANS);
        food += 5;
        break;
    case 2:
        oxen -= 40;
        print_message(MSG_FLAT_OXEN);
        break;
    case 3:
        print_message(ill = MSG_BAD_WATER);
        health -= 70;
        return AFTER_EVENT_DOCTOR;
    case 4:
        print_message(MSG_HEAVY_RAINS);
        misc -= 7;
        miles -= 7;
        break;
    case 5:
        print_message(MSG_BLACKBEARD);
        if (shot_hits()) {
            print_message(MSG_SCARED_PIRATES);
            food += 9;
            misc += 3;
        } else {
            print_message(injured = MSG_SHOT_BY_BLACKBEARD);
            health -= 80;
            money /= 4;
        }
        break;
    case 6:
        if (ammo > 1999)
            die(MSG_AMMO_EXPLODES);
        print_message(MSG_WAGON_FIRE);
        miles -= 9;
        misc -= 5;
        food -= 20;
        break;
    case 7:
        print_message(MSG_FOG);
        miles -= 10;
        break;
    case 8:
        print_message(MSG_SWAMPED_FORDING);
        food -= 30;
        clothing -= 20;
        oxen -= 40;
        if (random_range(1, 9) == 2)
            return AFTER_EVENT_DELAY;
        if (misc < 5)
            die(MSG_NO_MEDICAL_SUPPLIES);
        print_message(MSG_MILD_ILLNESS);
        misc -= 5;
        break;
    case 9:
    case 19:
        print_message(MSG_SPAGHETTI_TREE);
        break;
    case 10:
        print_message(MSG_RIDERS_AHEAD);
        if (shot_hits()) {
            print_message(ill = MSG_ROBBED_RIDERS);
            money += 70;
        } else {
            print_message(injured = MSG_KNIFED_BY_RIDERS);
            miles -= 50;
            health -= 35;
        }
        break;
    case 11:
        if (random_range(1, 9) == 2) {
            if (random_range(1, 2) == 1) {
                switch (random_range(2, 5)) {
                case 2:
                case 3:
                    print_message(ill = MSG_SEVERE_ILLNESS);
                    return AFTER_EVENT_DELAY;
                case 4:
                    death_by_poison = MSG_SPAGHETTI_STRANGLES;
                    break;
                }
            }
            print_message(MSG_SPAGHETTI_MONSTER);
        }
        return AFTER_EVENT_DELAY;
    case 12:
        print_message(MSG_HAIL_STORM);
        miles -= 5;
        health -= 19;
        misc -= 5;
        break;
    case 13:
        print_message(MSG_BLACK_CAT);
        break;
    case 14:
        print_message(MSG_BARGHEST);
        break;
    case 15:
        print_message(random_range(0, 1) ? MSG_WEREWOLF : MSG_VAMPIRES);
        break;
    case 16:
    case 22:
        print_message(MSG_ELF);
        break;
    case 17:
        print_message(MSG_OX_IN_HOLE);
        break;
    case 18:
        print_message(MSG_CASPER);
        break;
    case 20:
        print_message(MSG_OX_KICKED_BUCKET);
        break;
    case 21:
        print_message(MSG_DEVIL_IN_MIRROR);
        break;
    }
    return AFTER_EVENT_MOUNTAINS;
}

/* returns 0 if the turn is over (made it through South Pass) */
int cross_mountains(void)
{
    if (miles < 951)
        return 1;
    if (clothing < 36)
        die(MSG_PHANTOM_BLIZZARD);

    if (random_range(0, 9) <= 2) {
        print_message(MSG_RUGGED_MOUNTAINS);
        if (random_range(0, 1)) {
            if (random_range(0, 9) < 3) {
                print_message(MSG_CLOCKS_STOP);
                miles -= 85;
            } else {
                print_message(MSG_WAGON_DAMAGED);
                miles -= 99;
            }
        } else {
            print_message(MSG_LOST_CROSSROADS);
            miles -= 60;
        }
    }

    if (!south_pass_done) {
        south_pass_done = 1;
        if (random_range(0, 9) <= 5) {
            print_message(MSG_SOUTH_PASS);
            return 0;
        }
        print_message(MSG_BLIZZARD);
    } else {
        /* Blue Mountains */
        if (miles < 1951 || cleared_blue_mountains)
            return 1;
        cleared_blue_mountains = 1;
        if (!son_froze && !random_range(0, 4)) {
            print_message(MSG_SON_FREEZES);
            son_froze = 1;
            food += 50;
        }
        if (!random_range(0, 3))
            die(random_range(0, 4) ? MSG_DRAGON
                : random_range(0, 4) ? MSG_TYRANNOSAURUSES : MSG_RUNAWAY_BED);
        if (random_range(0, 9) < 5)
            print_message(MSG_BLIZZARD);
    }
    blizzard = 1;
    miles -= 101;
    ++ammo;
    miles -= 10;
    return 1;
}

void see_doctor_and_travel(void)
{
    if (random_range(0, 1)) {
        if (son_froze)
            print_message(MSG_SON_GHOST);
        if (ate_ox)
            print_message(MSG_OX_GHOST);
    }
    if (death_by_poison)
        die(death_by_poison);

    if (!cannot_afford_doctor && (injured || ill)) {
        if (son_froze && ill)
            print_message(MSG_SON_GHOST_SICK);
        if (money < DOCTOR_BILL) {
            print_message(cannot_afford_doctor = MSG_CANNOT_AFFORD_DOCTOR);
        } else {
            treated_illness = ill ? 1 : treated_illness;
            treated_injury = injured ? 1 : treated_injury;
            injured = ill = 0;
            print_message(MSG_DOCTOR_BILL);
            money -= DOCTOR_BILL;
        }
    }
    health -= injured ? 15 : treated_injury ? 5 : 0;
    health -= ill ? 5 : treated_illness ? 15 : 0;

    miles += 170;
    miles += oxen > 99 ? 30 : -99;
    health -= 10;
}

void play_turn(void)
{
    long item;

    if (oxen < 1) {
        if (!oxen_warning_shown) {
            print_message(oxen_warning_shown = MSG_NO_OXEN);
            oxen = 0;
        }
    } else {
        oxen_warning_shown = 0;
    }
    for (item = ITEM_FOOD; item < ITEM_COUNT; ++item)
        if (*supplies[item] < 0)
            *supplies[item] = 0;

    printf(message(FMT_STATUS), food, ammo, clothing, oxen, misc, money, health, miles);
    random_range(0, 1);         /* result is ignored */
    if (food < 14)
        print_message(MSG_LOW_FOOD);

    for (;;) {
        print_message(MSG_CHOOSE_ACTION);
        read_input(FMT_UNSIGNED, choice)
        if (choice == 1) {
            visit_fort();
            break;
        }
        if (choice == 2) {
            hunt();
            break;
        }
        if (choice == 3)
            break;
    }

    eat();

    switch (random_event()) {
    case AFTER_EVENT_MOUNTAINS:
        if (oxen > 39 && !random_range(0, 9)) {
            print_message(ate_ox = MSG_ATE_FUNNY_OX);
            oxen -= 40;
            food += 40;
        }
        if (!cross_mountains())
            return;
        break;
    case AFTER_EVENT_DELAY:
        miles -= 10;
        break;
    }
    see_doctor_and_travel();
}

int main() {
    srand(time(0));
    data_file = fopen(DATA, "r");
    if (!data_file || getdelim(&data, &data_size, EOF, data_file) < 0)
        return 666;

    print_message(MSG_INTRO);
    buy_supplies();
    ammo *= BULLETS_PER_DOLLAR;
    printf(message(FMT_MONEY_LEFT), money);

    do {
        if (health < 1 || miles < 0 || miles >= TRAIL_LENGTH)
            break;
        play_turn();
    } while (turn++ < LAST_TURN);

    if (health < 1) {
        if (ill)
            die(MSG_DIED_OF_DYSENTERY);
        if (injured)
            die(MSG_DIED_OF_INJURIES);
        die(MSG_DIED_OF_BOREDOM);
    }
    die(miles >= TRAIL_LENGTH ? MSG_REACHED_OREGON : MSG_DIED_OF_BOREDOM);
    return 666;
}
