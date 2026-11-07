/*
 * 2025/cesmoak - deobfuscated
 *
 * One program, four jobs.  The job is chosen from the first two bytes of
 * standard input:
 *
 *   prog.c source  -> "card punch": decode the whitespace hidden in prog.c
 *                     into a FORTRAN program and print it as punch cards
 *   punch cards    -> "IBM 7040":   run the FORTRAN program on the cards
 *   flux numbers   -> "pen":        stipple the numbers into a PGM image
 *   PGM image      -> "darkroom":   turn the stippled negative into a print
 *
 * The headers stdio.h, stdlib.h, string.h, math.h and complex.h are supplied
 * by the Makefile with -include.
 */

typedef float _Complex complex_float;

#define ARRAY_SIZE (1 << 23)

/*
 * IBM "9 code" 6-bit BCD character set.  The index of a character is its BCD
 * value.  A few slots hold the three bytes of the UTF-8 punch hole glyph
 * U+258C (0xe2 0x96 0x8c) and a newline, which are used for drawing cards.
 * The PGM header format string is stored right after the table.
 */
char *CHARSET =
    "0123456789"        /*  0 - 9                                     */
    "\x8c" "='  \n"     /* 10 - 15  (0x8c = last byte of the chad)     */
    "+ABCDEFGHI"        /* 16 - 25                                    */
    "\x96" ".)   "      /* 26 - 31  (0x96 = middle byte of the chad)   */
    "-JKLMNOPQR"        /* 32 - 41                                    */
    "\xe2" "$*    "     /* 42 - 48  (0xe2 = first byte, 48 = blank)    */
    "/STUVWXYZ"         /* 49 - 57                                    */
    "|,("               /* 58 - 60                                    */
    "\0"                /* 61                                         */
    "P%d %d %d %d \0";  /* 62 - 75  PGM header                         */

#define PGM_HEADER (CHARSET + 62)

/* BCD values of the characters the interpreter cares about */
enum {
    BCD_EQUALS = 11,
    BCD_PLUS = 16,
    BCD_A = 17,
    BCD_F = 22,
    BCD_G = 23,
    BCD_I = 25,
    BCD_DOT = 27,
    BCD_RPAREN = 28,
    BCD_MINUS = 32,
    BCD_N = 37,
    BCD_O = 38,
    BCD_ASTERISK = 44,
    BCD_BLANK = 48,
    BCD_SLASH = 49,
    BCD_W = 54,
    BCD_X = 55,
    BCD_COMMA = 59,
    BCD_LPAREN = 60,
    BCD_END_OF_STATEMENT = 62   /* separator in the decoded program */
};

/* punch pattern of the letter C (rows 12 and 3): a comment card */
#define PUNCH_C 33

/*
 * The first 38 decoded values are the pieces used to draw a punch card:
 *   0..19  four-character left edges: top, line 1, line 2, middle, bottom
 *   20     "-"  border cell
 *   22     the chad glyph (punched hole)
 *   26     " "  unpunched cell
 *   28     "-." end of top border
 *   31     " |" end of a middle line
 *   34     "-'\n" end of the bottom border (puts adds the blank line)
 * The FORTRAN program follows them.
 */
#define CARD_PIECES 38
#define CARD_LEFT_EDGE 0
#define CARD_CELL_BORDER 20
#define CARD_CELL_HOLE 22
#define CARD_CELL_EMPTY 26
#define CARD_END_TOP 28
#define CARD_END_MIDDLE 31
#define CARD_END_BOTTOM 34

/* where the width digits are patched into the decoded FORTRAN "K=1200" */
#define WIDTH_DIGITS 53

/* layout of the whitespace hidden in prog.c */
#define HIDDEN_DATA_START 465   /* bytes of prog.c before the hidden data */
#define RUN_VALUES 219          /* values coming from run lengths         */
#define GROUP_VALUES 603        /* values coming from 8-whitespace groups */

/* ---------------------------------------------------------------------- */

int input_kind = 10;            /* (first byte + second byte) % 9 */

char program_text[ARRAY_SIZE];  /* decoded program, or text of the cards */
int decoded_length;             /* length of the decoded program */

char line_buffer[ARRAY_SIZE];   /* current input line */

int card_punches[ARRAY_SIZE];   /* punch pattern of each column of each card */
int *card = card_punches + 5;
char *card_text = program_text;

/* FORTRAN interpreter state */
complex_float variables[ARRAY_SIZE];  /* indexed by hash of the name */
int label_to_statement[ARRAY_SIZE];
int statement_label[ARRAY_SIZE];
char *statement_start[ARRAY_SIZE];

/*
 * The numbers read one per line (pen and darkroom input) share the
 * variable table and the counter with the statements read from cards.
 */
complex_float *values = variables;
int record_count;

int do_limit[ARRAY_SIZE];
int do_step[ARRAY_SIZE];
int do_end_label[ARRAY_SIZE];
int do_variable[ARRAY_SIZE];
int do_statement[ARRAY_SIZE];
int do_depth;

char *cursor;                   /* points at the current character */
char current;                   /* the current character (BCD) */
char number_text[ARRAY_SIZE];
int number_found;

int pixels[ARRAY_SIZE];

/* ---------------------------------------------------------------------- */

/*
 * Punch pattern of a BCD character.  Bit 0 = row 12, bit 1 = row 11,
 * bit 2 = row 0 and bits 3..11 = rows 1..9.
 */
int punch_pattern(int code)
{
    int zone, digit;

    if (code == BCD_BLANK)
        return 0;
    zone = code / 16 + code / 48 + !code * 4;
    if (code % 16 == 9)
        digit = 2048;
    else
        digit = code / 8 % 2 << 10 | (code & 7 ? 4 << (code & 7) : 0);
    return zone | digit;
}

/* BCD character for a punch pattern (64 when nothing matches) */
int character_for_punches(int punches)
{
    int code = 0;

    while (code < 64 && punch_pattern(code) != punches)
        code++;
    return code;
}

char advance(void)
{
    return current = *++cursor;
}

int is_letter(int c)
{
    return c - 33 < 9u | c - 50 < 8u | c - 17 < 9u;
}

int is_alphanumeric(int c)
{
    return c < 10 | is_letter(c);
}

/*
 * Read a name.  Only the first three characters count; they are packed
 * into a base 64 number which is used as the index of the variable.
 */
int read_identifier(void)
{
    int hash = 0, significant = 3;

    while (is_alphanumeric(current)) {
        if (significant) {
            hash = hash * 64 + current + 1;
            significant--;
        }
        advance();
    }
    return hash;
}

/* Read a number with strtof; number_found says whether there was one */
float read_number(void)
{
    char *end;
    int k, length;
    float value;

    for (k = 0; k < 32; k++)
        number_text[k] = CHARSET[cursor[k]];
    number_text[k] = 0;
    value = strtof(number_text, &end);
    length = end - number_text;
    number_found = length > 0;

    /* in "1.EQ.2" the dot belongs to the operator, not to the number */
    if (length > 0 && number_text[length - 1] == '.' && is_letter(cursor[length]))
        length--;
    cursor += length;
    current = *cursor;
    return value;
}

complex_float parse_expression(int level);

/* ( expression ), number, variable or function call */
complex_float parse_primary(void)
{
    complex_float value, second = 0;
    float x, y;
    int name;

    if (current == BCD_LPAREN) {
        advance();
        value = parse_expression(0);
        advance();
        return value;
    }

    value = read_number();
    if (number_found)
        return value;

    name = read_identifier();
    value = variables[name];
    if (current != BCD_LPAREN)
        return value;

    /* function call with one or two arguments */
    advance();
    value = parse_expression(0);
    if (current == BCD_COMMA) {
        advance();
        second = parse_expression(3);
    }
    advance();
    x = value;
    y = second;

    switch (name % 28) {
    case 20:    /* CMPLX */
        value = x + y * I;
        break;
    case 25:    /* AIMAG: real part of -i*z is the imaginary part of z */
        y = -I * value;
        value = y;
        break;
    case 19:    /* COS */
        value = cos(x);
        break;
    case 15:    /* CABS */
        value = cabs(value);
        break;
    case 3:     /* AMOD */
        value = fmod(x, y);
        break;
    case 23:    /* ALOG */
        value = log(x);
        break;
    case 11:    /* ABS */
        value = fabs(x);
        break;
    case 4:     /* INT */
        value = (int)x;
        break;
    case 10:    /* SIN */
        value = sin(x);
        break;
    default:    /* REAL */
        value = x;
        break;
    }
    return value;
}

/* binary operators; relational ones are found by (name hash % 13) */
enum {
    OP_AND, OP_OR, OP_GT, OP_LT, OP_SUB, OP_POW, OP_EQ,
    OP_ADD, OP_DIV, OP_MUL, OP_NE, OP_GE, OP_LE
};

/*
 * Precedence climbing.  Levels:
 *   0 .OR.   1 .AND. (.NOT.)   2 relations   3 + -   4 * / (unary -)
 *   5 **     6 primary
 * All operators are left associative.  Logical and relational operators
 * only look at the real parts.
 */
complex_float parse_expression(int level)
{
    complex_float value, right;
    float left_real, right_real;

    if (level && (current == BCD_DOT) & (cursor[1] == BCD_N)) {
        /* .NOT. */
        cursor += 4;
        advance();
        value = !parse_expression(1);
    } else if ((level > 2) & (current == BCD_MINUS)) {
        /* unary minus binds looser than * / ** */
        advance();
        value = -parse_expression(4);
    } else if (level > 5) {
        value = parse_primary();
    } else {
        value = parse_expression(level + 1);
    }

    for (;;) {
        char *operator_start = cursor;
        int op = 0, op_level, extra = 0;

        if (current == BCD_DOT) {
            advance();
            if (current == BCD_O) {             /* .OR. */
                op = OP_OR;
                op_level = 0;
                extra = 2;
            } else if (current == BCD_A) {      /* .AND. */
                op = OP_AND;
                op_level = 1;
                extra = 3;
            } else {                            /* .LT. .LE. .GT. .GE. .EQ. .NE. */
                op = read_identifier() % 13;
                op_level = 2;
            }
        } else if (current == BCD_PLUS) {
            op = OP_ADD;
            op_level = 3;
        } else if (current == BCD_MINUS) {
            op = OP_SUB;
            op_level = 3;
        } else if (current == BCD_SLASH) {
            op = OP_DIV;
            op_level = 4;
        } else if (current == BCD_ASTERISK && cursor[1] == BCD_ASTERISK) {
            op = OP_POW;
            op_level = 5;
            extra = 1;
        } else if (current == BCD_ASTERISK) {
            op = OP_MUL;
            op_level = 4;
        } else {
            op_level = 7;                       /* not an operator */
        }

        if (op_level != level) {
            cursor = operator_start;
            current = *cursor;
            return value;
        }
        cursor += extra;
        advance();

        left_real = value;
        right = parse_expression(level + 1);
        right_real = right;

        switch (op) {
        case OP_AND: value = left_real && right_real; break;
        case OP_OR:  value = left_real || right_real; break;
        case OP_GT:  value = left_real > right_real; break;
        case OP_LT:  value = left_real < right_real; break;
        case OP_SUB: value = value - right; break;
        case OP_POW: value = cpow(value, right); break;
        case OP_EQ:  value = left_real == right_real; break;
        case OP_ADD: value = value + right; break;
        case OP_DIV: value = value / right; break;
        case OP_MUL: value = value * right; break;
        case OP_NE:  value = left_real != right_real; break;
        case OP_GE:  value = left_real >= right_real; break;
        default:     value = left_real <= right_real; break;
        }
    }
}

/* ---------------------------------------------------------------------- */
/* FORTRAN statements                                                       */

/* WRITE(u,label) list  together with its  label FORMAT(...) */
void write_statement(void)
{
    char *format;
    int arg_count = 0, item;

    cursor += 7;                        /* skip "WRITE(u," */
    advance();
    format = statement_start[label_to_statement[(int)read_number()]] + 6;

    /* the arguments are kept in the low slots of the variable table */
    while (advance() != BCD_BLANK)
        variables[arg_count++] = parse_expression(0);

    cursor = format;                    /* the '(' after FORMAT */
    advance();
    for (item = 0; item < arg_count || current != BCD_RPAREN; item++) {
        float count = read_number();
        int letter = current;

        if (count) {
            /* nX prints n blanks, nHxxx prints n characters */
            int k;
            for (k = 0; k < count; k++)
                putchar(CHARSET[letter != BCD_X ? advance() : BCD_BLANK]);
            advance();
            item--;                     /* uses no argument */
        } else {
            /* Iw or Fw.d */
            int spec;
            float value;

            advance();
            spec = read_number() * 10;  /* w.d -> w*10 + d */
            if (letter != BCD_F)
                value = (int)crealf(variables[item]);
            else
                value = crealf(variables[item]);
            printf("%*.*f", spec / 10, spec % 10, value);
        }
        if (current == BCD_COMMA)
            advance();
    }
    puts("");
}

/* run one statement; returns the (possibly changed) program counter */
int execute_statement(char *statement, int pc)
{
    int first_symbol;

    /* find the first character that is not part of a name or number */
    cursor = statement - 1;
    while (is_alphanumeric(advance()))
        ;
    first_symbol = current;
    cursor = statement - 1;
    advance();

    if (first_symbol != BCD_LPAREN) {
        int name = read_identifier();

        if (current == BCD_EQUALS) {
            complex_float value;

            advance();
            value = parse_expression(3);
            if (current == BCD_COMMA) {
                /* DO label var = start, limit [, step] */
                advance();
                do_limit[do_depth] = crealf(parse_expression(3));
                if (current == BCD_COMMA) {
                    advance();
                    do_step[do_depth] = crealf(parse_expression(3));
                } else {
                    do_step[do_depth] = 1;
                }
                cursor = statement + 1;     /* skip "DO" */
                advance();
                do_end_label[do_depth] = read_number();
                variables[do_variable[do_depth] = read_identifier()] = value;
                do_statement[do_depth++] = pc;
            } else {
                /* name = expression */
                variables[name] = value;
            }
        } else {
            cursor = statement + 3;
            current = *statement;
            if (current == BCD_G) {
                /* GOTO label  (a variable set by ASSIGN works too) */
                advance();
                pc = label_to_statement[(int)crealf(parse_expression(3))] - 1;
            } else if (current == BCD_A) {
                /* ASSIGN label TO name */
                float label;

                advance();
                advance();
                advance();
                label = read_number();
                advance();
                advance();
                variables[read_identifier()] = label;
            }
            /* anything else (CONTINUE, END, COMPLEX ...) is ignored */
        }
    } else if (current == BCD_I) {
        /* IF (condition) statement */
        int condition;

        advance();
        advance();
        advance();
        condition = crealf(parse_expression(0));
        advance();
        if (condition)
            return execute_statement(cursor, pc);
    } else if (current == BCD_W) {
        write_statement();
    }
    return pc;
}

void run_program(void)
{
    int pc;

    for (pc = 0; pc < record_count; pc++) {
        int this_statement = pc;
        int label, again;

        pc = execute_statement(statement_start[pc], pc);

        /* does this statement end DO loops?  (not when it jumped away) */
        label = statement_label[this_statement];
        do {
            again = 0;
            if (label && pc == this_statement && do_depth) {
                int loop = do_depth;

                while (loop-- && do_end_label[loop] != label)
                    ;
                if (loop != -1) {
                    float value;

                    do_depth = loop;
                    value = crealf(variables[do_variable[loop]] += do_step[loop]);
                    if (value <= do_limit[loop]) {
                        do_depth++;
                        pc = do_statement[loop];
                    } else if (loop && do_end_label[loop - 1] == label) {
                        again = 1;
                    }
                }
            }
        } while (again);
    }
}

/* ---------------------------------------------------------------------- */
/* reading the input                                                       */

/* turn the punches of a finished card into program text */
void store_card(void)
{
    int not_comment = card[0] != PUNCH_C;
    int column6 = card[5] * not_comment;
    int continuation = column6 && column6 != 4;   /* 4 = '0' punch */
    int length = continuation ? -1 : 1;           /* -1 appends to the previous card */
    int column;

    card[5] = 0;
    for (column = 0; column < 80; column++)
        card_text[column] = BCD_BLANK;

    for (column = 0; column < 72 - continuation * 6; column++) {
        int punches = card[column + continuation * 6] * not_comment;

        if (punches)
            card_text[length++] = character_for_punches(punches);   /* blanks are dropped */

        if (column == 4 && !continuation) {
            /* end of the label field: start a new statement */
            cursor = card_text;
            advance();
            statement_label[record_count] = read_number();
            label_to_statement[statement_label[record_count]] = record_count;
            statement_start[record_count++] = cursor;
        }
    }
    card += 99;
    card_text += ++length;
}

/* value of one whitespace character in the base 5 hidden data */
int whitespace_digit(int ch)
{
    if (ch > 13)
        return 0;       /* space */
    if (ch == 9)
        return 1;       /* tab */
    if (ch == 10)
        return 4;       /* newline */
    return ch - 9;      /* vertical tab 2, form feed 3, carriage return 4 */
}

void read_input(void)
{
    int ch, sum = 0, bytes_read = 0;
    int pos = 0, dashes = 10, line_in_card = 0, inside_card = 0, utf8_pending = 0;
    int run_length = 0, run_bits = 0, run_count = 0;
    int group_bits = 0, group_count = 0, group_values = 0;

    while ((line_buffer[pos] = ch = getchar()) != (char)EOF) {
        if (++bytes_read < 3) {
            sum += ch;
            input_kind = (bytes_read - 1) * sum % 9;
        }

        if (ch == '\n') {
            if (input_kind > 2)
                values[record_count++] = atof(line_buffer + 1);
            line_in_card++;
            if (dashes > 19) {
                /* a card border: the bottom one finishes the card */
                if (inside_card)
                    store_card();
                line_in_card = 0;
                inside_card = !inside_card;
            }
            pos = dashes = 0;
        }
        dashes += ch == '-';

        /* a chad: first byte of the UTF-8 hole glyph on a punch row */
        if (ch == 0xe2 && inside_card * line_in_card) {
            utf8_pending = 2;
            card[pos - 5] |= 1 << (line_in_card - 1);
        }
        pos += --utf8_pending < 0;  /* the glyph counts as one column */

        /* whitespace hidden in prog.c */
        if (!input_kind & ch < 33 & bytes_read > HIDDEN_DATA_START & group_values < GROUP_VALUES) {
            run_length++;
            group_bits = group_bits * 5 + whitespace_digit(ch);
            if (++group_count > 7) {
                /* eight whitespace characters give three 6-bit values */
                int k;
                for (k = 0; k < 3; k++) {
                    program_text[RUN_VALUES + group_values++] = group_bits % 64;
                    group_bits /= 64;
                }
                group_count = 0;
            }
        } else if (run_length) {
            /* each run of 1 to 4 whitespace characters gives 2 bits */
            run_bits = run_bits * 4 + run_length - 1;
            if (run_count++ > 1) {
                program_text[decoded_length] =
                    decoded_length < CARD_PIECES ? CHARSET[run_bits] : run_bits;
                decoded_length++;
                run_bits = run_count = 0;
            }
            run_length = 0;
        }
    }
    decoded_length += group_values;
}

/* ---------------------------------------------------------------------- */
/* output                                                                  */

/* print the decoded program as a deck of punch cards */
void punch_cards(void)
{
    char *statement = program_text + CARD_PIECES;
    char *end = statement - 1;
    int k;

    for (k = 0; k < decoded_length - CARD_PIECES; k++) {
        int label, cell;

        if (*++end != BCD_END_OF_STATEMENT)
            continue;

        label = *statement < 10 ? *statement++ : BCD_BLANK;   /* one digit label */

        /* 15 lines of 80 columns */
        for (cell = 0; cell < 1200; cell++) {
            int line = cell / 80, column = cell % 80;
            int code, piece, edge;

            if (column == 4)
                code = label;
            else if (column > 5 && statement + column - 6 < end)
                code = statement[column - 6];
            else
                code = BCD_BLANK;

            if (line == 0 || line == 14)
                piece = CARD_CELL_BORDER;
            else if (line >= 2 && code != BCD_BLANK && punch_pattern(code) >> (line - 2) & 1)
                piece = CARD_CELL_HOLE;
            else
                piece = CARD_CELL_EMPTY;    /* line 1 is left for writing */

            edge = line < 3 ? line : line < 14 ? 3 : 4;
            if (column == 0)
                printf("%.4s", program_text + CARD_LEFT_EDGE + edge * 4);
            printf("%s", program_text + piece);

            if (column == 79)
                puts(program_text + (line == 0 ? CARD_END_TOP :
                                     line == 14 ? CARD_END_BOTTOM : CARD_END_MIDDLE));
        }
        statement = end + 1;
    }
}

/*
 * mode 2 (pen): stipple dots whose size depends on the flux, removing the
 * pixels around each dot along a spiral (a Poisson disk pattern).
 * mode 1 (darkroom): spread each black pixel of the negative along the
 * same spiral, adding up the light.
 * With a command line argument the input values are drawn directly.
 */
void render_image(int mode, int direct)
{
    int width = sqrt(record_count / 2) * 2;
    int additive = mode < 2;
    int limit = 16 / mode >> mode;
    int pass, k;

    printf(PGM_HEADER, 2, width, width / 2, 255);

    for (pass = 0; pass < mode * mode * mode * mode * 2; pass++) {
        for (k = 0; k < record_count; k++) {
            int row = k / width, column = k % width;
            float random = drand48(), flux = crealf(values[k]);
            float radius = flux;
            unsigned size_class;

            if (radius)
                radius = (radius < .3 ? .3 : radius < .5 ? 4 * radius - .9 : 1.1)
                         - 1.9 * log(radius) / pow(radius, .03)
                         - (random < .05) - cos(flux + random);
            if (additive)
                radius = !flux;

            if (pass == 0)
                pixels[k] = radius > 0;             /* candidate dot */
            else if (pass == 31)
                pixels[k] = !pixels[k] * 6;         /* invert: paper is white */

            size_class = radius;
            if (pixels[k] && size_class == pass) {
                complex_float center = column + row * I;
                int step;

                for (step = 0; step < 12 * radius * radius / mode; step++) {
                    /* golden angle spiral, rotated differently for every pixel */
                    complex_float point = center + sqrt(step) * .45 *
                                          cexp((step * 2.4 + k * 7.8) * I);
                    unsigned x = 1 + crealf(point);
                    unsigned y = 1 + cimagf(point);

                    x--;
                    y--;
                    if (x < width && y < width / 2 && pixels[y * width + x] < limit)
                        pixels[y * width + x] = pixels[y * width + x] * additive + additive;
                }
                if (!additive)
                    pixels[k] = 3;                  /* the dot itself */
            }
        }
    }

    for (k = 0; k < record_count; k++)
        printf("%d\n", direct ? (int)creal(values[k] * 89.2) : pixels[k] * 31);
}

int main(int argc, char **argv)
{
    int mode;

    read_input();

    /* the first argument (the image width) replaces "1200  " in the program */
    if (--argc) {
        int k;
        for (k = 0; k < 6; k++)
            program_text[k + WIDTH_DIGITS] = *argv[1] ? *argv[1]++ - '0' : BCD_BLANK;
    }

    /* "#d" -> 0, "  " -> 1 (text);  "P2" -> 4 -> 1 (darkroom);  " 0" -> 8 -> 2 (pen) */
    mode = input_kind >> 2;
    if (mode) {
        render_image(mode, argc);
    } else {
        run_program();
        punch_cards();
    }
    return 0;
}
