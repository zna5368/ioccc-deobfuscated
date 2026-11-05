#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/*
 * Every record in the program is the same kind of Node, and each kind of
 * record uses only some of the fields:
 *
 *   string     chars, length
 *   list       items, count
 *   segment    a string, or a reference: is_reference set and name is the list
 *              of segments that spell the name of the variable
 *   variable   name, value, origin
 *   target     name, rules, modified_time, visit_count
 *   rule       prerequisites, recipes (a list of segment lists, one per line)
 *   frame      segments, stop_chars, parent (one open $( or ${ in the parser)
 */
typedef struct Node Node;

struct Node {
    union {
        int is_reference;
        int modified_time;
    };
    union {
        int length;
        int count;
        int origin;
        int visit_count;
    };
    union {
        char *chars;
        char *stop_chars;
    };
    Node **items;
    union {
        Node *name;
        Node *recipes;
    };
    union {
        Node *value;
        Node *rules;
        Node *prerequisites;
    };
    Node *segments;
    Node *parent;
};

/* The result of parsing up to a stop character */
typedef struct {
    Node *segments;
    char stop;
    int position;
} ParseResult;

/* A lower origin has a higher priority */
enum {
    ORIGIN_AUTOMATIC,
    ORIGIN_COMMAND_LINE,
    ORIGIN_MAKEFILE,
    ORIGIN_ENVIRONMENT
};

enum {
    MODE_MAKEFILE, /* comments are removed and a newline ends the text */
    MODE_RECIPE,   /* no comments and backslash newline is kept for the shell */
    MODE_VALUE     /* no comments and a newline is plain text */
};

static Node *variables;
static Node *targets;
static Node *goals;
static struct stat file_info;
static int build_status;

static Node *new_node(void)
{
    return calloc(1, sizeof(Node));
}

/* The terminating NUL of the set means that a NUL byte is whitespace too */
static int is_whitespace(int c)
{
    return strchr(" \n\t", c) != NULL;
}

static void list_append(Node *list, Node *item)
{
    list->items = realloc(list->items, ++list->count * sizeof(Node *));
    list->items[list->count - 1] = item;
}

/* The string shares chars with the caller, nothing is copied */
static Node *string_new(char *chars, int length)
{
    Node *string = new_node();
    string->chars = chars ? chars : malloc(1);
    string->length = length;
    return string;
}

static Node *string_from_c(char *chars)
{
    return string_new(chars, strlen(chars));
}

static int string_equal(Node *a, Node *b)
{
    return a->length == b->length && !memcmp(a->chars, b->chars, a->length);
}

static void append_char(Node *segments, char c)
{
    char *chars = malloc(1);
    *chars = c;
    list_append(segments, string_new(chars, 1));
}

static void trim_leading_whitespace(Node *string)
{
    while (string->length && is_whitespace(*string->chars)) {
        string->chars++;
        string->length--;
    }
}

static void trim_trailing_whitespace(Node *string)
{
    while (string->length && is_whitespace(string->chars[string->length - 1])) {
        string->length--;
    }
}

static Node *split_words(Node *string)
{
    Node *words = new_node();
    int start = 0;

    while (start < string->length) {
        while (start < string->length && is_whitespace(string->chars[start])) {
            start++;
        }
        int end = start;
        while (end < string->length && !is_whitespace(string->chars[end])) {
            end++;
        }
        if (end != start) {
            list_append(words, string_new(string->chars + start, end - start));
        }
        start = end + 1;
    }
    return words;
}

static Node *find_variable(Node *name)
{
    for (int i = 0; i < variables->count; i++) {
        if (string_equal(variables->items[i]->name, name)) {
            return variables->items[i];
        }
    }
    return NULL;
}

static void set_variable(char *chars, int length, int origin, Node *value)
{
    Node *name = string_new(chars, length);
    Node *variable = find_variable(name);

    if (variable == NULL) {
        variable = new_node();
        list_append(variables, variable);
        variable->name = name;
    } else if (origin > variable->origin) {
        return;
    }
    variable->origin = origin;
    variable->value = value;
}

/*
 * Parse text into a list of segments, starting at position and stopping at
 * the end of the line, at the end of the source or at one of stop_chars. The
 * stop_chars should include '$' for references to be recognized.
 */
static ParseResult parse(Node *source, int position, char *stop_chars, int mode)
{
    Node *frame = new_node();
    frame->segments = new_node();
    frame->stop_chars = stop_chars;

    int length = source->length;
    char *text = source->chars;
    int current_mode = mode;
    int inside_recipe_reference = 0;
    int in_comment = 0;
    char stop = 0;

    while (position < length) {
        char *active_stops = in_comment ? "" : frame->stop_chars;
        int start = position;
        Node *piece = NULL;

        /* Find the next character that ends the current piece of text */
        while (position < length) {
            int backslashes = 0;
            while (position < length && text[position] == '\\') {
                backslashes++;
                position++;
            }
            stop = text[position++];

            if (stop != '\n' || backslashes == 0 || current_mode == MODE_MAKEFILE) {
                int starts_comment = stop == '#' && !inside_recipe_reference
                    && current_mode == MODE_MAKEFILE;
                int ends_line = stop == '\n' && current_mode != MODE_VALUE;
                int can_be_escaped = starts_comment || ends_line;

                if (can_be_escaped || strchr(active_stops, stop)) {
                    /* Each pair of backslashes stands for one backslash */
                    int removed = can_be_escaped ? backslashes / 2 : 0;
                    piece = string_new(text + start, position - start - removed - 1);

                    /* An odd backslash cancels the '#' or the newline, which
                     * then becomes a space, and the text carries on */
                    if (can_be_escaped && backslashes % 2) {
                        piece->chars[piece->length - 1] = stop == '#' ? '#' : ' ';
                        stop = 0;
                    }
                    break;
                }
            }
            stop = 0;
        }

        if (!in_comment) {
            if (piece == NULL) {
                piece = string_new(text + start, length - start);
            }
            list_append(frame->segments, piece);
        }

        Node *node = new_node();

        if (stop == '\n') {
            break;
        } else if (stop == '#') {
            in_comment = 1;
        } else if (stop == '$') {
            char next = text[position++];

            if (next == '$') {
                append_char(frame->segments, next);
            } else if (next == '(' || next == '{') {
                node->parent = frame;
                node->segments = new_node();
                node->stop_chars = next == '(' ? ")$" : "}$";
                frame = node;
            } else {
                node->name = new_node();
                append_char(node->name, next);
                node->is_reference = 1;
                list_append(frame->segments, node);
            }
        } else if (stop != 0) {
            if (frame->parent == NULL) {
                break;
            }

            /* The closing bracket of a reference */
            node->name = frame->segments;
            frame = frame->parent;
            node->is_reference = 1;
            list_append(frame->segments, node);
        }

        /* The name inside a reference in a recipe is parsed like Makefile text */
        inside_recipe_reference = frame->parent != NULL && current_mode == MODE_RECIPE;
        current_mode = inside_recipe_reference ? MODE_MAKEFILE : mode;
    }

    ParseResult result = { frame->segments, stop, position };
    return result;
}

static Node *expand(Node *segments);

static Node *expand_reference(Node *reference)
{
    Node *name = expand(reference->name);
    Node *text = string_new(NULL, 0);
    Node *variable = find_variable(name);

    if (variable != NULL) {
        text = expand(parse(variable->value, 0, "$", MODE_VALUE).segments);
    }
    return text;
}

/* Join the segments into one string with all references replaced */
static Node *expand(Node *segments)
{
    Node *result = string_new(NULL, 0);

    for (int i = 0; i < segments->count; i++) {
        Node *piece = segments->items[i];

        if (piece->is_reference) {
            piece = expand_reference(piece);
        }
        result->chars = realloc(result->chars, result->length += piece->length);
        memcpy(result->chars + result->length - piece->length, piece->chars, piece->length);
    }
    return result;
}

static Node *find_target(Node *name)
{
    for (int i = 0; i < targets->count; i++) {
        if (string_equal(targets->items[i]->name, name)) {
            return targets->items[i];
        }
    }

    Node *target = new_node();
    target->name = name;
    target->rules = new_node();
    target->modified_time = -1;
    list_append(targets, target);
    return target;
}

/* The time is refreshed on every visit but a target is only built on the first */
static void build_target(Node *target)
{
    Node *name = target->name;

    if (!stat(memcpy(calloc(1, name->length + 1), name->chars, name->length), &file_info)) {
        target->modified_time = file_info.st_mtime;
    }
    if (target->visit_count++) {
        return;
    }

    /* A file that does not exist and has no rule to make it is an error */
    build_status = !target->rules->count && target->modified_time < 0;

    int out_of_date = target->modified_time < 0;
    Node *recipes = NULL;

    for (int i = 0; !build_status && i < target->rules->count; i++) {
        Node *rule = target->rules->items[i];

        for (int j = 0; !build_status && j < rule->prerequisites->count; j++) {
            Node *prerequisite = find_target(rule->prerequisites->items[j]);

            build_target(prerequisite);
            if (prerequisite->modified_time < 0
                || prerequisite->modified_time > target->modified_time) {
                out_of_date = 1;
            }
        }
        if (rule->recipes->count) {
            recipes = rule->recipes;
            if (rule->prerequisites->count) {
                set_variable("<", 1, ORIGIN_AUTOMATIC, rule->prerequisites->items[0]);
            }
        }
    }

    for (int i = 0; !build_status && recipes && out_of_date && i < recipes->count; i++) {
        set_variable("@", 1, ORIGIN_AUTOMATIC, name);

        Node *command = expand(recipes->items[i]);

        /* Recipe prefixes are skipped and have no effect. strchr also matches
         * the NUL at the end of the line, so a recipe that is empty here runs
         * off the end of its buffer and into the next block on the heap. The
         * allocations are in the same order as the original, even the ones
         * that are not used, so that it normally finds the same bytes. */
        while (command->length && strchr("@+-", *command->chars)) {
            command->chars++;
        }
        build_status = system(command->chars);
    }
}

/* Define each name=value entry and treat other command line words as goals */
static void read_definitions(char **entries, int origin)
{
    for (; *entries; entries++) {
        char *equals = *entries;

        while (*equals && *equals != '=') {
            equals++;
        }
        if (*equals) {
            set_variable(*entries, equals - *entries, origin, string_from_c(equals + 1));
        } else if (origin == ORIGIN_COMMAND_LINE) {
            list_append(goals, string_from_c(*entries));
        }
    }
}

/* Read the rule or variable assignment that starts at position */
static int read_statement(Node *makefile, int position, Node **current_rule)
{
    ParseResult left = parse(makefile, position, "=:$", MODE_MAKEFILE);
    ParseResult right;
    Node *value;
    Node *names;

    switch (left.stop) {
    case ':':
        right = parse(makefile, left.position, "$", MODE_MAKEFILE);

        Node *rule = new_node();
        *current_rule = rule;
        rule->prerequisites = split_words(expand(right.segments));
        rule->recipes = new_node();

        names = split_words(expand(left.segments));
        for (int i = 0; i < names->count; i++) {
            list_append(find_target(names->items[i])->rules, rule);
        }
        return right.position;

    case '=':
        /* There are no stop characters, the value is expanded when it is used */
        right = parse(makefile, left.position, "", MODE_MAKEFILE);

        value = expand(right.segments);
        trim_leading_whitespace(value);

        Node *name = expand(left.segments);
        trim_leading_whitespace(name);
        trim_trailing_whitespace(name);

        set_variable(name->chars, name->length, ORIGIN_MAKEFILE, value);
        return right.position;
    }

    /* Any other line such as an include directive is ignored */
    return left.position;
}

static void read_makefile(void)
{
    stat("Makefile", &file_info);

    int size = file_info.st_size;
    Node *makefile = string_new(NULL, 0);
    Node *current_rule = NULL;
    int position = 0;

    makefile->chars = malloc(size);
    makefile->length = read(open("Makefile", O_RDONLY), makefile->chars, size);

    while (position < size) {
        if (makefile->chars[position] == '\t') {
            ParseResult line = parse(makefile, position + 1, "$", MODE_RECIPE);

            position = line.position;
            append_char(line.segments, 0);
            list_append(current_rule->recipes, line.segments);
        } else {
            while (position < size && is_whitespace(makefile->chars[position])) {
                position++;
            }
            position = read_statement(makefile, position, &current_rule);
        }
    }
}

int main(int argc, char **argv, char **envp)
{
    (void)argc;

    variables = new_node();
    targets = new_node();
    goals = new_node();

    set_variable("MAKE", 4, ORIGIN_AUTOMATIC, string_from_c(argv[0]));
    read_definitions(argv + 1, ORIGIN_COMMAND_LINE);
    read_definitions(envp, ORIGIN_ENVIRONMENT);
    read_makefile();

    /* With no goals the first target in the Makefile is built */
    if (!goals->count && targets->count) {
        list_append(goals, targets->items[0]->name);
    }

    for (int i = 0; !build_status && i < goals->count; i++) {
        build_target(find_target(goals->items[i]));
    }
    return build_status;
}
