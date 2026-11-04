/*
 * Look-and-say sequence split into Conway's "elements".
 *
 * The build force-includes <stddef.h>, <stdlib.h> and <stdio.h>, just like
 * the original entry, so no #include lines are needed here.
 */

/*
 * The periodic table.  Only the space at index 1 is ever used: it separates
 * the elements of a line when printing.
 */
const char *periodic_table =

    "H   "                                                              "He  "
    "Li  Be  "                                      "B   C   N   O   F   Ne  "
    "Na  Mg  "                                      "Al  Si  P   S   Cl  Ar  "
    "K   Ca  Sc  Ti  V   Cr  Mn	 Fe  Co  Ni  Cu  Zn  Ga  Ge  As	 Se  Br  Kr  "
    "Rb  Sr  Y 	 Zr  Nb  Mo  Tc  Ru  Rh  Pd  Ag  Cd  In  Sn  Sb  Te  I   Xe  "
    "Cs  Ba* Lu  Hf  Ta	 W   Re  Os  Ir  Pt  Au  Hg  Tl  Pb  Bi  Po  At  Rn  "
    "Fr  Ra**"

        "   *La	 Ce  Pr  Nd  Pm	 Sm  Eu	 Gd  Tb  Dy  Ho	 Er  Tm	 Yb  "
        "  **Ac  Th  Pa  U   Np  Pu  ";

/*
 * A "safe" string buffer: reads outside the buffer return '\0' and writing
 * '\0' outside the buffer is silently ignored.  Writing anything else
 * outside the buffer aborts the program.
 */
typedef struct buffer {
    char *data;
    int length;
} *buffer;

static buffer new_buffer(int length)
{
    buffer b = malloc(sizeof(struct buffer));
    if (b == NULL)
        abort();
    b->length = length;
    b->data = malloc((length ? length : 1) * sizeof(char));
    if (b->data == NULL)
        abort();
    return b;
}

static void free_buffer(buffer b)
{
    free(b->data);
    free(b);
}

static char get_char(buffer b, int index)
{
    return index >= 0 && index < b->length ? b->data[index] : '\0';
}

static void set_char(buffer b, int index, char c)
{
    if (index >= 0 && index < b->length)
        b->data[index] = c;
    else if (c != '\0')
        abort();
}

/* length of the string held in the buffer */
static int string_length(buffer b)
{
    int i;
    for (i = 0; get_char(b, i); i++)
        ;
    return i;
}

static void print_buffer(buffer b)
{
    int i;
    for (i = 0; get_char(b, i); i++)
        putchar(get_char(b, i));
}

/* compare the first n characters of a and b, 0 when they are equal */
static int compare(buffer a, buffer b, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (get_char(a, i) != get_char(b, i))
            return get_char(a, i) - get_char(b, i);
    return 0;
}

/* copy of the first n characters of src */
static buffer prefix(buffer src, int n)
{
    buffer dst = new_buffer(n);
    int i;
    for (i = 0; i < n; i++)
        set_char(dst, i, get_char(src, i));
    return dst;
}

/* copy of src starting at index start */
static buffer suffix(buffer src, int start)
{
    buffer dst = new_buffer(string_length(src) - start);
    int i;
    for (i = start; get_char(src, i); i++)
        set_char(dst, i - start, get_char(src, i));
    return dst;
}

/*
 * A NULL terminated list of elements (buffers), with the same "safe" rules:
 * reads outside the list return NULL.
 */
typedef struct element_list {
    buffer *elements;
    int length;
} *element_list;

static element_list new_list(int length)
{
    element_list list = malloc(sizeof(struct element_list));
    if (list == NULL)
        abort();
    list->length = length;
    list->elements = malloc((length ? length : 1) * sizeof(buffer));
    if (list->elements == NULL)
        abort();
    return list;
}

static void free_list(element_list list)
{
    free(list->elements);
    free(list);
}

static buffer get_element(element_list list, int index)
{
    return index >= 0 && index < list->length ? list->elements[index] : NULL;
}

static void set_element(element_list list, int index, buffer b)
{
    if (index >= 0 && index < list->length)
        list->elements[index] = b;
    else if (b != NULL)
        abort();
}

static void print_generation(element_list list, int generation)
{
    int i;
    printf("%d:", generation);
    for (i = 0; get_element(list, i); i++) {
        putchar(' ');
        print_buffer(get_element(list, i));
    }
    putchar('\n');
}

/* the look-and-say description of src ("1211" -> "111221") */
static buffer look_and_say(buffer src)
{
    buffer dst = new_buffer(2 * string_length(src));
    char c;
    char run; /* a char, so runs of 10 or more print oddly and 257+ overflow */
    int i, out = 0;

    for (i = 0; (c = get_char(src, i)); i++) {
        for (run = 1; c == get_char(src, i + run); run++)
            ;
        i += run - 1;
        set_char(dst, out++, run + '0');
        set_char(dst, out++, c);
    }
    set_char(dst, out, '\0');
    return dst;
}

/*
 * Look-and-say of src without its last run, because that run could still
 * change depending on whatever follows src.  src is freed.
 */
static buffer look_and_say_settled(buffer src)
{
    buffer dst = look_and_say(src);
    free_buffer(src);
    set_char(dst, string_length(dst) - 2, '\0');
    return dst;
}

/*
 * Decide whether a string ending in the digit left can be split from the
 * string right, i.e. whether the two halves will never interact again.
 *
 * Every prefix of right is evolved; if its first digit ever equals left the
 * halves merge, so no split.  If the evolving prefix enters a cycle (found
 * with a fast and a slow copy, Floyd style) the halves stay apart forever.
 * When no prefix decides, try again one generation later.
 */
static int can_split(char left, buffer right)
{
    buffer fast, slow, next;
    int length, step, result;

    if (left == get_char(right, 0))
        return 0;

    for (length = 0; get_char(right, length); length++) {
        fast = prefix(right, length);
        slow = prefix(right, length);

        for (step = 0; get_char(fast, 0); step++) {
            fast = look_and_say_settled(fast);
            if (left == get_char(fast, 0)) {
                free_buffer(fast);
                free_buffer(slow);
                return 0;
            }

            if (step % 2)
                slow = look_and_say_settled(slow);

            if (compare(fast, slow, string_length(slow)) == 0) {
                free_buffer(fast);
                free_buffer(slow);
                return 1;
            }
        }

        free_buffer(fast);
        free_buffer(slow);
    }

    next = look_and_say(right);
    result = can_split(left, next);
    free_buffer(next);
    return result;
}

/* split every element of the list wherever it can be split */
static element_list split_elements(element_list old)
{
    element_list list;
    buffer element, rest;
    int i, pos, count;

    count = 0;
    for (i = 0; (element = get_element(old, i)); i++)
        count += string_length(element);
    list = new_list(count);

    count = 0;
    for (i = 0; (element = get_element(old, i)); i++) {
        set_element(list, count++, element);

        for (pos = 0; get_char(element, pos + 1); pos++) {
            rest = suffix(element, pos + 1);
            if (can_split(get_char(element, pos), rest)) {
                /* cut element after pos and carry on with the rest */
                set_char(element, pos + 1, '\0');
                set_element(list, count++, element = rest);
                pos = 0; /* the loop increment makes this 1 */
            } else {
                free_buffer(rest);
            }
        }
    }

    free_list(old);
    set_element(list, count, NULL);
    return list;
}

int main(int argc, char **argv)
{
    element_list list;
    buffer element;
    int i, j;
    int count = argc - 1;

    list = new_list(count);
    for (i = 0; i < count; i++) {
        for (j = 0; argv[i + 1][j]; j++)
            ;
        set_element(list, i, element = new_buffer(j));
        for (j = 0; argv[i + 1][j]; j++)
            set_char(element, j, argv[i + 1][j]);
    }

    for (i = 0; get_element(list, 0); i++) {
        list = split_elements(list);
        print_generation(list, i);

        for (j = 0; (element = get_element(list, j)); j++) {
            set_element(list, j, look_and_say(element));
            free_buffer(element);
        }
    }

    return 0;
}
