#include <stdio.h>
#include <stdlib.h>

/*
 * The text printed by the embedded Lazy K program. Each character is passed to
 * the program as one combinator: '@' is K, ' ' is I and a newline is S.
 */
static const char banner[] =
    "@@@@@\n"
    "@\n"
    "@@@@@\n"
    "    @\n"
    "@@@@@\n"
    "\n"
    "@   @\n"
    "@  @\n"
    "@@@\n"
    "@  @\n"
    "@   @\n"
    "\n"
    "@@@@@  @@@   @@@   @@@   @@@\n"
    "  @   @   @ @   @ @   @ @   @\n"
    "  @   @   @ @     @     @\n"
    "  @   @   @ @   @ @   @ @   @\n"
    "@@@@@  @@@   @@@   @@@   @@@\n";

/*
 * Combinator source text. Application is written by juxtaposition, so "S K K"
 * means ((S K) K). Besides S, K and I there are two built-in leaves:
 *   P  print: P n rest outputs the counter n and then continues with rest
 *   C  count: C x is the value of the counter x plus one, a bare C is zero
 */

/* Church numerals: the number n is the function that applies f n times */
#define CHURCH_ZERO   "K I"
#define SUCCESSOR     "S(S(K S)K)"
#define TWO           "(" SUCCESSOR "I)"
#define FOUR          "(S I I" TWO ")"                     /* 2^2     */
#define TEN           "(S(K" TWO ")(" SUCCESSOR FOUR "))"  /* 2*(4+1) */
#define THIRTY_TWO    "(S(K" TWO ")(S(S I I)I" TWO "))"    /* 2*2^4   */
#define SIXTY_FOUR    "(S(S(" SUCCESSOR "))I" TWO ")"      /* 4^3     */
#define TWO_FIFTY_SIX "(S I I(S I I" TWO "))"              /* 4^4     */

/* The Lazy K list [head, tail...] is the function \f. f head tail */
#define CONS(head, tail) "S(S I(K" head "))(K" tail ")"

/* Lazy K ends its output at any number above 255, 256 meaning exit status 0 */
#define END_OF_OUTPUT CONS(TWO_FIFTY_SIX, "(K K)")

/* \self list token number. self self (cons number list) */
#define PUSH_NUMBER                                                           \
    "S(K(S(K K)))(S(S(K S)(S(K K)(S(K S)(S(S(K K))I))))"                      \
    "(K(S(K(S(S(K S)(S(K(S I))K))))(S(K K)K))))"

/*
 * \push list token. Works out which token it was given by applying the token
 * to prepared arguments, and then calls push with the character code:
 *   K -> '@' (64), I -> ' ' (32), S -> '\n' (10), (K K) -> returns the list
 */
#define CLASSIFY_TOKEN                                                        \
    "S(S(K S)(S(S S S(K(S(K S)))(S(S S S(K(S(K S)))(S(K(S(S(K S)(S(S(K S)K)"  \
    "(K(S(S(K S)(S(K(S(S(S I(K K))(K K))))(S(K K)(S(S(K S)(S(K(S(S I(K K))))" \
    "(S(K K)(S(S(K K))(K" SIXTY_FOUR ")))))"                                  \
    "(S(K K)(S(S(K K))(K" TEN ")))))))"                                       \
    "(S(K K)(S(K K)(S(S(K K))(K" THIRTY_TWO "))))))))))"                      \
    "(S(K K)(S(K K)(S(K(S(S I(K(S(S I(K(K I)))(K S))))))K)))))"               \
    "(K(K(K K)))))(K(K(K(K I))))))"

/* \self list token. Decode one token and add its character to the list */
#define COLLECT_TOKEN "S(K(" CLASSIFY_TOKEN "))(" PUSH_NUMBER ")"

/* Takes the tokens one at a time, so the last token ends up at the list head */
#define BANNER_DECODER "S I I(" COLLECT_TOKEN ")(" END_OF_OUTPUT ")"
#define END_OF_TOKENS  "K K"

/* \number. number C C, which counts up to give the value of a Church numeral */
#define NUMERAL_TO_COUNT "S(S S(K I))(K C)"

/* \self list. Print the head of the list and call self on the tail */
#define PRINT_HEAD_STEP                                                       \
    "S(K(S I))(S(K K)(S(K(S(S(K P)(" NUMERAL_TO_COUNT "))))K))"

/* The Y combinator gives PRINT_HEAD_STEP its recursion */
#define Y_COMBINATOR "S S(" SUCCESSOR ")(K(S I I))"
#define PRINT_LIST   Y_COMBINATOR "(" PRINT_HEAD_STEP ")"

typedef enum {
    APPLICATION,
    COMBINATOR_S,
    COMBINATOR_K,
    COMBINATOR_I,
    PRINT,
    COUNT,
    INPUT
} NodeType;

/*
 * One node of the expression graph. An APPLICATION means left applied to right.
 * An INPUT node is the rest of stdin as a list: right is NULL until the
 * character has been read, then right holds it and left holds the next INPUT.
 */
typedef struct Node {
    struct Node *right;
    struct Node *left;
    NodeType type;
    int count;
} Node;

static Node *successor;
static Node *church_zero;

static Node *new_node(NodeType type, Node *left, Node *right)
{
    Node *node = malloc(sizeof(Node));

    node->right = right;
    node->left = left;
    node->type = type;
    node->count = 0;
    return node;
}

static Node *apply(Node *function, Node *argument)
{
    return new_node(APPLICATION, function, argument);
}

/* Reads terms up to the end of the text or an unmatched closing parenthesis */
static Node *parse_terms(const char **text)
{
    Node *result = NULL;

    while (**text != '\0' && **text != ')') {
        char symbol = *(*text)++;
        Node *term;

        switch (symbol) {
        case 'S':
            term = new_node(COMBINATOR_S, NULL, NULL);
            break;
        case 'K':
            term = new_node(COMBINATOR_K, NULL, NULL);
            break;
        case 'I':
            term = new_node(COMBINATOR_I, NULL, NULL);
            break;
        case 'P':
            term = new_node(PRINT, NULL, NULL);
            break;
        case 'C':
            term = new_node(COUNT, NULL, NULL);
            break;
        case '(':
            term = parse_terms(text);
            (*text)++;
            break;
        default:
            continue;
        }
        result = result ? apply(result, term) : term;
    }
    return result;
}

static Node *parse(const char *text)
{
    return parse_terms(&text);
}

static Node *church_numeral(int number)
{
    Node *numeral = church_zero;

    while (number-- > 0)
        numeral = apply(successor, numeral);
    return numeral;
}

/* Fills in an unread INPUT node. End of file reads as the number 256. */
static void read_input(Node *input)
{
    int character = getchar();

    if (character < 0)
        character = 256;
    input->right = church_numeral(character);
    input->left = new_node(INPUT, NULL, NULL);
}

/*
 * Reduces a term by rewriting the graph in place until no rule can be applied,
 * and returns the counter it ended up as.
 *
 * Reaching a combinator means walking down the left side of the applications
 * above it. For the expression ((X a) b) c the nodes passed are
 *   apply3 = (((X a) b) c), apply2 = ((X a) b) and apply1 = (X a)
 * so a, b and c are found at the right of apply1, apply2 and apply3.
 *
 * The root node has no argument, which stops a combinator at the top of the
 * term from reducing, and gives the top of the term a parent to be rewritten.
 */
static int evaluate(Node *term)
{
    Node *root = apply(term, NULL);

    for (;;) {
        Node *node = root;
        Node *apply1 = NULL;
        Node *apply2 = NULL;
        Node *apply3 = NULL;
        Node *last_needed;

        while (node->type == APPLICATION) {
            apply3 = apply2;
            apply2 = apply1;
            apply1 = node;
            node = node->left;
        }

        /* Stop if the combinator has fewer arguments than it needs */
        switch (node->type) {
        case COMBINATOR_S:
            last_needed = apply3;
            break;
        case COMBINATOR_K:
        case PRINT:
            last_needed = apply2;
            break;
        default:
            last_needed = apply1;
            break;
        }
        if (last_needed == NULL || last_needed->right == NULL)
            return root->left->count;

        switch (node->type) {
        case COMBINATOR_S: /* S a b c => (a c) (b c) */
            apply3->left = apply(apply1->right, apply3->right);
            apply3->right = apply(apply2->right, apply3->right);
            break;

        case COMBINATOR_K: /* K a b => a */
            apply3->left = apply1->right;
            break;

        case COMBINATOR_I: /* I a => a */
            apply2->left = apply1->right;
            break;

        case PRINT: { /* P a b => b, after outputting a */
            int character = evaluate(apply1->right);

            if (character > 255)
                exit(character - 256);
            putchar(character);
            fflush(stdout);
            apply3->left = apply2->right;
            break;
        }

        case COUNT: /* C a => the counter a plus one */
            apply1->count = evaluate(apply1->right) + 1;
            apply1->type = COUNT;
            break;

        case INPUT: /* input f => f head tail */
            if (node->right == NULL)
                read_input(node);
            apply1->left = apply(apply1->right, node->right);
            apply1->right = node->left;
            break;

        default:
            break;
        }
    }
}

/* Builds the program by applying the decoder to the banner from its end */
static Node *banner_program(void)
{
    Node *program = parse(BANNER_DECODER);
    int position;

    for (position = sizeof(banner) - 2; position >= 0; position--) {
        switch (banner[position]) {
        case '@':
            program = apply(program, parse("K"));
            break;
        case ' ':
            program = apply(program, parse("I"));
            break;
        case '\n':
            program = apply(program, parse("S"));
            break;
        }
    }
    return apply(program, parse(END_OF_TOKENS));
}

int main()
{
    Node *program;
    Node *input;
    Node *output_list;

    successor = parse(SUCCESSOR);
    church_zero = parse(CHURCH_ZERO);

    /* A Lazy K program maps the list of input bytes to a list of output
     * bytes. This one is wrapped in K so that it ignores its input. */
    program = apply(parse("K"), banner_program());
    input = new_node(INPUT, NULL, NULL);
    output_list = apply(program, input);

    evaluate(apply(parse(PRINT_LIST), output_list));
    return 0;
}
