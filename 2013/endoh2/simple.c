#include <stdio.h>

/*
 * Stage one of a two stage program. The obfuscated source keeps the second
 * program in one big comment, so this stage has to be given that source:
 *
 *     ./simple < endoh2.c > jpeg.c
 *
 * The comment holds a payload of two parts:
 *
 *     compressed layout   823 bytes that expand to a bitmap of filled and
 *                         empty cells, 108 cells per row
 *     template text       the characters of jpeg.c, to be poured into the
 *                         filled cells one at a time
 */

/* Where the payload sits in the source file. */
#define SOURCE_BYTES     3583   /* bytes read from standard input */
#define PAYLOAD_START    16     /* skips the indentation and the opening comment mark */
#define PAYLOAD_END      3581   /* stops before the closing comment mark */

/* Stand-ins for characters that cannot appear in the payload. */
#define SLASH_STAND_IN   '_'    /* a real slash could form a closing comment mark */
#define SPACE_STAND_IN   '@'    /* whitespace is removed from the payload */

#define BUFFER_BYTES     13997
#define COMPRESSED_BYTES 823

/* Each compressed byte is an opcode measured from this character. */
#define OPCODE_BASE      'X'

/* Opcodes below zero: the low five bits of the byte are the cells themselves. */
#define LITERAL_CELLS    5

/* Opcodes 0 to 5: a run of filled cells. */
#define LONG_RUN_OPCODE  5
#define SHORT_RUN_CELLS  30
#define LONG_RUN_CELLS   160

/* Opcodes 6 to 35: copy earlier cells, length and distance packed in base 9. */
#define LAST_NEAR_OPCODE 35
#define NEAR_BASE        9
#define NEAR_LENGTH_UNIT 6
#define NEAR_OFFSET_UNIT 12

/* Opcodes above 35: copy from the row above, in a length that doubles. */
#define ROW_COPY_CELLS   27

/* Shape of the program being printed. */
#define ROW_CELLS        108
#define LINE_BYTES       (ROW_CELLS + 1)    /* the cells and a newline */
#define OUTPUT_LINES     55
#define OUTPUT_BYTES     (OUTPUT_LINES * LINE_BYTES)

/* Template character that stands for the whole payload as a string literal. */
#define STRING_MARKER    '$'

/* Holds the payload, and after it the expanded layout. */
char buffer[BUFFER_BYTES];

/* Copies the visible characters of the payload, returning the end of them. */
char *read_payload(void)
{
    char *end = buffer;
    int offset;

    for (offset = 0; offset < SOURCE_BYTES; offset++) {
        int ch = getchar();

        if (offset >= PAYLOAD_START && offset < PAYLOAD_END && ch > ' ') {
            *end++ = ch == SLASH_STAND_IN ? '/' : ch;
        }
    }
    return end;
}

/* Expands the compressed layout, writing one 0 or 1 per cell. */
void expand_layout(char *cell)
{
    const char *code;

    for (code = buffer; code < buffer + COMPRESSED_BYTES; code++) {
        int opcode = *code - OPCODE_BASE;
        int count;
        int distance;
        int i;

        if (opcode < 0) {
            for (i = 0; i < LITERAL_CELLS; i++) {
                *cell++ = *code >> i & 1;
            }
        } else if (opcode <= LONG_RUN_OPCODE) {
            count = opcode == LONG_RUN_OPCODE ? LONG_RUN_CELLS : SHORT_RUN_CELLS;
            for (i = 0; i < count; i++) {
                *cell++ = 1;
            }
        } else {
            if (opcode > LAST_NEAR_OPCODE) {
                count = ROW_COPY_CELLS << (opcode - LAST_NEAR_OPCODE - 1);
                distance = ROW_CELLS;
            } else {
                count = opcode / NEAR_BASE * NEAR_LENGTH_UNIT;
                distance = (opcode % NEAR_BASE + 1) * NEAR_OFFSET_UNIT;
            }
            /* One at a time, as a copy may overlap the cells it is writing. */
            for (i = 0; i < count; i++, cell++) {
                *cell = cell[-distance] != 0;
            }
        }
    }
}

/* Pours the template into the filled cells of the layout. */
void print_program(const char *layout)
{
    const char *cell = layout + ROW_CELLS;              /* the first row is blank */
    const char *template_text = buffer + COMPRESSED_BYTES;
    const char *string_text = NULL;                     /* NULL until the marker is reached */
    int position;

    for (position = 1; position <= OUTPUT_BYTES; position++) {
        int column = position % LINE_BYTES;

        if (column == 0) {
            putchar('\n');
        } else if (!*cell++) {
            putchar(' ');
        } else if (*template_text != STRING_MARKER) {
            char ch = *template_text++;

            putchar(ch == SPACE_STAND_IN ? ' ' : ch);
        } else if (string_text != NULL && *string_text != '\0') {
            /* Every row of the string is its own quoted piece. */
            if (column == 1 || column == ROW_CELLS) {
                putchar('"');
            } else {
                putchar(*string_text++);
            }
        } else {
            /* The quote that opens the string, or else the one that closes it. */
            if (string_text == NULL) {
                string_text = buffer;
            } else {
                string_text = template_text++;
            }
            putchar('"');
        }
    }
}

int main()
{
    char *layout = read_payload();

    expand_layout(layout);
    print_program(layout);
    return 0;
}
