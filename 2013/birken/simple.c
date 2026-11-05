#include <stdio.h>

#define FIELD_HEIGHT 42
#define FIELD_WIDTH 21
#define LAST_COLUMN (FIELD_WIDTH - 1)

#define LINE_SIZE 42
#define SPRITE_WIDTH 17
#define SPRITE_LEFT_MARGIN 2
#define SPRITE_COLUMNS 19 /* margin + sprite width */

#define COLOR_COUNT 8
#define EMPTY (-1)

#define SQUARES_PER_PIECE 4
#define EMITTER_COUNT 10
#define LAST_EMITTER_LEFT 18 /* a piece 3 squares wide against the right wall */
#define MAX_EMITTER_DROPS 36
#define PLATFORM_COUNT 5
#define PLATFORM_DROPS 21 /* the last drop removes the platform */

/* Screen layout, in terminal rows and columns */
#define BLACK 0
#define WHITE 7
#define FIELD_TOP_ROW 2
#define FIELD_LEFT_COLUMN 4
#define LEFT_WALL_COLUMN 2
#define RIGHT_WALL_COLUMN 46
#define FLOOR_ROW 44
#define BORDER_LENGTH 45

/* ANSI escape sequences */
#define MOVE_AND_SET_BACKGROUND "\033[%d;%df\033[4%dm"
#define CLEAR_SCREEN "\033[2J"
#define HIDE_CURSOR "\033[?25l"
#define MOVE_BELOW_FIELD "\033[47;1f"
#define SHOW_CURSOR "\033[?25h"
#define BLACK_BACKGROUND "\033[40m"

/*
 * Every orientation of every tetrimino. The suffix says which way the odd
 * square points. The first three must stay in this order: they are the pieces
 * that leave one square behind, and each one's value is both the color slot it
 * is drawn with and the distance from its left edge to the protruding square.
 */
enum {
    L_DOWN, T_DOWN, J_DOWN,
    L_LEFT, L_UP, L_RIGHT,
    T_LEFT, T_UP, T_RIGHT,
    J_LEFT, J_UP, J_RIGHT,
    S_FLAT, S_TALL,
    Z_FLAT, Z_TALL,
    I_FLAT, I_TALL,
    O_BLOCK,
    ROTATION_COUNT
};

/* Index into the palette. The sprite's colors are moved into the first slots */
enum { SLOT_L, SLOT_T, SLOT_J, SLOT_S, SLOT_Z, SLOT_I, SLOT_O };

typedef struct {
    int x;
    int y;
} Square;

typedef struct {
    Square squares[SQUARES_PER_PIECE];
    int slot;
    int mirrored_slot; /* a mirrored L is a J and a mirrored S is a Z */
} Rotation;

typedef struct {
    int rotation;
    int column;
} Drop;

typedef struct {
    int length;
    Drop drops[MAX_EMITTER_DROPS];
} Emitter;

static const Rotation rotations[ROTATION_COUNT] = {
    /* ### / #.. */
    [L_DOWN] = {{{2, 0}, {0, 1}, {1, 0}, {0, 0}}, SLOT_L, SLOT_J},
    /* ### / .#. */
    [T_DOWN] = {{{1, 1}, {2, 0}, {1, 0}, {0, 0}}, SLOT_T, SLOT_T},
    /* ### / ..# */
    [J_DOWN] = {{{2, 1}, {2, 0}, {1, 0}, {0, 0}}, SLOT_J, SLOT_L},
    /* ## / .# / .# */
    [L_LEFT] = {{{1, 2}, {1, 1}, {1, 0}, {0, 0}}, SLOT_L, SLOT_J},
    /* ..# / ### */
    [L_UP] = {{{2, 1}, {1, 1}, {2, 0}, {0, 1}}, SLOT_L, SLOT_J},
    /* #. / #. / ## */
    [L_RIGHT] = {{{1, 2}, {0, 2}, {0, 1}, {0, 0}}, SLOT_L, SLOT_J},
    /* .# / ## / .# */
    [T_LEFT] = {{{1, 2}, {1, 1}, {0, 1}, {1, 0}}, SLOT_T, SLOT_T},
    /* .#. / ### */
    [T_UP] = {{{2, 1}, {1, 1}, {0, 1}, {1, 0}}, SLOT_T, SLOT_T},
    /* #. / ## / #. */
    [T_RIGHT] = {{{0, 2}, {1, 1}, {0, 1}, {0, 0}}, SLOT_T, SLOT_T},
    /* .# / .# / ## */
    [J_LEFT] = {{{1, 2}, {0, 2}, {1, 1}, {1, 0}}, SLOT_J, SLOT_L},
    /* #.. / ### */
    [J_UP] = {{{2, 1}, {1, 1}, {0, 1}, {0, 0}}, SLOT_J, SLOT_L},
    /* ## / #. / #. */
    [J_RIGHT] = {{{0, 2}, {0, 1}, {1, 0}, {0, 0}}, SLOT_J, SLOT_L},
    /* .## / ##. */
    [S_FLAT] = {{{1, 1}, {2, 0}, {0, 1}, {1, 0}}, SLOT_S, SLOT_Z},
    /* #. / ## / .# */
    [S_TALL] = {{{1, 2}, {1, 1}, {0, 1}, {0, 0}}, SLOT_S, SLOT_Z},
    /* ##. / .## */
    [Z_FLAT] = {{{2, 1}, {1, 1}, {1, 0}, {0, 0}}, SLOT_Z, SLOT_S},
    /* .# / ## / #. */
    [Z_TALL] = {{{0, 2}, {1, 1}, {0, 1}, {1, 0}}, SLOT_Z, SLOT_S},
    /* #### */
    [I_FLAT] = {{{3, 0}, {2, 0}, {1, 0}, {0, 0}}, SLOT_I, SLOT_I},
    /* # / # / # / # */
    [I_TALL] = {{{0, 3}, {0, 2}, {0, 1}, {0, 0}}, SLOT_I, SLOT_I},
    /* ## / ## */
    [O_BLOCK] = {{{1, 1}, {0, 1}, {1, 0}, {0, 0}}, SLOT_O, SLOT_O},
};

/*
 * After a protruding piece has landed, these drops build the rectangle above
 * it. The rectangle is 3 rows tall (7 for emitters 3 and 7) and spans the whole
 * field, so its rows clear and only the protruding square stays. Pieces further
 * right than column 9 use the same lists, mirrored.
 */
static const Emitter emitters[EMITTER_COUNT] = {
    /* protruding piece starts in column 0 */
    {15, {
        {Z_FLAT, 2}, {Z_FLAT, 4}, {I_FLAT, 2}, {Z_FLAT, 6},
        {Z_FLAT, 8}, {I_FLAT, 6}, {Z_FLAT, 10}, {Z_FLAT, 12},
        {I_FLAT, 10}, {Z_FLAT, 14}, {Z_FLAT, 16}, {I_FLAT, 14},
        {S_TALL, 18}, {O_BLOCK, 0}, {L_LEFT, 19},
    }},
    /* protruding piece starts in column 1 */
    {15, {
        {T_RIGHT, 0}, {Z_FLAT, 3}, {Z_FLAT, 5}, {Z_FLAT, 7},
        {I_FLAT, 4}, {Z_FLAT, 9}, {Z_FLAT, 11}, {I_FLAT, 8},
        {Z_FLAT, 13}, {Z_FLAT, 15}, {I_FLAT, 12}, {Z_FLAT, 17},
        {T_LEFT, 19}, {I_FLAT, 16}, {T_DOWN, 1},
    }},
    /* protruding piece starts in column 2 */
    {15, {
        {L_DOWN, 1}, {J_RIGHT, 0}, {Z_FLAT, 4}, {I_FLAT, 2},
        {Z_FLAT, 6}, {Z_FLAT, 8}, {I_FLAT, 6}, {Z_FLAT, 10},
        {Z_FLAT, 12}, {I_FLAT, 10}, {Z_FLAT, 14}, {Z_FLAT, 16},
        {I_FLAT, 14}, {S_TALL, 18}, {L_LEFT, 19},
    }},
    /* protruding piece starts in column 3 */
    {36, {
        {S_FLAT, 1}, {L_RIGHT, 2}, {Z_FLAT, 5}, {Z_FLAT, 7},
        {Z_FLAT, 9}, {Z_FLAT, 11}, {Z_FLAT, 13}, {L_UP, 5},
        {Z_FLAT, 15}, {J_LEFT, 13}, {Z_FLAT, 17}, {T_RIGHT, 6},
        {S_TALL, 19}, {L_UP, 16}, {I_TALL, 4}, {I_FLAT, 9},
        {L_LEFT, 4}, {L_DOWN, 8}, {J_UP, 11}, {T_RIGHT, 17},
        {Z_FLAT, 7}, {O_BLOCK, 12}, {T_RIGHT, 0}, {I_FLAT, 5},
        {Z_TALL, 15}, {S_TALL, 9}, {T_DOWN, 10}, {T_DOWN, 14},
        {S_TALL, 19}, {Z_FLAT, 17}, {S_TALL, 0}, {I_FLAT, 13},
        {S_TALL, 0}, {L_LEFT, 2}, {I_FLAT, 1}, {L_LEFT, 19},
    }},
    /* protruding piece starts in column 4 */
    {15, {
        {S_FLAT, 2}, {J_DOWN, 3}, {S_FLAT, 0}, {L_DOWN, 0},
        {Z_FLAT, 6}, {Z_FLAT, 8}, {I_FLAT, 6}, {Z_FLAT, 10},
        {Z_FLAT, 12}, {I_FLAT, 10}, {Z_FLAT, 14}, {Z_FLAT, 16},
        {I_FLAT, 14}, {S_TALL, 18}, {L_LEFT, 19},
    }},
    /* protruding piece starts in column 5 */
    {15, {
        {S_FLAT, 3}, {S_FLAT, 1}, {T_RIGHT, 0}, {I_FLAT, 1},
        {Z_FLAT, 7}, {Z_FLAT, 9}, {Z_FLAT, 11}, {I_FLAT, 8},
        {Z_FLAT, 13}, {Z_FLAT, 15}, {I_FLAT, 12}, {Z_FLAT, 17},
        {T_LEFT, 19}, {I_FLAT, 16}, {T_DOWN, 5},
    }},
    /* protruding piece starts in column 6 */
    {15, {
        {S_FLAT, 4}, {S_FLAT, 2}, {I_FLAT, 3}, {S_FLAT, 0},
        {L_DOWN, 0}, {Z_FLAT, 8}, {L_DOWN, 7}, {Z_FLAT, 10},
        {Z_FLAT, 12}, {I_FLAT, 10}, {Z_FLAT, 14}, {Z_FLAT, 16},
        {I_FLAT, 14}, {S_TALL, 18}, {L_LEFT, 19},
    }},
    /* protruding piece starts in column 7 */
    {36, {
        {S_FLAT, 5}, {Z_FLAT, 9}, {Z_FLAT, 11}, {Z_FLAT, 13},
        {Z_FLAT, 15}, {Z_FLAT, 17}, {I_TALL, 11}, {L_RIGHT, 17},
        {O_BLOCK, 13}, {S_FLAT, 3}, {L_RIGHT, 9}, {S_FLAT, 1},
        {T_LEFT, 19}, {T_LEFT, 7}, {J_RIGHT, 12}, {Z_TALL, 6},
        {L_LEFT, 9}, {I_FLAT, 2}, {T_DOWN, 18}, {J_LEFT, 19},
        {L_RIGHT, 2}, {T_RIGHT, 5}, {I_TALL, 15}, {S_TALL, 7},
        {T_DOWN, 17}, {J_RIGHT, 14}, {T_LEFT, 3}, {O_BLOCK, 12},
        {J_DOWN, 4}, {I_FLAT, 8}, {I_TALL, 16}, {Z_TALL, 0},
        {I_FLAT, 16}, {Z_TALL, 0}, {T_RIGHT, 0}, {J_DOWN, 1},
    }},
    /* protruding piece starts in column 8 */
    {15, {
        {S_FLAT, 6}, {J_DOWN, 7}, {S_FLAT, 4}, {S_FLAT, 2},
        {I_FLAT, 3}, {S_FLAT, 0}, {L_DOWN, 0}, {Z_FLAT, 10},
        {Z_FLAT, 12}, {I_FLAT, 10}, {Z_FLAT, 14}, {Z_FLAT, 16},
        {I_FLAT, 14}, {S_TALL, 18}, {L_LEFT, 19},
    }},
    /* protruding piece starts in column 9 */
    {15, {
        {S_FLAT, 7}, {S_FLAT, 5}, {S_FLAT, 3}, {I_FLAT, 5},
        {S_FLAT, 1}, {T_RIGHT, 0}, {I_FLAT, 1}, {Z_FLAT, 11},
        {Z_FLAT, 13}, {Z_FLAT, 15}, {I_FLAT, 12}, {Z_FLAT, 17},
        {T_LEFT, 19}, {I_FLAT, 16}, {T_DOWN, 9},
    }},
};

/*
 * A temporary flat floor built on one square of the row below, for when that
 * row cannot support the next row. Mirrored for squares further right.
 */
static const Drop platforms[PLATFORM_COUNT][PLATFORM_DROPS] = {
    /* first T_UP lands on supporting column pair 0 */
    {
        {T_UP, 0}, {Z_FLAT, 2}, {Z_FLAT, 4}, {Z_FLAT, 6},
        {Z_FLAT, 8}, {Z_FLAT, 10}, {Z_FLAT, 12}, {Z_FLAT, 14},
        {Z_FLAT, 16}, {Z_FLAT, 18}, {I_FLAT, 1}, {I_FLAT, 5},
        {I_FLAT, 2}, {I_FLAT, 9}, {I_FLAT, 6}, {I_FLAT, 13},
        {I_FLAT, 10}, {T_UP, 17}, {I_FLAT, 14}, {J_RIGHT, 0},
        {L_LEFT, 19},
    },
    /* first T_UP lands on supporting column pair 1 */
    {
        {T_UP, 2}, {Z_FLAT, 4}, {Z_FLAT, 6}, {Z_FLAT, 8},
        {Z_FLAT, 10}, {Z_FLAT, 12}, {Z_FLAT, 14}, {Z_FLAT, 16},
        {Z_FLAT, 18}, {S_FLAT, 0}, {I_FLAT, 1}, {I_FLAT, 5},
        {I_FLAT, 2}, {I_FLAT, 9}, {I_FLAT, 6}, {I_FLAT, 13},
        {I_FLAT, 10}, {T_UP, 17}, {I_FLAT, 14}, {J_RIGHT, 0},
        {L_LEFT, 19},
    },
    /* first T_UP lands on supporting column pair 2 */
    {
        {T_UP, 4}, {Z_FLAT, 6}, {Z_FLAT, 8}, {Z_FLAT, 10},
        {Z_FLAT, 12}, {Z_FLAT, 14}, {Z_FLAT, 16}, {Z_FLAT, 18},
        {I_FLAT, 5}, {I_FLAT, 9}, {I_FLAT, 6}, {I_FLAT, 13},
        {I_FLAT, 10}, {T_UP, 17}, {I_FLAT, 14}, {S_FLAT, 2},
        {S_FLAT, 3}, {S_FLAT, 0}, {T_RIGHT, 0}, {T_DOWN, 1},
        {L_LEFT, 19},
    },
    /* first T_UP lands on supporting column pair 3 */
    {
        {T_UP, 6}, {Z_FLAT, 8}, {Z_FLAT, 10}, {Z_FLAT, 12},
        {Z_FLAT, 14}, {Z_FLAT, 16}, {Z_FLAT, 18}, {I_FLAT, 7},
        {I_FLAT, 11}, {I_FLAT, 8}, {T_UP, 15}, {I_FLAT, 12},
        {S_FLAT, 4}, {S_FLAT, 5}, {S_FLAT, 2}, {S_FLAT, 3},
        {S_FLAT, 0}, {T_RIGHT, 0}, {T_DOWN, 1}, {Z_FLAT, 17},
        {L_LEFT, 19},
    },
    /* first T_UP lands on supporting column pair 4 */
    {
        {T_UP, 8}, {Z_FLAT, 10}, {Z_FLAT, 12}, {Z_FLAT, 14},
        {Z_FLAT, 16}, {Z_FLAT, 18}, {I_FLAT, 9}, {I_FLAT, 13},
        {I_FLAT, 10}, {T_UP, 17}, {I_FLAT, 14}, {S_FLAT, 6},
        {S_FLAT, 7}, {S_FLAT, 4}, {S_FLAT, 5}, {S_FLAT, 2},
        {S_FLAT, 3}, {S_FLAT, 0}, {T_RIGHT, 0}, {T_DOWN, 1},
        {L_LEFT, 19},
    },
};

static int sprite[FIELD_HEIGHT][FIELD_WIDTH]; /* color slot of each pixel */
static int field[FIELD_HEIGHT][FIELD_WIDTH];  /* color of each square */
static int screen[FIELD_HEIGHT][FIELD_WIDTH]; /* what the terminal shows */
static int color_uses[COLOR_COUNT];
static int palette[COLOR_COUNT];        /* color slot to terminal color */
static int slot_of_color[COLOR_COUNT];  /* terminal color to color slot */

static void draw_cell(int column, int row, int color)
{
    printf(MOVE_AND_SET_BACKGROUND "  ", row, column, color);
}

/* Redraw only the squares that changed since the last call */
static void render_field(void)
{
    int row, column;

    for (row = 0; row < FIELD_HEIGHT; row++) {
        for (column = 0; column < FIELD_WIDTH; column++) {
            int color = field[row][column];

            if (color != screen[row][column]) {
                draw_cell(FIELD_LEFT_COLUMN + 2 * column, FIELD_TOP_ROW + row,
                          color != EMPTY ? color : BLACK);
                screen[row][column] = color;
            }
        }
    }
    fflush(stdout);
}

static void draw_walls_and_floor(void)
{
    int i;

    for (i = 0; i < BORDER_LENGTH; i++) {
        if (i >= FIELD_TOP_ROW) {
            draw_cell(LEFT_WALL_COLUMN, i, WHITE);
            draw_cell(RIGHT_WALL_COLUMN, i, WHITE);
        }
        draw_cell(LEFT_WALL_COLUMN + i, FLOOR_ROW, WHITE);
    }
}

/* Fill the squares of a piece whose top is at the given row */
static void paint_piece(const Rotation *piece, int top, int left, int mirrored,
                        int color)
{
    int i;

    for (i = 0; i < SQUARES_PER_PIECE; i++) {
        int column = left + piece->squares[i].x;

        field[top + piece->squares[i].y][mirrored ? LAST_COLUMN - column
                                                  : column] = color;
    }
}

static int piece_has_landed(const Rotation *piece, int top, int left,
                            int mirrored)
{
    int i, landed = 0;

    for (i = 0; i < SQUARES_PER_PIECE; i++) {
        int column = left + piece->squares[i].x;
        int below = top + piece->squares[i].y + 1;

        if (mirrored) {
            column = LAST_COLUMN - column;
        }
        if (below == FIELD_HEIGHT || field[below][column] != EMPTY) {
            landed = 1;
        }
    }
    return landed;
}

/* Remove each full row and let everything above it move down by one */
static void clear_full_rows(void)
{
    int row, column, above;

    for (row = 0; row < FIELD_HEIGHT; row++) {
        for (column = 0; column < FIELD_WIDTH; column++) {
            if (field[row][column] == EMPTY) {
                break;
            }
        }
        if (column == FIELD_WIDTH) {
            for (above = row; above > 0; above--) {
                for (column = 0; column < FIELD_WIDTH; column++) {
                    field[above][column] = field[above - 1][column];
                }
            }
            for (column = 0; column < FIELD_WIDTH; column++) {
                field[0][column] = EMPTY;
            }
        }
    }
}

/* Spawn a piece at the top of the field and let it fall until it lands */
static void drop_piece(int rotation, int left, int mirrored)
{
    const Rotation *piece = &rotations[rotation];
    int color = palette[mirrored ? piece->mirrored_slot : piece->slot];
    int top = 0;
    int falling = 1;

    while (falling) {
        if (top > 0) {
            paint_piece(piece, top - 1, left, mirrored, EMPTY);
        }
        if (piece_has_landed(piece, top, left, mirrored)) {
            falling = 0;
        }
        paint_piece(piece, top, left, mirrored, color);
        top++;
        render_field();
    }
    clear_full_rows();
    render_field();
}

/* Returns the index of the last row, or -1 when there is no input */
static int read_sprite(void)
{
    static char line[LINE_SIZE];
    int last_row = -1;
    int row, column;

    for (row = 0; row < FIELD_HEIGHT; row++) {
        for (column = 0; column < FIELD_WIDTH; column++) {
            sprite[row][column] = EMPTY;
        }
    }
    while (fgets(line, LINE_SIZE, stdin)) {
        last_row++;
        for (column = 0; column < SPRITE_WIDTH; column++) {
            char pixel = line[column];

            line[column] = 0; /* so a shorter next line ends with blanks */
            if (pixel >= '0' && pixel <= '7') {
                int color = pixel - '0';

                color_uses[color]++;
                sprite[last_row][column + SPRITE_LEFT_MARGIN] = color;
            }
        }
    }
    return last_row;
}

/* Give the colors of the sprite the slots of the pieces that will emit them */
static void assign_palette(int last_row)
{
    int next_slot = 0;
    int i, row, column;

    for (i = 0; i < COLOR_COUNT; i++) {
        palette[i] = COLOR_COUNT - 1 - i;
    }
    for (i = 0; i < COLOR_COUNT; i++) {
        if (color_uses[COLOR_COUNT - 1 - i]) {
            int color = palette[next_slot];

            palette[next_slot] = palette[i];
            palette[i] = color;
            next_slot++;
        }
    }
    for (i = 0; i < COLOR_COUNT; i++) {
        slot_of_color[palette[i]] = i;
    }
    for (row = 0; row <= last_row; row++) {
        for (column = 0; column < FIELD_WIDTH; column++) {
            if (sprite[row][column] != EMPTY) {
                sprite[row][column] = slot_of_color[sprite[row][column]];
            }
        }
    }
}

/* Leave one square of a color slot behind in a sprite column */
static void emit_square(int column, int slot)
{
    int left = column - slot;
    int mirrored = left >= EMITTER_COUNT;
    const Emitter *emitter = &emitters[mirrored ? LAST_EMITTER_LEFT - left : left];
    int i;

    drop_piece(slot, left, 0);
    for (i = 0; i < emitter->length; i++) {
        drop_piece(emitter->drops[i].rotation, emitter->drops[i].column,
                   mirrored);
    }
}

/* Print the sprite one row at a time, from the bottom up */
static void play_sprite(int last_row)
{
    int support = 0; /* column pair of the last square that can hold a platform */
    int row, column, i;

    for (row = last_row; row >= 0; row--) {
        int unsupported = 0;
        int platform = 0;
        int mirrored = 0;

        if (row != last_row) {
            for (column = 0; column < SPRITE_COLUMNS; column++) {
                if (sprite[row + 1][column] != EMPTY) {
                    support = column / 2;
                } else if (sprite[row][column] != EMPTY) {
                    unsupported++;
                }
            }
            if (unsupported) {
                platform = support;
                if (platform >= PLATFORM_COUNT) {
                    platform = 2 * PLATFORM_COUNT - 1 - platform;
                    mirrored = 1;
                }
                for (i = 0; i < PLATFORM_DROPS - 1; i++) {
                    drop_piece(platforms[platform][i].rotation,
                               platforms[platform][i].column, mirrored);
                }
            }
        }
        for (column = 0; column < SPRITE_COLUMNS; column++) {
            if (sprite[row][column] != EMPTY) {
                emit_square(column, sprite[row][column]);
            }
        }
        if (unsupported) {
            drop_piece(platforms[platform][PLATFORM_DROPS - 1].rotation,
                       platforms[platform][PLATFORM_DROPS - 1].column, mirrored);
        }
    }
}

int main()
{
    int last_row;
    int row, column;

    printf(CLEAR_SCREEN HIDE_CURSOR);
    last_row = read_sprite();
    assign_palette(last_row);
    draw_walls_and_floor();
    for (row = 0; row < FIELD_HEIGHT; row++) {
        for (column = 0; column < FIELD_WIDTH; column++) {
            field[row][column] = EMPTY;
        }
    }
    play_sprite(last_row);
    printf(MOVE_BELOW_FIELD SHOW_CURSOR BLACK_BACKGROUND);
    return system("reset");
}
