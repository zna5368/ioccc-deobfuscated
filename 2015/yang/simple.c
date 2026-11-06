#include <stdio.h>
#include <math.h>

/*
 * The output text is poured into a canvas that is a ring of 64 rows.  Each row
 * has 79 cells of text followed by a newline.  A spare 65th row exists because
 * count_free_cells() may look one row past the last one.
 */
#define CANVAS_ROWS 64
#define CANVAS_COLS 80
#define TEXT_COLS 79
#define CELL(row, col) canvas[(row) * CANVAS_COLS + (col)]

/* A cell holds either one of these markers, or CELL_TEXT to accept output. */
#define CELL_TEXT 0
#define CELL_BLANK ' '
#define CELL_NEWLINE '\n'

/* The first output row is indented so that #include ends exactly at its end. */
#define INCLUDE_COLUMN 62
#define INITIAL_WRITE_ROW 37

/* Rows relative to the write row (modulo CANVAS_ROWS) used by the patterns. */
#define PATTERN_ROW_OFFSET 44
#define DANGO_EYE_ROW_OFFSET 42

/* Pattern spacing: n is counted in half rows because cells are twice as tall. */
#define MIN_PATTERN_GAP 30
#define MIN_PATTERN_DISTANCE_SQUARED 1600

/* The longest word of the generated code that may not be split is "return". */
#define LONGEST_WORD 6

/* Size of the pointer table in the generated program. */
#define TABLE_SIZE 512

#define RANDOM_MULTIPLIER 1103515245u
#define RANDOM_MASK ((1 << 30) - 1)

/*
 * Language dialect tests.  Each one relies on the same characters meaning
 * something different to a different version of C or C++.
 *
 * A character literal is an int in C but a char in C++.
 */
#define IS_CPLUSPLUS (sizeof(' ') == 1)

/*
 * C11 and C++11 read U"1" as one 32 bit string.  Older dialects see the macro
 * parameter U, which is empty, followed by the ordinary string "1".
 */
#define STRING_ELEMENT_SIZE(U) sizeof(U"1"[0])

/* Plain C11 is the only dialect that draws no patterns. */
#define PATTERNS_ENABLED (IS_CPLUSPLUS || STRING_ELEMENT_SIZE() == 1)

/*
 * C++11 reads R"()" as an empty raw string.  Older dialects see the empty macro
 * parameter R followed by the ordinary string "()", which starts with '('.
 */
#define PARENTHESES_STRING(R) R"()"
#define HAS_RAW_STRINGS (!PARENTHESES_STRING()[0])

/*
 * C99 ignores everything after the two slashes, which leaves 2 - 1 = 1.  C90
 * has no such comment, so it sees a division and a block comment: 2 / 2 - 1 = 0.
 */
static int has_line_comments(void)
{
    return 2 //* */ 2
        - 1;
}

/* The start of the generated program, which builds the pointer table. */
static const char program_header[] =
    "#include<stdio.h>"
    "typedef int O;"
    "void o(O _){putchar(_);}"
    "O main(){"
    "O*_[512],**p=_,**d,b,q;"
    "for(b=0;b++<512;p=_+q)_[q=(p-_+1)*9%512]=(O*)p;";

static const char program_footer[] = "return+0;}\n";

static char canvas[(CANVAS_ROWS + 1) * CANVAS_COLS];
static int read_row = 0;
static int read_col = CANVAS_COLS;      /* row 0 counts as already printed */
static int write_row = INITIAL_WRITE_ROW;
static int rows_since_pattern = 26;     /* in half rows */
static int last_pattern_x = 40;
static int random_state;

/*
 * Returns a pseudo random number from 0 up to limit.  The original multiplies
 * signed ints, and that overflow lets gcc -O2 and higher change the numbers
 * produced inside a C pattern.  Unsigned math gives the -O0 (or -fwrapv) result.
 */
static float random_float(float limit)
{
    random_state = (int)(((unsigned)random_state + 1u) * RANDOM_MULTIPLIER
                         & RANDOM_MASK);
    return random_state * limit / RANDOM_MASK;
}

static void init_canvas(void)
{
    int row, col;

    for (row = 0; row <= CANVAS_ROWS; row++)
        CELL(row, TEXT_COLS) = CELL_NEWLINE;
    for (col = 0; col < INCLUDE_COLUMN; col++)
        CELL(1, col) = CELL_BLANK;
}

/* Blanks every cell along one line going outward from the pattern's center. */
static void carve_spoke(float center_x, float angle, float length, float squash)
{
    float distance, y_offset;
    int col, row;

    for (distance = 0; distance < length; distance += .1) {
        y_offset = distance * sin(angle);
        col = center_x + cos(angle) * distance;
        if (col >= 0 && col < TEXT_COLS) {
            row = (int)(write_row + PATTERN_ROW_OFFSET
                        + y_offset * (.5 - (y_offset > 0 ? 3 * squash : squash)))
                  % CANVAS_ROWS;
            CELL(row, col) = CELL_BLANK;
        }
    }
}

/* Leaves two gaps in a pattern by restoring text cells on the given rows. */
static void draw_eyes(int center_col, int top_row, int bottom_row)
{
    if (center_col > 5)
        CELL(top_row, center_col - 3) = CELL(bottom_row, center_col - 3) = CELL_TEXT;
    if (center_col < 75)
        CELL(top_row, center_col + 2) = CELL(bottom_row, center_col + 2) = CELL_TEXT;
}

/*
 * C draws a five armed sea star, with eyes if it is at least C99.  C++ draws a
 * circle, which is an uneven dango with eyes if there are no raw strings.
 */
static void draw_pattern(float center_x)
{
    float base_radius, arm_length, phase, squash, sweep, radius;
    int top_row, bottom_row;
    int has_eyes = 0;

    squash = 0;
    if (IS_CPLUSPLUS) {
        base_radius = 17 + random_float(5);
        arm_length = 0;
        phase = 0;
        if (!HAS_RAW_STRINGS)
            squash = .1;
    } else {
        base_radius = 1 + random_float(2);
        arm_length = 12 - base_radius + random_float(8);
        phase = random_float(4);
        has_eyes = has_line_comments();
    }

    /* The radius waves five times during one turn: sweep / 5 is the angle. */
    sweep = radius = -.05;
    while (sweep < 32) {
        carve_spoke(center_x, sweep / 5, radius, squash);
        sweep += .05;
        radius = base_radius + arm_length
                 + arm_length * .5 * sin(sweep + phase);
    }

    if (has_eyes) {
        top_row = (write_row + PATTERN_ROW_OFFSET) % CANVAS_ROWS;
        draw_eyes(center_x, top_row, top_row);
    } else if (squash != 0) {
        top_row = (write_row + DANGO_EYE_ROW_OFFSET) % CANVAS_ROWS;
        bottom_row = (top_row + 1) % CANVAS_ROWS;
        draw_eyes(center_x + (random_float(base_radius) - base_radius / 2),
                  top_row, bottom_row);
    }
}

/* Moves to the next row, preparing a fresh row ahead of the output. */
static void advance_row(void)
{
    float center_x, x_distance;
    int col;

    for (col = 0; col < TEXT_COLS; col++)
        CELL(write_row, col) = CELL_TEXT;
    CELL(write_row, TEXT_COLS) = CELL_NEWLINE;
    write_row = (write_row + 1) % CANVAS_ROWS;

    rows_since_pattern += 2;
    center_x = random_float(70) + 5;
    x_distance = center_x - last_pattern_x;
    if (rows_since_pattern > MIN_PATTERN_GAP
        && x_distance * x_distance + rows_since_pattern * rows_since_pattern
           > MIN_PATTERN_DISTANCE_SQUARED
        && PATTERNS_ENABLED) {
        draw_pattern(center_x);
        rows_since_pattern = 0;
        last_pattern_x = center_x;
    }

    read_row = (read_row + 1) % CANVAS_ROWS;
    read_col = 0;
}

/* Prints blanks and newlines from the canvas until a cell accepts the text. */
static void emit_char(int text)
{
    int cell;

    do {
        if (read_col && CELL(read_row, read_col - 1) == CELL_NEWLINE)
            advance_row();
        cell = CELL(read_row, read_col++);
        putchar(cell ? cell : text);
    } while (cell);
}

static void emit_string(const char *text)
{
    while (*text)
        emit_char(*text++);
}

static void emit_stars(int count)
{
    while (count-- > 0)
        emit_char('*');
}

/* Counts the text cells ahead, up to the length of the longest word. */
static int count_free_cells(void)
{
    int count = 0;

    while (count < LONGEST_WORD && !CELL(read_row, read_col + count))
        count++;
    return count;
}

/* The table order in the generated program: slot + 1, times 9, modulo 512. */
static int next_slot(int slot)
{
    return (slot + 1) * 9 % TABLE_SIZE;
}

int main(int argc, char **argv)
{
    int current_slot = 0, target_slot, dereferences, input, swap;
    int old_name = 'p', new_name = 'd';

    /* The addresses of the arguments vary per run when ASLR is enabled. */
    while (argc--)
        random_state += argv[argc] - (char *)0;

    init_canvas();
    emit_string(program_header);

    /*
     * Each table entry points at the previous slot of the cycle.  A byte is
     * encoded as the distance between the current slot and a slot found by
     * following pointers.  With 512 slots, a slot exactly 0 to 255 away always
     * exists in at least one direction.
     */
    while ((input = getchar()) != EOF) {
        target_slot = current_slot;
        for (dereferences = TABLE_SIZE;
             current_slot - target_slot != input
             && target_slot - current_slot != input;
             dereferences--)
            target_slot = next_slot(target_slot);

        /* Prints o(p-(d=***(O*****)p)); or o((d=***(O*****)p)-p); */
        emit_string("o(");
        if (current_slot > target_slot) {
            emit_char(old_name);
            emit_char('-');
        }
        emit_char('(');
        emit_char(new_name);
        emit_char('=');
        emit_stars(dereferences);
        emit_string("(O**");
        emit_stars(dereferences);
        emit_char(')');
        emit_char(old_name);
        emit_char(')');
        if (current_slot <= target_slot) {
            emit_char('-');
            emit_char(old_name);
        }
        emit_string(");");

        swap = old_name;
        old_name = new_name;
        new_name = swap;
        current_slot = target_slot;
    }

    /* Pad with empty blocks until "return" fits without being broken up. */
    while (count_free_cells() < LONGEST_WORD)
        emit_string("{;}");
    emit_string(program_footer);
    return 0;
}
