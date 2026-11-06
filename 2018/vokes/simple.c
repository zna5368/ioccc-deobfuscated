#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Reads a directed graph (one line per node: "id edge edge ...") and prints
 * its strongly connected components in reverse topological order using
 * Tarjan's algorithm.  The members of each component are sorted with a
 * counting sort before printing.
 */

typedef unsigned long word;

#define UNVISITED (~0UL)
#define LINE_SIZE 0x3543        /* longer lines are split by fgets */

struct node {
    word index;                 /* Tarjan visit order, UNVISITED if not yet */
    word lowlink;               /* smallest index reachable from this node */
    word on_stack;              /* nonzero while the node is on the stack */
    word edge_count;
    word edges[];               /* ids of nodes this node depends on */
};

/* graph, indexed by node id */
static struct node **nodes;
static word node_capacity;
static word max_id;

/* Tarjan state */
static word next_index;
static word *stack;
static word stack_size, stack_capacity;

/* the component currently being printed */
static word *component, *sorted, *bucket;
static word component_size, component_capacity;
static word component_number;

static char line[LINE_SIZE];

static void *xrealloc(void *ptr, size_t size)
{
    void *result = realloc(ptr, size);
    if (!result || !size)
        exit(1);
    return result;
}

static struct node *init_node(struct node *v)
{
    v->edge_count = 0;
    v->index = v->lowlink = UNVISITED;
    return v;
}

/* make the node table large enough to hold max_id */
static void grow_node_table(void)
{
    word old_capacity = node_capacity, k;

    if (!node_capacity)
        node_capacity = 1;
    while (node_capacity <= max_id) {
        node_capacity *= 2;
        if (!node_capacity) {
            puts("Out of range.");
            exit(1);
        }
    }
    nodes = xrealloc(nodes, node_capacity * sizeof *nodes);
    for (k = old_capacity; k < node_capacity; k++)
        nodes[k] = NULL;
}

/* append edges id -> targets[0..count-1], creating any missing nodes */
static void add_edges(word id, word count, word *targets)
{
    struct node *v = nodes[id];
    word old_count = v ? v->edge_count : 0, k;

    v = xrealloc(v, sizeof *v + (old_count + count) * sizeof(word));
    if (!nodes[id])
        init_node(v);
    nodes[id] = v;

    for (k = 0; k < count; k++) {
        word target = targets[k];
        v->edges[v->edge_count++] = target;
        if (!nodes[target])
            nodes[target] = init_node(xrealloc(NULL, sizeof(struct node)));
    }
}

/* sort component[] into sorted[], all values are below range */
static void counting_sort(word range)
{
    word k;

    bucket = xrealloc(bucket, range * sizeof *bucket);
    for (k = 0; k < range; k++)
        bucket[k] = 0;
    for (k = 0; k < component_size; k++)
        bucket[component[k]]++;
    for (k = 0; k + 1 < range; k++)
        bucket[k + 1] += bucket[k];
    for (k = component_size; k-- > 0;)
        sorted[--bucket[component[k]]] = component[k];
}

/* pop the stack down to root, those nodes form one component */
static void emit_component(word root)
{
    word id, largest = 0, k;

    component_size = 0;
    do {
        id = stack[--stack_size];
        nodes[id]->on_stack = 0;
        if (component_size >= component_capacity) {
            component_capacity = 2 * (component_capacity ? component_capacity : 1);
            component = xrealloc(component, component_capacity * sizeof *component);
            sorted = xrealloc(sorted, component_capacity * sizeof *sorted);
        }
        component[component_size++] = id;
        if (id > largest)
            largest = id;
    } while (id != root);

    counting_sort(largest + 1);

    printf("%lu:", component_number++);
    for (k = 0; k < component_size; k++)
        printf(" %lu", sorted[k]);
    puts("");
}

static void strongconnect(word id)
{
    struct node *v = nodes[id];
    word k;

    if (v->index != UNVISITED)
        return;

    v->index = v->lowlink = next_index++;
    v->on_stack = 1;
    stack[stack_size++] = id;
    if (stack_size == stack_capacity) {
        stack_capacity *= 2;
        stack = xrealloc(stack, stack_capacity * sizeof *stack);
    }

    for (k = 0; k < v->edge_count; k++) {
        word target = v->edges[k];
        struct node *w = nodes[target];
        if (w->index == UNVISITED) {
            strongconnect(target);
            if (v->lowlink > w->lowlink)
                v->lowlink = w->lowlink;
        } else if (w->on_stack) {
            if (v->lowlink > w->index)
                v->lowlink = w->index;
        }
    }

    if (v->lowlink == v->index)
        emit_component(id);
}

static int is_digit(char ch)
{
    return (word)ch - '0' <= 9;
}

int main(int argc, char **argv)
{
    word numbers_capacity = 1, count, id;
    word *numbers = xrealloc(NULL, numbers_capacity * sizeof *numbers);
    char *token;

    (void)argc;
    (void)argv;

    while (fgets(line, sizeof line, stdin)) {
        /* read the leading run of numbers on the line */
        count = 0;
        token = strtok(line, "\t ");
        while (token && is_digit(*token)) {
            numbers[count] = (word)atoi(token);
            if (numbers[count] > max_id)
                max_id = numbers[count];
            count++;
            if (count == numbers_capacity) {
                numbers_capacity *= 2;
                numbers = xrealloc(numbers, numbers_capacity * sizeof *numbers);
            }
            token = strtok(NULL, " \t");
        }

        if (max_id >= node_capacity)
            grow_node_table();
        if (count > 0)
            add_edges(numbers[0], count - 1, &numbers[1]);
    }

    /* input was read but no node was ever defined */
    if (node_capacity && !nodes[max_id])
        return 1;

    if (nodes) {
        stack_capacity = 2;
        stack = xrealloc(NULL, stack_capacity * sizeof *stack);
        for (id = 0; id < node_capacity; id++)
            if (nodes[id])
                strongconnect(id);
    }
    return 0;
}
