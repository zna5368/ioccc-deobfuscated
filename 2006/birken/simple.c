#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * EDAMAME: a relay circuit simulator that reads a TOFU netlist on stdin.
 *
 * The netlist is a stream of whitespace separated tokens:
 *
 *   def NAME param... :  body...      define a component
 *   r coil common on off              the only primitive: a relay
 *   run STEPS HALF_PERIOD NAME arg... build NAME and simulate it
 *
 * Special wire names: 0 (a fresh unconnected wire), 1 (always high),
 * c (the clock), k (the inverted clock).  In the run line any other
 * argument is an output probe whose values are printed.
 *
 * (The original source hides a second program in its output format
 * string, after the terminating NUL:
 *   main(){int i=0;for(;i<40;i++)printf("%d ",i*i+i+41);}  )
 */

#define LINE_SIZE 314

struct token {
    char *text;
    struct token *next;
};

struct relay;

struct relay_link {
    struct relay *relay;
    struct relay_link *next;
};

struct wire {
    int value;
    struct relay_link *relays; /* relays touching this wire */
    struct wire *next_all;
};

struct relay {
    int state; /* 1 = coil energized, common connected to "on" */
    struct wire *coil, *common, *on, *off;
    struct relay *next_all;
};

struct wire_list {
    struct wire *wire;
    struct wire_list *next;
};

struct binding {
    char *name;
    struct wire *wire;
    struct binding *next;
};

static struct token *tokens;           /* every token of the input */
static struct token *relay_definition; /* parameter list of primitive "r" */
static struct wire *all_wires;
static struct relay *all_relays;
static struct wire_list *outputs, **outputs_tail = &outputs;
static struct wire high, clock_wire, inverse_clock;

static struct wire *new_wire(void)
{
    struct wire *w = calloc(1, sizeof *w);
    w->next_all = all_wires;
    all_wires = w;
    return w;
}

static void attach(struct wire *w, struct relay *r)
{
    struct relay_link *link = malloc(sizeof *link);
    link->relay = r;
    link->next = w->relays;
    w->relays = link;
}

static int same(const char *a, const char *b)
{
    return (a && b) ? !strcmp(a, b) : 0;
}

static struct token *find_token(struct token *t, const char *text)
{
    while (t) {
        if (same(text, t->text))
            return t;
        t = t->next;
    }
    return 0;
}

/* Return the parameter list of the component definition called name. */
static struct token *find_definition(char *name)
{
    struct token *t = tokens;
    if (same("r", name))
        return relay_definition;
    for (;;) {
        t = find_token(t, "def");
        t = t->next;
        if (same(name, t->text))
            return t->next;
    }
}

static void add_arg(struct wire_list ***tail, struct wire *w)
{
    struct wire_list *a = malloc(sizeof *a);
    a->wire = w;
    a->next = 0;
    **tail = a;
    *tail = &a->next;
}

/* Instantiate a component definition, connecting its parameters to args. */
static void instantiate(struct token *params, struct wire_list *args)
{
    if (params == relay_definition) {
        struct relay *r = calloc(1, sizeof *r);
        r->next_all = all_relays;
        all_relays = r;
        r->coil = args->wire;
        r->common = args->next->wire;
        r->on = args->next->next->wire;
        r->off = args->next->next->next->wire;
        for (; args; args = args->next)
            attach(args->wire, r);
        return;
    }

    /* bind each formal parameter name to the wire passed in */
    struct binding *scope = 0, *b;
    struct token *body = params;
    while (body->text[0] != ':') {
        b = malloc(sizeof *b);
        b->name = body->text;
        b->wire = args->wire;
        b->next = scope;
        scope = b;
        args = args->next;
        body = body->next;
    }
    body = body->next;

    /* each body statement: COMPONENT arg... */
    while (body && !same("run", body->text) && !same("def", body->text)) {
        struct token *def = find_definition(body->text), *sub;
        struct wire_list *sub_args = 0, **tail = &sub_args;
        body = body->next;
        for (sub = def; sub->text[0] != ':'; sub = sub->next, body = body->next) {
            char *name = body->text;
            struct wire *w = same("0", name)   ? new_wire()
                             : same(name, "1") ? &high
                             : same("c", name) ? &clock_wire
                             : same(name, "k") ? &inverse_clock
                                               : 0;
            if (!w) {
                for (b = scope; b && !same(name, b->name); b = b->next)
                    ;
                if (b) {
                    w = b->wire;
                } else {
                    /* a new internal wire local to this instance */
                    w = new_wire();
                    b = malloc(sizeof *b);
                    b->name = name;
                    b->wire = w;
                    b->next = scope;
                    scope = b;
                }
            }
            add_arg(&tail, w);
        }
        instantiate(def, sub_args);
    }
}

/* Spread a high level through every closed relay contact. */
static void propagate(struct wire *w)
{
    struct relay_link *link;
    for (link = w->relays; link; link = link->next) {
        struct relay *r = link->relay;
        struct wire *common = r->common;
        struct wire *contact = r->state ? r->on : r->off;
        if (contact->value < common->value) {
            contact->value++;
            propagate(contact);
        } else if (common->value < contact->value) {
            common->value++;
            propagate(common);
        }
    }
}

static char *copy(const char *s)
{
    char *d = malloc(1 + strlen(s) * sizeof(char));
    return strcpy(d, s);
}

int main()
{
    char line[LINE_SIZE], previous[LINE_SIZE], current[LINE_SIZE];
    char *p, *word;
    struct token **tail = &tokens, *t, *run = 0;
    struct wire_list *args = 0, **args_tail = &args, *o;
    struct wire *w;
    struct relay *r;
    int i, steps, half_period, countdown, clock_level;
    int relay_count = 0, wire_count = 0;
    static const char *relay_params[] = {"r", "r", "r", "r", ":"};

    /* the primitive relay has four parameters */
    for (i = 4; i >= 0; i--) {
        t = calloc(1, sizeof *t);
        t->text = copy(relay_params[i]);
        t->next = relay_definition;
        relay_definition = t;
    }

    *previous = 0;
    clock_level = 1;
    high.value = 1;

    /* read and tokenize the netlist */
    while (gets(line)) {
        for (p = line; *p; p++)
            if (*p >= 'A' && *p <= 'Z')
                *p += 'a' - 'A';
        p = strchr(line, '#');
        if (p)
            *p = 0;
        for (word = strtok(line, " "); word; word = strtok(0, " ")) {
            t = calloc(1, sizeof *t);
            t->text = copy(word);
            *tail = t;
            tail = &t->next;
            if (same("run", t->text))
                run = t;
        }
    }

    /* run STEPS HALF_PERIOD NAME args... */
    t = run->next;
    steps = atoi(t->text);
    t = t->next;
    half_period = atoi(t->text);
    countdown = half_period;
    t = t->next;
    struct token *top = find_definition(t->text);
    t = t->next;
    for (; t && !same("def", t->text); t = t->next) {
        switch (t->text[0]) {
        case '0': w = new_wire(); break;
        case '1': w = &high; break;
        case 'c': w = &clock_wire; break;
        case 'k': w = &inverse_clock; break;
        default:
            w = new_wire();
            add_arg(&outputs_tail, w);
        }
        add_arg(&args_tail, w);
    }
    instantiate(top, args);

    for (r = all_relays; r; r = r->next_all)
        relay_count++;
    for (w = all_wires; w; w = w->next_all)
        wire_count++;
    printf("Build complete: %d components, %d nodes.\n", relay_count,
           wire_count);

    for (i = 1; steps > i; i++) {
        if (!--countdown) {
            countdown = half_period;
            clock_level = 1 - clock_level;
        }
        inverse_clock.value = !(clock_wire.value = clock_level);

        for (w = all_wires; w; w = w->next_all)
            w->value = 0;
        propagate(&high);
        if (clock_wire.value)
            propagate(&clock_wire);
        else
            propagate(&inverse_clock);

        /* relays switch according to their coil after settling */
        for (r = all_relays; r; r = r->next_all)
            r->state = r->coil->value;

        p = current;
        for (o = outputs; o; o = o->next)
            *p++ = '0' + o->wire->value;
        *p = 0;
        if (!same(current, previous)) {
            strcpy(previous, current);
            printf("%d: %s\n", i, current);
        }
    }
    return 0;
}
