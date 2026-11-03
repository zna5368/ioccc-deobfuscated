#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>

/*
 * Layout of schneiderwent.data (one comma separated line):
 *
 *   85 x  slot -> phrase set index   (minutes 0..59, then hours at 61..84)
 *   32 x  phrase set bit masks       (which messages to print)
 *   42 x 11 message table            (pairs of rows: ASCII row, EBCDIC row)
 *
 * The messages are stored in EBCDIC.  Each odd row holds the EBCDIC text and
 * the even row before it is filled in with the ASCII translation at start up.
 * Once decoded the messages are:
 *
 *    0 "midnight "    2 "in the "    4 "wee hours "   6 "morning "
 *    8 "noon "       10 "afternoon " 12 "evening "    14 "at night "
 *   16 "exactly "    18 "just "      20 "after "      22 "past "
 *   24 "almost "     26 "a quarter " 28 "half "       30 "ten "
 *   32 "twenty "     34 "after "     36 "'til "       38 "past "
 *   40 "it's "
 */
#define NUM_SLOTS 85
#define NUM_PHRASE_SETS 32
#define NUM_MESSAGES 42
#define MESSAGE_LEN 11

#define HOUR_SLOT_BASE 61 /* hour h uses slot 61 + h */
#define MINUTE_PHRASE_BITS 22
#define HOUR_PHRASE_BITS 16
#define MSG_ITS 40

#define DELIMS ",\n\0"

int slot_to_phrase_set[NUM_SLOTS];
unsigned long phrase_set[NUM_PHRASE_SETS];
unsigned char messages[NUM_MESSAGES][MESSAGE_LEN];

/* 'J' is 74 (even) in ASCII and 209 (odd) in EBCDIC, so this selects the
   row of each message pair that is in the machine's own character set. */
int native_row = 'J' % 2;

/* Translate one EBCDIC character to ASCII.  Returns 0 if it is unknown. */
static int ebcdic_to_ascii(int c)
{
    if (c >= 129 && c <= 137) return c - 32; /* a..i */
    if (c >= 145 && c <= 153) return c - 39; /* j..r */
    if (c >= 162 && c <= 169) return c - 47; /* s..z */
    if (c == 125) return '\'';
    if (c == 64) return ' ';
    return 0;
}

/* Fill each even (ASCII) row from the odd (EBCDIC) row that follows it. */
static void decode_messages(void)
{
    int row, col, ascii;

    for (row = 1; row < NUM_MESSAGES; row += 2) {
        for (col = 0; messages[row][col]; col++) {
            ascii = ebcdic_to_ascii(messages[row][col]);
            if (ascii)
                messages[row - 1][col] = ascii;
        }
    }
}

static void print_message(int msg)
{
    int col = 0;

    msg += native_row;
    while (messages[msg][col])
        printf("%c", messages[msg][col++]);
}

/* Print the messages whose bits are set in the phrase set for this slot.
   Minute phrases use every other message (bit * 2), hour phrases use the
   bit number directly. */
static void print_phrases(int slot, int num_bits, int msg_step)
{
    unsigned long mask = phrase_set[slot_to_phrase_set[slot]];
    int bit;

    for (bit = 0; bit < num_bits; bit++)
        if (mask & (1UL << bit))
            print_message(bit * msg_step);
}

static void load_data(FILE *file)
{
    char line[2048];
    int i, j;

    fgets(line, 2048, file);
    slot_to_phrase_set[0] = atoi(strtok(line, ",\0"));
    for (i = 1; i < NUM_SLOTS; i++)
        slot_to_phrase_set[i] = atoi(strtok(NULL, DELIMS));
    for (i = 0; i < NUM_PHRASE_SETS; i++)
        phrase_set[i] = strtoul(strtok(NULL, DELIMS), NULL, 10);
    for (i = 0; i < NUM_MESSAGES; i++)
        for (j = 0; j < MESSAGE_LEN; j++)
            messages[i][j] = (unsigned char)atoi(strtok(NULL, DELIMS));
}

int main(int argc, char *argv[])
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char hhmm[7];
    int hour, minute;
    FILE *data = NULL;

    if (argc > 1)
        data = fopen(argv[1], "r");
    strftime(hhmm, 7, "%H:%M\n", local);

    if (data == NULL) {
        printf("%s", hhmm);
        exit(0);
    }

    load_data(data);
    fclose(data);

    hour = atoi(strtok(hhmm, ":\0"));
    minute = atoi(strtok(NULL, ":\0"));

    decode_messages();
    print_message(MSG_ITS);

    /* e.g. "just after ten 'til " */
    print_phrases(minute, MINUTE_PHRASE_BITS, 2);

    /* from 36 minutes on the time is described relative to the next hour */
    if (minute >= 36)
        hour++;
    if (hour == 24)
        hour = 0;
    if (hour != 0 && hour != 12)
        printf("%d ", hour < 12 ? hour : hour - 12);

    /* e.g. "in the evening " */
    print_phrases(HOUR_SLOT_BASE + hour, HOUR_PHRASE_BITS, 1);
    printf("\n");

    exit(0);
}
