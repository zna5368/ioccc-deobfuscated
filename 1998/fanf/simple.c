#include <stdlib.h>
#include <stdio.h>

/*
 * A tiny graph-reduction machine for SK combinators (the "OFL" engine),
 * running a combinator program that reads a lambda expression and prints
 * the equivalent S/K/I combinator expression.
 *
 * Every value is a node.  The "eval" function pointer says what kind of
 * node it is:
 *
 *   apply      an application (left right)
 *   indirect   a node that has been reduced to the value of "left"
 *   S K I J    primitive combinators (J is "false": J x y -> y)
 *   character  a character constant (ch holds the character or EOF)
 *   put_char   P c f -> f I     after writing character c to stdout
 *   get_char   G f   -> f c     after reading character c from stdin
 *
 * Evaluating a node walks down the left spine of applications, recording
 * in "up" the application that contains it.  When a combinator is reached
 * it climbs back up one application per argument it needs and overwrites
 * the topmost application with the result.  If there are not enough
 * arguments the expression is already fully evaluated and is returned.
 */

struct node {
    int ch;                          /* character of a character node */
    struct node *left;               /* function part of an application */
    struct node *right;              /* argument part of an application */
    struct node *up;                 /* enclosing application while evaluating */
    struct node *(*eval)(struct node *);
};

static struct node *apply(struct node *p);
static struct node *indirect(struct node *p);
static struct node *combinator_I(struct node *p);
static struct node *combinator_J(struct node *p);
static struct node *combinator_K(struct node *p);
static struct node *combinator_S(struct node *p);
static struct node *character(struct node *p);
static struct node *put_char(struct node *p);
static struct node *get_char(struct node *p);

static struct node *new_node(struct node *(*eval)(struct node *))
{
    struct node *p = malloc(sizeof *p);
    p->ch = 0;
    p->left = p->right = p->up = NULL;
    p->eval = eval;
    return p;
}

static struct node *new_apply(struct node *function, struct node *argument)
{
    struct node *p = new_node(apply);
    p->left = function;
    p->right = argument;
    return p;
}

static struct node *new_character(int ch)
{
    struct node *p = new_node(character);
    p->ch = ch;
    return p;
}

/* (f x): evaluate f, remembering that it is applied in this node */
static struct node *apply(struct node *p)
{
    p->left->up = p;
    return p->left->eval(p->left);
}

/* this node has been replaced by the value of its left child */
static struct node *indirect(struct node *p)
{
    p->left->up = p->up;
    return p->left->eval(p->left);
}

/* I x -> x */
static struct node *combinator_I(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = I x */
    p->eval = indirect;
    p->left = p->right;
    return p->eval(p);
}

/* J x y -> y   (false) */
static struct node *combinator_J(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = J x */
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = J x y */
    p->eval = indirect;
    p->left = p->right;
    return p->eval(p);
}

/* K x y -> x   (true) */
static struct node *combinator_K(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = K x */
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = K x y */
    p->eval = indirect;
    p->left = p->left->right;
    return p->eval(p);
}

#if 0
/* Y f -> f (Y f), made by turning the node into a cycle (not used) */
static struct node *combinator_Y(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = Y f */
    p->left = p->right;
    p->right = p;
    return p->eval(p);
}
#endif

/* S f g x -> (f x) (g x) */
static struct node *combinator_S(struct node *p)
{
    struct node *f, *g, *x;

    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = S f */
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = S f g */
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = S f g x */
    f = p->left->left->right;
    g = p->left->right;
    x = p->right;
    p->left = new_apply(f, x);
    p->right = new_apply(g, x);
    return p->eval(p);
}

/*
 * Evaluate the argument of the application p to a character.
 * Returns 0 if it is not a character.
 */
static int evaluate_character_argument(struct node *p)
{
    p->right->up = NULL;
    p->right = p->right->eval(p->right);
    return p->right->eval == character;
}

/* 'c' x -> K if x is 'c', J if x is another character */
static struct node *character(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = 'c' x */
    if (!evaluate_character_argument(p))
        return p;
    p->eval = p->left->ch != p->right->ch ? combinator_J : combinator_K;
    return p->eval(p);
}

/* P c f -> f I, after printing c */
static struct node *put_char(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = P c */
    if (!evaluate_character_argument(p))
        return p;
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = P c f */
    putchar(p->left->right->ch);
    p->left = p->right;
    p->right = new_node(combinator_I);
    return p->eval(p);
}

/* G f -> f c, where c is read from stdin */
static struct node *get_char(struct node *p)
{
    if (!p->up)
        return p;
    p->up->left = p;
    p = p->up;                       /* p = G f */
    p->left = p->right;
    p->right = new_character(getchar());
    return p->eval(p);
}

/*
 * The combinator program.  Application is written by juxtaposition and is
 * left associative.  Every use of a name builds a fresh copy of its
 * definition.
 */

static const struct {
    const char *name;
    struct node *(*eval)(struct node *);
} primitives[] = {
    {"I", combinator_I},
    {"J", combinator_J},
    {"K", combinator_K},
    {"P", put_char},
    {"G", get_char},
    {"S", combinator_S},
};

static const struct {
    const char *name;
    int ch;
} characters[] = {
    {"ef", EOF},
    {"sp", ' '},
    {"ob", '('},
    {"cb", ')'},
    {"lm", '\\'},
    {"nl", '\n'},
    {"ht", '\t'},
    {"qs", 'S'},
    {"qk", 'K'},
    {"qi", 'I'},
};

static const struct {
    const char *name;
    const char *body;
} definitions[] = {
    /* standard library */
    {"B", "S (K S) K"},                     /* B f g x   -> f (g x)     */
    {"SS", "B (B S) B"},                    /* SS c f g x -> c (f x) (g x) */
    {"C", "SS B S (K K)"},                  /* C f g x   -> f x g       */
    {"CC", "B (B C) B"},                    /* CC c f g x -> c (f x) g   */
    {"BB", "CC B (B B B) B"},               /* BB c f g x -> c (f (g x)) */
    {"Y", "S (C B (S I I)) (C B (S I I))"}, /* Y f -> f (Y f)           */
    {"CI", "C I"},                          /* CI x f    -> f x         */

    /* expression trees: atom c, or pair f x for the application (f x) */
    {"pair", "BB (B (B K)) C CI"},
    {"atom", "B K CI"},

    /* continuation passing I/O: bind m f k -> m (\x(f x k)) */
    {"bind", "CC B B C"},

    /* ore p q x -> p x or q x */
    {"ore", "SS (CI K)"},

    /* read the next character that is not a space, newline or tab */
    {"gns", "Y (B (bind G) (CC S (C (ore sp (ore nl ht))) CI))"},

    /* print a tree: atom c as c, pair f x as (fx) */
    {"pr", "Y (CC C (BB CI (B (BB (bind (P ob)) K)) (SS C (CC C (BB BB bind) K) "
           "(C (CC bind) (K (P cb))))) P)"},

    /* read one term and translate it; ')' and EOF give the marker atom ' ' */
    {"trans", "Y (B (bind gns) (S (BB S (C lm) lam) (C (BB S (C ob) brac) "
              "(S (C cb (CI (atom sp))) (S (C ef (CI (atom sp))) (B CI atom))))))"},

    /* after '(': read terms up to the ')' marker, applying them in turn */
    {"brac", "S bind (B Y (C (BB B B bind) (S (BB S (BB S (S I)) "
             "(CC (BB (B K)) (BB K) pair)) (CC S (CC BB (BB C (C (CI sp)) CI)) pair))))"},

    /* after '\': read the variable and the body, then abstract it */
    {"lam", "B (bind gns) (C (CC BB bind (B CI)) abs)"},

    /* abs v v -> I,  abs v c -> K c,  abs v (f x) -> opt (abs v f) (abs v x) */
    {"abs", "Y (C (BB S (BB C CI) (SS S (BB C (BB B opt)) I)) "
            "(CC S (C C (atom qi)) (B (pair (atom qk)) atom)))"},

    /* make f g -> S f g */
    {"make", "B pair (pair (atom qs))"},

    /* S (K p) I -> p,  S (K p) (K r) -> K (p r),  otherwise S f g */
    {"opt", "S (S I (S (BB (CC B) CI (BB K K make)) (S (BB C (BB C (C (CI qk))) "
            "(SS (SS S) (S (BB (BB (S I)) S (BB (BB (CC B) CI) (BB K K) make)) "
            "(B (CC B (BB (CC C) (BB (C (CI qk)) (pair (atom qk))) pair)) make)) "
            "(B (C (BB B C (C (CI qi)))) make))) make))) (B K make)"},
};

/* main program: read and translate, print the result, then a newline */
static const char program[] = "bind (bind trans pr) (P nl) I";

#define COUNT(table) (sizeof(table) / sizeof(table[0]))

static struct node *parse_expression(const char **source);

static int is_letter(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void skip_spaces(const char **source)
{
    while (**source == ' ')
        ++*source;
}

/* does the C string candidate equal the first length characters of name? */
static int name_matches(const char *candidate, const char *name, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i)
        if (candidate[i] != name[i])
            return 0;
    return candidate[length] == '\0';
}

/* build a fresh copy of the named combinator */
static struct node *instantiate(const char *name, size_t length)
{
    size_t i;
    const char *body;

    for (i = 0; i < COUNT(primitives); ++i)
        if (name_matches(primitives[i].name, name, length))
            return new_node(primitives[i].eval);
    for (i = 0; i < COUNT(characters); ++i)
        if (name_matches(characters[i].name, name, length))
            return new_character(characters[i].ch);
    for (i = 0; i < COUNT(definitions); ++i)
        if (name_matches(definitions[i].name, name, length)) {
            body = definitions[i].body;
            return parse_expression(&body);
        }
    fprintf(stderr, "unknown name %.*s\n", (int)length, name);
    exit(1);
}

/* term := name | '(' expression ')' */
static struct node *parse_term(const char **source)
{
    struct node *term;
    const char *name;

    skip_spaces(source);
    if (**source == '(') {
        ++*source;
        term = parse_expression(source);
        skip_spaces(source);
        ++*source;                   /* the closing ')' */
        return term;
    }
    name = *source;
    while (is_letter(**source))
        ++*source;
    return instantiate(name, *source - name);
}

/* expression := term { term }, applied left to right */
static struct node *parse_expression(const char **source)
{
    struct node *expression = parse_term(source);

    for (;;) {
        skip_spaces(source);
        if (**source == '\0' || **source == ')')
            return expression;
        expression = new_apply(expression, parse_term(source));
    }
}

int main(void)
{
    const char *source = program;
    struct node *root = parse_expression(&source);

    return !root->eval(root);
}
