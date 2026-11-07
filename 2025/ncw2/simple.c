/* C64 EMULATOR */

#include <unistd.h>

/*
 * Everything the original FRACTRAN program prints before the maze starts:
 * terminal colours, a cleared screen and the Commodore 64 boot screen.
 */
static const char boot_screen[] =
    "\033[48;2;66;49;142m"      /* background colour: C64 dark blue  */
    "\033[38;2;149;134;216m"    /* text colour: C64 light blue        */
    "\033[2J"                   /* clear the screen                   */
    "\033[H"                    /* move the cursor to the top left    */
    "\n"
    "    **** COMMODORE 64 BASIC V2 ****\n"
    "\n"
    "READY.\n"
    "10 PRINT CHR$(205.5+RND(1)); : GOTO 10\n"
    "RUN\n";

/* The first 300 binary digits of pi (11.00100100001111...), in hex. */
static const char pi_hex_digits[] =
    "c90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3";

#define PI_BIT_COUNT 300

/* UTF-8 box drawing characters used for the maze. */
static const char forward_slash[]  = "╱";   /* a 1 bit of pi */
static const char backward_slash[] = "╲";   /* a 0 bit of pi */

/* Retro speed: every byte is followed by a 10 ms pause. */
static void put_byte(char byte)
{
    write(1, &byte, 1);
    usleep(10000);
}

static void put_string(const char *text)
{
    while (*text)
        put_byte(*text++);
}

static int hex_value(char digit)
{
    return digit <= '9' ? digit - '0' : digit - 'a' + 10;
}

/* Returns binary digit number index (0 = most significant) of pi. */
static int pi_bit(int index)
{
    int nibble = hex_value(pi_hex_digits[index / 4]);
    return (nibble >> (3 - index % 4)) & 1;
}

int main(void)
{
    put_string(boot_screen);

    /* 10 PRINT CHR$(205.5+RND(1)); : GOTO 10  -- forever */
    for (int bit = 0; ; bit = (bit + 1) % PI_BIT_COUNT)
        put_string(pi_bit(bit) ? forward_slash : backward_slash);
}
