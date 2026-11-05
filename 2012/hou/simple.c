#include <stdio.h>
#include <string.h>

/*
 * Regular expression based text rewriter.
 *
 * The syntax file holds one rule per line: a regular expression, a space,
 * and a printf format that replaces the matched text.  Every rule is compiled
 * into one shared program for a small virtual machine, in the style of Russ
 * Cox's "Regular Expression Matching: the Virtual Machine Approach".  The
 * machine runs all possible matches in parallel as threads, reading each input
 * character only once, so no expression can make it slow.
 *
 * Program layout:
 *
 *   0  not executed, marks whether the run loop should continue
 *   1  split to 5 and fall through       \
 *   2  consume any character, go to 4     | search loop: a new match can
 *   3  not reached                        | begin at every input character
 *   4  jump to 1                         /
 *   5  split to the rule after this one, fall through to this rule
 *   .. code of the rule, ending in OP_MATCH
 *   .. split to the next rule, and so on
 */

#define BUFFER_SIZE (1 << 18)
#define PROGRAM_SIZE (BUFFER_SIZE + 321)

/* Jumping here ends the thread, because the machine never executes 0 */
#define PC_DEAD 0
#define PC_SEARCH_LOOP 1
#define PC_FIRST_RULE 5

typedef enum {
    OP_CONSUME, /* wait for the next character, then go to pc + 2 */
    OP_JUMP,    /* go to target right now */
    OP_SPLIT,   /* start a thread at target and fall through to pc + 1 */
    OP_MATCH    /* rule number target matched, this thread is finished */
} Opcode;

/*
 * An instruction only fires when character is 0 or equals the current input
 * character.  Otherwise execution falls through to the next instruction.
 */
typedef struct {
    char character;
    int target;
    Opcode op;
    char *visited;     /* input position of the last visit by any thread */
    char *match_start; /* where the match of the arriving thread began */
} Instruction;

/* The text covered by a rule and the format used to replace it */
typedef struct {
    char *format;
    char *start;
    char *end;
} Match;

/* A parenthesised group that is still being compiled */
typedef struct {
    int start;        /* placeholder in front of the group */
    int pending_jump; /* jump to patch with the end of the group, or 0 */
} Group;

static Instruction program[PROGRAM_SIZE] = {
    /* Non-empty string that lets the run loop begin, see run_program */
    [0] = { .visited = "Make a contract with me" },
    [PC_SEARCH_LOOP] = { .target = PC_FIRST_RULE, .op = OP_SPLIT },
    [2] = { .op = OP_CONSUME },
    [3] = { .op = OP_JUMP, .visited = "and become a magical girl! " },
    [4] = { .target = PC_SEARCH_LOOP, .op = OP_JUMP },
    [PC_FIRST_RULE] = { .target = PC_DEAD, .op = OP_JUMP },
};

/* Compiler state */
static int next_pc = PC_FIRST_RULE + 1;
static int atom_start = PC_FIRST_RULE; /* placeholder before the last atom */
static int rule_split = PC_FIRST_RULE; /* instruction to link the next rule */
static int rule_count = 0;
static char *rule_formats[BUFFER_SIZE];
static Group groups[BUFFER_SIZE];
static int group_depth = 1;

/* Virtual machine state */
static int threads[BUFFER_SIZE];
static int thread_count;
static char *cursor; /* read position in the syntax file, then the input */
static char *thread_match_start;
static Match matches[BUFFER_SIZE];
static int match_count = 0;

static char syntax_text[BUFFER_SIZE];
static char input_text[BUFFER_SIZE];

static void emit(char character, int target, Opcode op)
{
    program[next_pc].character = character;
    program[next_pc].target = target;
    program[next_pc].op = op;
    next_pc++;
}

static void emit_fail(void)
{
    emit(0, PC_DEAD, OP_JUMP);
}

/*
 * Every atom begins with a jump to the following instruction.  This does
 * nothing until an operator after the atom replaces it to wrap the atom.
 */
static void emit_placeholder(void)
{
    atom_start = next_pc;
    emit(0, next_pc + 1, OP_JUMP);
}

/* "x": on a mismatch the fall through reaches emit_fail and the thread dies */
static void compile_character(char character)
{
    emit_placeholder();
    emit(character, 0, OP_CONSUME);
    emit_fail();
}

static void compile_literal(void)
{
    if (*cursor == '\\') {
        cursor++;
        if (*cursor == 'n')
            *cursor = '\n';
    }
    compile_character(*cursor);
}

/* . */
static void compile_any(void)
{
    compile_character(0);
}

/*
 * x*
 *
 *   atom:      jump to decide
 *   atom + 1:  code of x
 *   after:     jump to split
 *   decide:    placeholder
 *   split:     split to atom + 1, fall through out of the loop
 */
static void compile_star(void)
{
    int atom = atom_start;

    program[atom].character = 0;
    program[atom].target = next_pc + 1;
    program[atom].op = OP_JUMP;
    emit(0, next_pc + 2, OP_JUMP);
    emit_placeholder();
    emit(0, atom + 1, OP_SPLIT);
}

/* x+: the atom has already run once when the split is reached */
static void compile_plus(void)
{
    emit(0, atom_start + 1, OP_SPLIT);
}

/* x?: a star whose body jumps past the split instead of back to it */
static void compile_optional(void)
{
    compile_star();
    program[next_pc - 3].target++;
}

static void push_group(int start)
{
    group_depth++;
    groups[group_depth].start = start;
    groups[group_depth].pending_jump = 0;
}

static void patch_pending_jump(void)
{
    int jump = groups[group_depth].pending_jump;

    if (jump)
        program[jump].target = next_pc;
}

/* ( */
static void open_group(void)
{
    int start = next_pc;

    emit_placeholder();
    push_group(start);
}

/* ) */
static void close_group(void)
{
    patch_pending_jump();
    atom_start = groups[group_depth].start;
    group_depth--;
}

/*
 * x|y: built from the star layout, which already has a split between
 * the last atom and the code following it.  The jump at the end of x is kept
 * as pending and is patched to the end of the group, so that x does not loop.
 * This is why | only applies to the single atom on its left.
 */
static void compile_alternation(void)
{
    patch_pending_jump();
    groups[group_depth].pending_jump = next_pc;
    compile_star();
}

/* "text": the operators after it apply to the whole string */
static void compile_quoted(void)
{
    int start;

    emit_placeholder();
    start = atom_start;
    for (cursor++; *cursor != '"'; cursor++)
        compile_literal();
    atom_start = start;
}

/*
 * [abc] becomes the alternation a|b|c|fail.  The last choice fails one
 * character later, after it consumes any character.
 *
 * [^abc] becomes a chain of tests that kill the thread on a, b or c,
 * followed by the code for any character.
 */
static void compile_class(void)
{
    int start;
    int negated;

    emit_placeholder();
    start = atom_start;

    cursor++;
    negated = *cursor == '^';
    if (negated)
        cursor++;

    push_group(0);
    for (; *cursor != ']'; cursor++) {
        char member = *cursor;

        if (cursor[1] == '-')
            cursor += 2;
        for (; member <= *cursor; member++) {
            emit_placeholder();
            if (negated) {
                emit(member, PC_DEAD, OP_JUMP);
            } else {
                emit(member, 0, OP_CONSUME);
                emit_fail();
                compile_alternation();
            }
        }
    }

    compile_any();
    if (!negated)
        emit_fail();
    close_group();
    atom_start = start;
}

/* The space that ends a regular expression, followed by the format */
static void compile_rule_end(void)
{
    cursor++;
    rule_formats[rule_count] = cursor;
    cursor = strchr(cursor, '\n');

    emit(0, rule_count, OP_MATCH);
    rule_count++;

    /* Earlier rules are started first by the split chain */
    program[rule_split].character = 0;
    program[rule_split].target = next_pc;
    program[rule_split].op = OP_SPLIT;
    rule_split = next_pc;
    emit_placeholder();
}

static void compile_syntax(void)
{
    for (cursor = syntax_text; *cursor; cursor++) {
        switch (*cursor) {
        case '*':
            compile_star();
            break;
        case '+':
            compile_plus();
            break;
        case '"':
            compile_quoted();
            break;
        case '[':
            compile_class();
            break;
        case '(':
            open_group();
            break;
        case ')':
            close_group();
            break;
        case '|':
            compile_alternation();
            break;
        case '.':
            compile_any();
            break;
        case ' ':
            compile_rule_end();
            break;
        case '?':
            compile_optional();
            break;
        default:
            compile_literal();
            break;
        }
    }

    /* The placeholder after the last rule falls into this */
    emit_fail();
}

/*
 * Record that a rule matched from thread_match_start up to cursor.
 *
 * Matches are kept in order and never overlap.  A new match ends later than
 * all others, so it replaces those that begin inside it, unless one of them
 * has the same text and comes from an earlier rule.  If it would overlap the
 * remaining match then that older match wins and nothing changes.
 */
static void record_match(int rule)
{
    char *format = rule_formats[rule];
    int previous_count = match_count;

    while (match_count > 0) {
        Match *last = &matches[match_count - 1];
        int same_text = last->end == cursor
                        && last->start == thread_match_start;

        if (last->start < thread_match_start
            || (same_text && format > last->format))
            break;
        match_count--;
    }

    if (match_count == 0
        || matches[match_count - 1].end <= thread_match_start) {
        matches[match_count].format = format;
        matches[match_count].start = thread_match_start;
        matches[match_count].end = cursor;
        match_count++;
    } else {
        match_count = previous_count;
    }
}

/*
 * Run one thread until it needs the next input character or dies.  The return
 * value is where it continues on the next character, or PC_DEAD.
 */
static int run_thread(int pc)
{
    thread_match_start = program[pc].match_start;

    /* The visited check means each instruction runs once per character */
    while (pc != PC_DEAD && program[pc].visited != cursor) {
        Instruction *instruction = &program[pc];

        /* Threads that meet here continue with the latest match start */
        if (instruction->match_start
            && instruction->match_start > thread_match_start)
            thread_match_start = instruction->match_start;

        instruction->visited = cursor;

        if (instruction->character && instruction->character != *cursor) {
            pc++;
            continue;
        }

        switch (instruction->op) {
        case OP_CONSUME:
            program[pc + 2].match_start = thread_match_start;
            return pc + 2;
        case OP_JUMP:
            pc = instruction->target;
            break;
        case OP_SPLIT:
            program[instruction->target].match_start = thread_match_start;
            threads[thread_count++] = instruction->target;
            pc++;
            break;
        case OP_MATCH:
            record_match(instruction->target);
            return PC_DEAD;
        }
    }

    return PC_DEAD;
}

/*
 * Step all threads through the input.  program[0].visited is the position of
 * the current step, so the loop stops after the step that saw the terminating
 * NUL.  The first test reads the catch phrase instead of a position.
 */
static void run_program(void)
{
    threads[0] = PC_SEARCH_LOOP;
    thread_count = 1;

    while (*program[0].visited) {
        int next_count = 0;
        int i;

        program[0].visited = cursor;
        program[PC_SEARCH_LOOP].match_start = cursor;

        /*
         * Threads for the next step reuse the front of the list.  This never
         * overwrites a waiting thread because each thread adds at most one.
         * The list can grow during the loop because of OP_SPLIT.
         */
        for (i = 0; i < thread_count; i++) {
            int resume_pc = run_thread(threads[i]);

            if (resume_pc != PC_DEAD)
                threads[next_count++] = resume_pc;
        }

        thread_count = next_count;
        cursor++;
    }
}

static void read_file(char *buffer, const char *path)
{
    buffer[fread(buffer, 1, BUFFER_SIZE, fopen(path, "r"))] = 0;
}

/* Print with the text ending at end, which may be in the middle of a buffer */
static void print_up_to(char *end, const char *format, const char *text)
{
    char saved = *end;

    *end = 0;
    /* Formats can use %s up to three times, as with <a %s>%s</a> */
    printf(format, text, text, text);
    *end = saved;
}

int main(int argc, char **argv)
{
    if (argc >= 3) {
        char *printed_up_to;
        int i;

        read_file(syntax_text, argv[1]);
        compile_syntax();

        for (i = 0; i < rule_count; i++)
            *strchr(rule_formats[i], '\n') = 0;

        read_file(input_text, argv[2]);
        cursor = input_text;
        run_program();

        printed_up_to = input_text;
        for (i = 0; i < match_count; i++) {
            print_up_to(matches[i].start, "%s", printed_up_to);
            print_up_to(matches[i].end, matches[i].format, matches[i].start);
            printed_up_to = matches[i].end;
        }
        printf("%s", printed_up_to);
    }

    return 0;
}
