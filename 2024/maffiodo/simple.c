/*
 * The original entry has no #include lines; its Makefile passes
 * -include stdio.h -include string.h -include stdlib.h -include ctype.h
 * on the command line instead. They are written out here for clarity.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* value types (stored in Node.type) */
#define TYPE_UNDEFINED 0
#define TYPE_NUMBER    1
#define TYPE_STRING    2   /* also used for functions: str holds "{ body }" */
#define TYPE_OBJECT    4   /* objects and arrays */

/* statement control flags (stored in the global `flags`) */
#define FLAG_RETURN 1
#define FLAG_BREAK  2
#define FLAG_SKIP   4      /* currently parsing code that must not run */

#define MAX_NAME 99
#define MAX_SOURCE (1 << 16)

/*
 * One struct is used for everything: values, variables, object properties
 * and the linked lists that hold them.
 */
typedef struct node Node;
struct node {
    union {
        char *str;              /* string contents / function body source */
        double num;             /* number value */
    };
    char *name;                 /* variable or property name */
    char mark;                  /* garbage collector mark */
    char retained;              /* garbage collector "keep" flag */
    char type;                  /* TYPE_* */
    Node *owner;                /* value: the variable/property holding it */
    Node *value;                /* variable/property: its current value */
    Node *next;                 /* next node in a linked list */
    Node *props;                /* head of the property list */
    Node *(*native)(Node *);    /* native C function, if any */
    int length;                 /* number of properties */
};

Node *scope;            /* linked list of all visible variables */
Node *heap;             /* linked list of every allocated value */
int gc_mark_value;
char *source_start;
char *src;              /* parser cursor into the source code */
char ch;
char flags = 0;         /* FLAG_* bits */
char name_buf[MAX_NAME];  /* current identifier / scratch text buffer */

/* allocate a new value and push it on the heap list */
Node *new_value(int type)
{
    Node *node = calloc(1, sizeof(struct node));
    node->next = heap;
    node->type = type;
    return heap = node;
}

/* find the node called name_buf in a list */
Node *find_property(Node *list)
{
    while (list) {
        if (!strcmp(name_buf, list->name))
            return list;
        list = list->next;
    }
    return 0;
}

/* add a new node called name_buf (holding an undefined value) to a list */
Node *declare_property(Node **list)
{
    Node *node = new_value(TYPE_OBJECT);
    node->name = strdup(name_buf);
    node->next = *list;
    node->value = new_value(TYPE_UNDEFINED);
    return *list = node;
}

/* skip white space and // comments */
void skip_space(void)
{
    int comment;

    while ((comment = (src[0] == '/' && src[1] == '/')) | isspace(*src)) {
        if (comment) {
            while (*src && *src != '\n')
                src++;
        } else {
            src++;
        }
    }
}

/* skip white space, then one character such as ')' ']' '{' or ';' */
void skip_char(void)
{
    skip_space();
    if (*src)
        src++;
}

/* position of the current character in set, -1 if missing
 * (the end of the source matches the terminating '\0' of set) */
int char_index(char *set)
{
    char *found = strchr(set, *src);
    return found ? found - set : -1;
}

int at_expression_end(void)
{
    return char_index(";)]") != -1;
}

int name_is(char *word)
{
    return !strcmp(word, name_buf);
}

/* consume a keyword if it is next in the source */
int match_keyword(char *keyword)
{
    int len = strlen(keyword);

    if (!strncmp(keyword, src, len) && !isalnum(src[len])) {
        src += len;
        return 1;
    }
    return 0;
}

/* skip to just after the '}' that closes an already opened '{' */
void skip_block(void)
{
    int depth = 1;

    while (*src && depth) {
        char c = *src++;
        if (c == '{')
            depth++;
        else if (c == '}')
            depth--;
    }
}

double to_number(Node *value)
{
    char *end;

    if (value->type & TYPE_NUMBER)
        return value->num;
    if (value->type & TYPE_STRING)
        return strtod(value->str, &end);
    return 0;
}

Node *to_string(Node *value)
{
    Node *result = new_value(TYPE_STRING);
    char *text = "[object]";
    char type = value->type;

    if (type & TYPE_NUMBER) {
        sprintf(name_buf, "%g", value->num);
        text = name_buf;
    }
    if (type & TYPE_STRING)
        text = value->str;
    result->str = strdup(text);
    return result;
}

Node *execute(int whole_block);

int is_ident_char(void)
{
    return *src == '_' || isalnum(*src);
}

void read_identifier(void)
{
    char *out = name_buf;

    while (is_ident_char())
        *out++ = *src++;
    *out = 0;
}

/*
 * Loose equality / ordering: returns 0 when equal, otherwise a value
 * whose sign tells the order.
 */
int loose_compare(Node *left, char left_type, Node *right,
                  double left_num, double right_num)
{
    if (left_type == right->type) {
        if (left_type & TYPE_OBJECT)
            return left != right;            /* same object? */
        if (left_type & TYPE_NUMBER)
            return left_num - right_num;
        return strcmp(to_string(left)->str, to_string(right)->str);
    }
    if ((left_type | right->type) == TYPE_STRING)
        return strcmp(to_string(left)->str, to_string(right)->str);
    return left_num - right_num;
}

/* comparison kinds */
enum { CMP_EQ, CMP_GE, CMP_LE, CMP_GT, CMP_LT, CMP_NE };

int compare(int kind, Node *left, char left_type, Node *right,
            double left_num, double right_num)
{
    int diff = loose_compare(left, left_type, right, left_num, right_num);

    switch (kind) {
    case CMP_EQ: return diff == 0;
    case CMP_GE: return diff >= 0;
    case CMP_LE: return diff <= 0;
    case CMP_GT: return diff > 0;
    case CMP_LT: return diff < 0;
    case CMP_NE: return diff != 0;
    }
    return diff;
}

/* obj.name / obj[name]: find (or create) property name_buf of obj */
Node *member(Node *obj)
{
    Node *prop = find_property(obj->props);
    Node *value;

    if (!prop) {
        prop = declare_property(&obj->props);
        obj->length++;
    }
    value = prop->value;
    if (!value->owner)
        value->owner = prop;
    return value;
}

/* binary operators; the position in this string is the precedence */
enum {
    OP_COMMA, OP_ASSIGN, OP_NOT, OP_AND, OP_OR, OP_GREATER, OP_LESS,
    OP_SUB, OP_ADD, OP_MUL, OP_DIV, OP_MOD, OP_INDEX, OP_CALL, OP_DOT
};
#define BINARY_OPS ",=!&|><-+*/%[(."

/* unary operators */
enum { UN_NEG, UN_PLUS, UN_NOT, UN_BITNOT };
#define UNARY_OPS "-+!~"

/* parse and evaluate an expression containing operators of precedence
 * min_prec or higher */
Node *evaluate(int min_prec)
{
    int op, unary, left_int, right_int;
    double left_num, right_num;
    Node *result, *right, *var;
    Node *x = new_value(TYPE_UNDEFINED);

    skip_space();
    if (at_expression_end())
        return x;

    unary = char_index(UNARY_OPS);
    if (unary > -1)
        src++;

    /* primary expression */
    ch = *src;
    if (ch == '(') {
        src++;
        x = evaluate(0);
        skip_char();                         /* ')' */
    } else if (ch == '[' || ch == '{') {     /* only [] and {} literals */
        x->type = TYPE_OBJECT;
        src++;
        skip_char();
    } else if (ch == '"' || ch == '\'') {
        char *out = name_buf;
        src++;
        x->type = TYPE_STRING;
        while (*src != ch) {
            char c = *src++;
            if (c == '\\') {
                c = *src++;
                if (c == 'n')
                    c = '\n';
                else if (c == 'x')
                    c = strtol(src, &src, 16);
            }
            *out++ = c;
        }
        *out = 0;
        src++;
        x->str = strdup(name_buf);
    } else if (isdigit(*src)) {
        x->type = TYPE_NUMBER;
        x->num = strtod(src, &src);
    } else if (is_ident_char()) {
        int is_declaration = 0;
        for (;;) {
            read_identifier();
            if (name_is("function")) {
                /* function () { body } -> a string holding "{ body }" */
                char *body;
                skip_char();                 /* '(' */
                skip_char();                 /* ')' */
                skip_char();                 /* '{' */
                body = src - 1;
                x = new_value(TYPE_STRING);
                skip_block();
                x->str = strndup(body, src - body);
                break;
            }
            if (name_is("var")) {
                src++;
                is_declaration = 1;
                continue;
            }
            var = find_property(scope);
            if (is_declaration || !var)
                var = declare_property(&scope);
            x = var->value;
            x->owner = var;
            break;
        }
    }

    if (unary > -1) {
        left_num = to_number(x);
        left_int = left_num;
        switch (unary) {
        case UN_NEG:    left_num = -left_num; break;
        case UN_PLUS:   break;
        case UN_NOT:    left_num = !left_int; break;
        case UN_BITNOT: left_num = ~left_int; break;
        }
        x = new_value(TYPE_NUMBER);
        x->num = left_num;
    }

    /* binary operators */
    while (*src) {
        char op_char, left_type;
        int assign, doubled, kind, is_comparison = 0;

        skip_space();
        if (at_expression_end())
            break;
        op = char_index(BINARY_OPS);
        if (op < 0) {
            src++;
            continue;
        }
        op_char = *src;
        if (op < min_prec)
            break;

        assign = 0;     /* 1 for "=", 2 for "op=" */
        kind = CMP_EQ;
        doubled = 0;    /* "&&", "||", ">>", "<<" */
        src++;
        if (*src == '=') {
            assign = 2;
            src++;
        } else if (*src == op_char) {
            doubled = 1;
            src++;
        }

        right = 0;
        result = new_value(TYPE_NUMBER);
        left_num = to_number(x);
        left_type = x->type;
        left_int = left_num;
        if (op < OP_CALL) {
            right = evaluate(op == OP_INDEX ? 0 : op);
            right_num = to_number(right);
            right_int = right_num;
        }

        switch (op) {
        case OP_COMMA:
            result = right;
            break;
        case OP_ASSIGN:
            if (assign) {                    /* "==" */
                kind = CMP_EQ;
                is_comparison = 1;
                break;
            }
            assign = 1;                      /* "=" */
            if (right->type & TYPE_OBJECT) {
                result = right;              /* objects by reference */
            } else {
                result = new_value(right->type);
                result->native = right->native;
                if (right->type & TYPE_STRING)
                    result->str = strdup(right->str);
                else
                    result->num = right->num;
            }
            break;
        case OP_NOT:                         /* "!=" */
            kind = CMP_NE;
            is_comparison = 1;
            break;
        case OP_AND:
            result->num = doubled ? left_int && right_int : left_int & right_int;
            break;
        case OP_OR:
            result->num = doubled ? left_int || right_int : left_int | right_int;
            break;
        case OP_GREATER:
            if (doubled) {
                result->num = left_int >> right_int;
                break;
            }
            kind = assign ? CMP_GE : CMP_GT;
            is_comparison = 1;
            break;
        case OP_LESS:
            if (doubled) {
                result->num = left_int << right_int;
                break;
            }
            kind = assign ? CMP_LE : CMP_LT;
            is_comparison = 1;
            break;
        case OP_SUB:
            result->num = left_num - right_num;
            break;
        case OP_ADD:
            if ((left_type | right->type) & TYPE_STRING) {
                Node *a = to_string(x), *b = to_string(right);
                char *joined = malloc(strlen(a->str) + strlen(b->str) + 1);
                sprintf(joined, "%s%s", a->str, b->str);
                result->type = TYPE_STRING;
                result->str = joined;
            } else {
                result->num = left_num + right_num;
            }
            break;
        case OP_MUL:
            result->num = left_num * right_num;
            break;
        case OP_DIV:
            result->num = left_num / right_num;
            break;
        case OP_MOD:
            result->num = left_int % right_int;
            break;
        case OP_INDEX:
            skip_char();                     /* ']' */
            if (left_type & TYPE_STRING) {   /* character of a string */
                sprintf(name_buf, "%c",
                        right_int < (int)strlen(x->str) ? x->str[right_int] : 0);
                result->type = TYPE_STRING;
                result->str = strdup(name_buf);
            } else {
                sprintf(name_buf, "%s", to_string(right)->str);
                result = member(x);
            }
            break;
        case OP_CALL: {
            Node *saved_scope = scope, *args;
            int count = 0, saved_flags;

            sprintf(name_buf, "arguments");
            args = declare_property(&scope)->value;
            while (*src) {
                skip_space();
                right = evaluate(OP_ASSIGN);
                if (!flags) {
                    sprintf(name_buf, "%d", count++);
                    declare_property(&args->props)->value = right;
                    args->length++;
                }
                if (*src == ')')
                    break;
                src++;                       /* ',' */
            }
            src++;                           /* ')' */
            saved_flags = flags;
            if (x->native) {
                if (!flags)
                    result = x->native(args->props);
            } else {
                char *return_to = src;
                src = x->str;                /* run the function body */
                result = execute(0);
                src = return_to;
            }
            if (!saved_flags)
                flags = 0;                   /* clear the "return" flag */
            scope = saved_scope;             /* drop arguments and locals */
            break;
        }
        case OP_DOT:
            skip_space();
            read_identifier();
            if (name_is("length"))
                result->num = left_type & TYPE_STRING ? (int)strlen(x->str) : x->length;
            else
                result = member(x);
            break;
        }
        if (is_comparison) {
            result->num = compare(kind, x, left_type, right, left_num, right_num);
            assign = 0;
        }

        if (assign) {
            if (!flags)
                result = x->owner->value = result;
            else
                continue;
        }
        x = result;
    }
    return x;
}

/* garbage collector mark phase */
void gc_mark(Node *node, Node *stop, int follow_next)
{
    if (node && node != stop) {
        Node *value = node->value;

        node->mark = gc_mark_value;
        if (value) {
            if (value->mark != gc_mark_value)
                gc_mark(value, 0, 0);
            gc_mark(value->props, 0, 1);
        }
        gc_mark(node->props, 0, 1);
        if (follow_next)
            gc_mark(node->next, stop, 1);
    }
}

/* mark & sweep garbage collector (never called by the program) */
void gc_collect(Node *stop)
{
    Node **link = &heap, *node = heap;

    gc_mark_value = 1;
    gc_mark(heap, stop, 1);
    gc_mark_value = 0;
    gc_mark(scope, 0, 1);
    while (node) {
        Node **next = &node->next;
        if (node->mark & !node->retained) {
            *link = *next;
            if (node->type & TYPE_STRING)
                free(node->str);
            if (node->name)
                free(node->name);
            free(node);
        } else {
            node->mark = 0;
            link = next;
        }
        node = *next;
    }
}

/* run one statement, or (whole_block) statements up to the closing '}' */
Node *execute(int whole_block)
{
    Node *result = new_value(TYPE_UNDEFINED), *stmt, *cond;
    Node *saved_scope = scope, *heap_start = heap;

    while (*src) {
        skip_space();
        if (*src == '{') {
            src++;
            if (!flags)
                result = execute(1);
            else
                skip_block();
        } else {
            int is_loop = 0, saved_flags;

            if (*src == '}') {
                src++;
                break;
            } else if (match_keyword("if") || (is_loop = match_keyword("while"))) {
                char *cond_start;
                int skipped;

                saved_flags = flags;
                for (;;) {
                    cond_start = src;
                    skip_char();             /* '(' */
                    cond = evaluate(0);
                    skip_char();             /* ')' */
                    if (!(int)cond->num)
                        flags = FLAG_SKIP;
                    skipped = flags;
                    stmt = execute(0);
                    if (*src == ';')
                        src++;
                    if (!skipped)
                        result = stmt;
                    skip_space();
                    if (!is_loop || flags || skipped)
                        break;
                    src = cond_start;        /* loop: test the condition again */
                }
                if (!is_loop && match_keyword("else")) {
                    flags = (!skipped & !saved_flags) * FLAG_SKIP;
                    skipped = flags;
                    stmt = execute(0);
                    if (!skipped)
                        result = stmt;
                }
                if (is_loop)
                    flags = saved_flags & ~FLAG_BREAK;
                else
                    flags = saved_flags | (flags & (FLAG_RETURN | FLAG_BREAK));
            } else if (match_keyword("return")) {
                stmt = evaluate(0);
                if (!flags) {
                    result = stmt;
                    flags |= FLAG_RETURN;
                }
            } else if (match_keyword("break")) {
                if (!flags)
                    flags |= FLAG_BREAK;
            } else if (*src) {
                stmt = evaluate(0);
                if (!flags)
                    result = stmt;
                skip_char();                 /* ';' */
            }
        }
        if (!whole_block)
            break;
    }
    result->retained = 1;   /* a gc_collect(heap_start) call belonged here */
    result->retained = 0;
    scope = saved_scope;    /* forget variables declared in this block */
    return result;
}

/* the only native function: print(value) */
Node *native_print(Node *args)
{
    printf("%s", to_string(args->value)->str);
    return new_value(TYPE_UNDEFINED);
}

int main(void)
{
    Node *print;
    char source[MAX_SOURCE];

    sprintf(name_buf, "print");
    print = declare_property(&scope);
    print->type = TYPE_STRING;
    print->value->native = native_print;

    source_start = src = source;
    while ((ch = getchar()) > 0)
        *src++ = ch;
    *src = 0;
    src = source_start;
    execute(1);
}
