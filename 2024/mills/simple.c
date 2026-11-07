/*
 * A Z-machine version 3 interpreter for Infocom style text adventures.
 *
 *     ./simple story.z3 [game.sav]
 *
 * The obfuscated original is a tiny stack machine whose byte code, hidden
 * in string literals, implements the Z-machine.  This file is that byte
 * code translated back into ordinary C functions.
 */

#define STORY_SIZE  131072      /* bytes of Z-machine memory (128K)     */
#define STACK_BYTES 16128       /* bytes reserved for the game's stack  */
#define LINE_WIDTH  78          /* output is word wrapped at this column */

/* story file header fields */
#define HDR_VERSION        0
#define HDR_FLAGS1         1
#define HDR_START_PC       6
#define HDR_DICTIONARY     8
#define HDR_OBJECTS       10
#define HDR_GLOBALS       12
#define HDR_STATIC_MEMORY 14
#define HDR_FLAGS2        16
#define HDR_ABBREVIATIONS 24

/* byte offsets inside a 9 byte object table entry */
#define OBJ_PARENT     4
#define OBJ_SIBLING    5
#define OBJ_CHILD      6
#define OBJ_PROPERTIES 7

char **args;                    /* args[1] is the story, args[2] the save file */

unsigned char mem[STORY_SIZE + STACK_BYTES];
unsigned char *pc;              /* Z-machine program counter                */
unsigned short *stack;          /* bottom of the game stack (after memory)  */
unsigned short *sp;             /* top of the game stack                    */
unsigned short *fp;             /* frame of the current routine, fp[1..15]
                                   are its local variables                   */

unsigned short operand[4];      /* operands of the current instruction      */
int operand_count;

unsigned char output_line[128]; /* text waiting to be written to stdout     */
int output_length;
unsigned char column;           /* cursor column used for word wrapping     */
int in_status_line;             /* status line text is not word wrapped     */

/* alphabet A2 (zchars 7..31), 7 is a newline */
unsigned char alphabet2[] = "\r0123456789.,!?_#'\"/\\-:()";

void print_char(long c);

long read_word(long address)
{
    return mem[address] << 8 | mem[address + 1];
}

void write_word(long address, long value)
{
    mem[address + 1] = value;
    mem[address] = value >> 8;
}

long fetch_word(void)
{
    long word = read_word(pc - mem);
    pc += 2;
    return word;
}

/*
 * Variables: 0 is the top of the stack, 1-15 are locals of the current
 * routine and 16-255 are globals.
 */
long get_var(long var)
{
    if (var == 0)
        return *sp--;
    if (var < 16)
        return fp[var];
    return read_word(read_word(HDR_GLOBALS) + 2 * (var - 16));
}

void set_var(long var, long value)
{
    if (var == 0)
        *++sp = value;
    else if (var < 16)
        fp[var] = value;
    else
        write_word(read_word(HDR_GLOBALS) + 2 * (var - 16), value);
}

/* store the result of an instruction in the variable named by the next byte */
void store_result(long value)
{
    set_var(*pc++, value);
}

/*
 * A routine call saves the return address and the caller's frame on the
 * stack.  The new frame pointer points at that saved data.
 */
void push_context(void)
{
    long offset = pc - mem;

    *++sp = offset >> 16;
    *++sp = offset;
    *++sp = fp - stack;
    fp = sp;
}

void pop_context(void)
{
    long low, high;

    sp = fp;
    fp = stack + *sp--;
    low = *sp--;
    high = *sp--;
    pc = mem + (high << 16) + low;
}

void return_from_routine(long value)
{
    pop_context();
    store_result(value);
}

/*
 * Conditional branch.  The branch data tells whether to branch when the
 * condition is true or false, and the offset to jump by.  Offsets 0 and 1
 * mean return false or true from the current routine instead.
 */
void branch(long condition)
{
    long data = *pc++, offset;
    int on_true = data >> 7;

    data &= 127;
    if (data & 64)
        offset = data - 64;
    else
        offset = data << 8 | *pc++;

    if ((condition != 0) != on_true)
        return;

    if (offset < 2) {
        return_from_routine(offset);
        return;
    }
    if (offset >> 13)
        offset -= 1 << 14;
    pc += offset - 2;
}

/* ---------------------------------------------------------------- objects */

long object_address(long object)
{
    /* 31 default property words come first, objects are numbered from 1 */
    return object ? read_word(HDR_OBJECTS) + 62 + 9 * ((short)object - 1) : 0;
}

/* first property of an object with a number less than or equal to prop */
long find_property(long object, long prop)
{
    long p = read_word(object_address(object) + OBJ_PROPERTIES);

    p += 1 + 2 * mem[p];        /* skip the object's short name */
    while ((short)prop < (mem[p] & 31))
        p += (mem[p] >> 5) + 2;
    return p;
}

long attribute_address(long object, long attribute)
{
    return object_address(object) + attribute / 8;
}

long attribute_mask(long attribute)
{
    return 1 << (7 - (attribute & 7));
}

void remove_object(long object)
{
    long o = object_address(object);
    long parent = mem[o + OBJ_PARENT];
    long link;

    if (parent) {
        /* find the byte that points at the object: the parent's child
           or the sibling of the object before it */
        link = object_address(parent) + OBJ_CHILD;
        while (mem[link] != object)
            link = object_address(mem[link]) + OBJ_SIBLING;
        mem[link] = mem[o + OBJ_SIBLING];
    }
    mem[o + OBJ_PARENT] = 0;
    mem[o + OBJ_SIBLING] = 0;
}

void insert_object(long object, long destination)
{
    long child = object_address(destination) + OBJ_CHILD;
    long o;

    remove_object(object);
    o = object_address(object);
    mem[o + OBJ_PARENT] = destination;
    mem[o + OBJ_SIBLING] = mem[child];
    mem[child] = object;
}

/* store the parent, sibling or child of an object */
long object_field(long object, int field)
{
    long value = object ? mem[object_address(object) + field] : 0;

    store_result(value);
    return value;
}

/* ------------------------------------------------------------------- text */

void flush_output(void)
{
    write(1, output_line, output_length);
    output_length = 0;
}

void buffer_char(long c)
{
    output_line[output_length++] = c;
}

/*
 * Words are collected in a buffer that is written out whenever a space or
 * newline arrives.  When a word passes the right margin, the space in front
 * of it is turned into a newline.
 */
void print_char(long c)
{
    int new_column;

    if (in_status_line) {
        buffer_char(c);
        return;
    }
    if (c == 13) {
        buffer_char('\n');
        flush_output();
        column = 0;
        return;
    }
    if (c == ' ')
        flush_output();
    buffer_char(c);

    new_column = column + 1;
    column = new_column;
    if (new_column > LINE_WIDTH) {
        if (output_line[0] == ' ') {
            output_line[0] = '\n';
            column = output_length - 1;
        }
        flush_output();
    }
}

void print_number(long n)
{
    long tens;

    if ((short)n < 0) {
        print_char('-');
        n = -n;
    }
    tens = (short)n / 10;
    if (tens)
        print_number(tens);
    print_char(n - tens * 10 + '0');
}

long print_zstring(long address);

/*
 * Decode one 5 bit zchar.  The state says how to read it:
 *   0, 4, 5  alphabet A0 (lower case), A1 (upper case) or A2 (punctuation)
 *   1, 2, 3  this zchar picks one of the 96 abbreviations
 *   6        first half of a 10 bit character code
 *   7+       second half, the first half is in the high bits of the state
 * The new state is returned.
 */
int decode_zchar(int state, int z)
{
    long entry;

    if (state > 6) {
        print_char((state >> 3) << 5 | z);
        return 0;
    }
    if (state == 6)
        return z * 8 + 7;
    if (state >= 1 && state <= 3) {
        entry = read_word(HDR_ABBREVIATIONS) + 64 * (state - 1) + 2 * z;
        print_zstring(2 * read_word(entry));
        return 0;
    }
    if (z == 0) {
        print_char(' ');
        return 0;
    }
    if (z < 6)
        return z;               /* abbreviation or shift */
    if (state == 5) {
        if (z == 6)
            return 6;           /* 10 bit character follows */
        print_char(alphabet2[z - 7]);
        return 0;
    }
    print_char(z + (state ? 'A' : 'a') - 6);
    return 0;
}

/* print the packed text at an address and return the address after it */
long print_zstring(long address)
{
    unsigned char *p = mem + address;
    long word;
    int state = 0;

    do {
        word = p[0] << 8 | p[1];
        p += 2;
        state = decode_zchar(state, word >> 10 & 31);
        state = decode_zchar(state, word >> 5 & 31);
        state = decode_zchar(state, word & 31);
    } while (!(word >> 15));
    return p - mem;
}

void print_object_name(long object)
{
    print_zstring(read_word(object_address(object) + OBJ_PROPERTIES) + 1);
}

void print_ascii(char *s)
{
    while (*s)
        print_char(*s++);
}

/*
 * Status line in reverse video at the top of the screen: the current
 * location then the score and moves, or the time for timed games.
 */
void show_status(void)
{
    long minutes, tens;

    flush_output();
    in_status_line = 1;
    print_ascii("\0337\033[H\033[K\033[7m ");    /* save cursor, home, clear */
    print_object_name(get_var(16));
    print_ascii(" \033[;72H\033[0;1m");
    print_number(get_var(17));
    if (mem[HDR_FLAGS1] & 2) {
        print_char(':');
        minutes = get_var(18);
        tens = (short)minutes / 10;
        print_number(tens);
        print_number(minutes - tens * 10);
    } else {
        print_char('/');
        print_number(get_var(18));
    }
    print_ascii("\0338");                        /* restore cursor */
    in_status_line = 0;
    flush_output();
}

/* ------------------------------------------------------------------ input */

int is_separator(long c)
{
    long p = read_word(HDR_DICTIONARY);
    int n = mem[p];

    while (n--)
        if (mem[++p] == c)
            return 1;
    return 0;
}

/*
 * Encode a word as six zchars (packed into 4 bytes) and binary search the
 * dictionary for it.  Returns the address of the entry or 0.
 */
long lookup_word(long text, long entry)
{
    unsigned char *p = mem + text + mem[entry + 1];
    unsigned char *end = p + mem[entry];
    int z[9], n = 0, i;
    long key, c, dictionary, entries, low, high, middle, address, found;

    while (n < 6) {
        if (p == end) {
            z[n++] = 5;                 /* pad */
            continue;
        }
        c = *p++;
        if (c > 96 && c < 123) {
            z[n++] = c - 91;            /* lower case letter */
            continue;
        }
        z[n++] = 5;                     /* shift to alphabet A2 */
        for (i = 0; alphabet2[i] && alphabet2[i] != c; i++)
            ;
        if (alphabet2[i]) {
            z[n++] = i + 7;
        } else {
            z[n++] = 6;                 /* 10 bit character code */
            z[n++] = c & 31;
        }
    }
    key = (long)(z[0] << 10 | z[1] << 5 | z[2]) << 16 |
          0x8000 | z[3] << 10 | z[4] << 5 | z[5];

    dictionary = read_word(HDR_DICTIONARY);
    entries = dictionary + mem[dictionary] + 1;
    low = 0;
    high = read_word(entries + 1);
    while (low < high) {
        middle = (low + high) >> 1;
        address = entries + 3 + middle * mem[entries];
        found = (long)mem[address] << 24 | mem[address + 1] << 16 |
                mem[address + 2] << 8 | mem[address + 3];
        if (key == found)
            return address;
        if (key < found)
            high = middle;
        else
            low = middle + 1;
    }
    return 0;
}

/* record a word in the parse buffer, end points after its delimiter */
void add_word(long text, long parse, long length, unsigned char *end)
{
    long count = mem[parse + 1] + 1;
    long entry = parse + 4 * count;

    mem[parse + 1] = count;
    mem[entry] = length;
    mem[entry + 1] = (end - mem - 1) - (text + length);
}

/*
 * Split the text into words.  Spaces separate words, while the dictionary
 * separators (like commas) are separators and words of their own.
 */
void tokenise(long text, long parse)
{
    unsigned char *p = mem + text + 1;
    long length = 0, c, entry;

    mem[parse + 1] = 0;
    if (mem[parse]) {
        c = *p++;
        for (;;) {
            if (c && c != ' ' && !is_separator(c)) {
                length++;
                c = *p++;
            } else if (length) {
                add_word(text, parse, length, p);
                length = 0;
            } else if (!c) {
                break;
            } else if (c == ' ') {
                c = *p++;
            } else {
                c = *p++;
                add_word(text, parse, 1, p);
            }
        }
    }
    for (entry = parse + 4 * mem[parse + 1]; entry != parse; entry -= 4)
        write_word(entry - 2, lookup_word(text, entry));
}

/* read a line of input into the text buffer and parse it */
void read_line(long text, long parse)
{
    unsigned char *p = mem + text + 1;
    long last = text;

    show_status();
    column = 0;
    while (p - mem != text + mem[text]) {
        if (read(0, p, 1) < 1)
            exit(0);
        last = p - mem;
        if (*p++ == '\n')
            break;
        if (mem[last] > 64 && mem[last] < 91)
            mem[last] += 32;            /* lower case */
    }
    mem[last] = 0;
    tokenise(text, parse);
}

/* ------------------------------------------------------------ save files */

long save_game(void)
{
    int fd;

    push_context();
    fd = creat(args[2], 0600);
    if (fd != -1) {
        write(fd, mem, read_word(HDR_STATIC_MEMORY));
        write(fd, stack, (fp - stack + 1) * 2);
        close(fd);
    }
    pop_context();
    return fd != -1;
}

/* a successful restore continues after the save instruction */
long restore_game(void)
{
    int flags2 = mem[HDR_FLAGS2];
    int fd = open(args[2], 0);

    if (fd != -1) {
        read(fd, mem, read_word(HDR_STATIC_MEMORY));
        fp = stack + (read(fd, stack, STORY_SIZE) >> 1) - 1;
        close(fd);
        pop_context();
        mem[HDR_FLAGS2] = flags2;
    }
    return fd != -1;
}

void restart_game(void)
{
    int flags2 = mem[HDR_FLAGS2];
    int fd = open(args[1], 0), i;

    if (fd == -1)
        exit(0);
    sp = fp = stack;
    for (i = 0; i < 64; i++)            /* clear the screen */
        write(1, "\n", 1);
    read(fd, mem, STORY_SIZE);
    close(fd);
    if (mem[HDR_VERSION] != 3)
        exit(0);
    mem[HDR_FLAGS2] = flags2;
    pc = mem + read_word(HDR_START_PC);
}

/* ----------------------------------------------------------- instructions */

void call_routine(void)
{
    long routine = operand[0];
    int locals, i;

    if (!routine) {
        store_result(0);
        return;
    }
    push_context();
    pc = mem + 2 * routine;
    locals = *pc++;
    for (i = 1; i <= locals; i++) {
        *++sp = fetch_word();           /* default value */
        if (i < operand_count)
            fp[i] = operand[i];         /* argument */
    }
}

void two_operand(int opcode)
{
    long a = operand[0], b = operand[1], value, p;
    int i;

    switch (opcode & 31) {
    case 0:
        break;
    case 1:                                             /* je */
        for (i = operand_count - 1; i; i--)
            if (operand[i] == a)
                break;
        branch(i);
        break;
    case 2:                                             /* jl */
        branch((short)a < (short)b);
        break;
    case 3:                                             /* jg */
        branch((short)a > (short)b);
        break;
    case 4:                                             /* dec_chk */
        value = get_var(a) - 1;
        set_var(a, value);
        branch((short)value < (short)b);
        break;
    case 5:                                             /* inc_chk */
        value = get_var(a) + 1;
        set_var(a, value);
        branch((short)value > (short)b);
        break;
    case 6:                                             /* jin */
        branch((a ? mem[object_address(a) + OBJ_PARENT] : 0) == b);
        break;
    case 7:                                             /* test */
        branch((a & b) == b);
        break;
    case 8:                                             /* or */
        store_result(a | b);
        break;
    case 9:                                             /* and */
        store_result(a & b);
        break;
    case 10:                                            /* test_attr */
        branch(a ? mem[attribute_address(a, b)] & attribute_mask(b) : 0);
        break;
    case 11:                                            /* set_attr */
        if (a)
            mem[attribute_address(a, b)] |= attribute_mask(b);
        break;
    case 12:                                            /* clear_attr */
        if (a)
            mem[attribute_address(a, b)] &= ~attribute_mask(b);
        break;
    case 13:                                            /* store */
        if (a)
            set_var(a, b);
        else
            *sp = b;
        break;
    case 14:                                            /* insert_obj */
        if (a && b)
            insert_object(a, b);
        break;
    case 15:                                            /* loadw */
        store_result(read_word(a + 2 * b));
        break;
    case 16:                                            /* loadb */
        store_result(mem[a + b]);
        break;
    case 17:                                            /* get_prop */
        value = 0;
        if (a) {
            p = find_property(a, b);
            if ((mem[p] & 31) == b)
                value = mem[p] >> 5 ? read_word(p + 1) : mem[p + 1];
            else
                value = read_word(read_word(HDR_OBJECTS) + 2 * (b - 1));
        }
        store_result(value);
        break;
    case 18:                                            /* get_prop_addr */
        value = 0;
        if (a) {
            p = find_property(a, b);
            if ((mem[p] & 31) == b)
                value = p + 1;
        }
        store_result(value);
        break;
    case 19:                                            /* get_next_prop */
        store_result(mem[find_property(a, b)] & 31);
        break;
    case 20:                                            /* add */
        store_result(a + b);
        break;
    case 21:                                            /* sub */
        store_result(a - b);
        break;
    case 22:                                            /* mul */
        store_result((short)a * (short)b);
        break;
    case 23:                                            /* div */
        store_result((short)a / (short)b);
        break;
    case 24:                                            /* mod */
        store_result((short)a % (short)b);
        break;
    default:
        exit(0);
    }
}

void one_operand(int opcode)
{
    long a = operand[0];

    switch (opcode & 15) {
    case 0:                                             /* jz */
        branch(!a);
        break;
    case 1:                                             /* get_sibling */
        branch(object_field(a, OBJ_SIBLING));
        break;
    case 2:                                             /* get_child */
        branch(object_field(a, OBJ_CHILD));
        break;
    case 3:                                             /* get_parent */
        object_field(a, OBJ_PARENT);
        break;
    case 4:                                             /* get_prop_len */
        store_result(a ? (mem[a - 1] >> 5) + 1 : 0);
        break;
    case 5:                                             /* inc */
        set_var(a, get_var(a) + 1);
        break;
    case 6:                                             /* dec */
        set_var(a, get_var(a) - 1);
        break;
    case 7:                                             /* print_addr */
        print_zstring(a);
        break;
    case 8:
        break;
    case 9:                                             /* remove_obj */
        if (a)
            remove_object(a);
        break;
    case 10:                                            /* print_obj */
        print_object_name(a);
        break;
    case 11:                                            /* ret */
        return_from_routine(a);
        break;
    case 12:                                            /* jump */
        pc += (short)a - 2;
        break;
    case 13:                                            /* print_paddr */
        print_zstring(2 * a);
        break;
    case 14:                                            /* load */
        store_result(a ? get_var(a) : 0);
        break;
    case 15:                                            /* not */
        store_result(~a);
        break;
    }
}

void zero_operand(int opcode)
{
    switch (opcode & 15) {
    case 0:                                             /* rtrue */
        return_from_routine(1);
        break;
    case 1:                                             /* rfalse */
        return_from_routine(0);
        break;
    case 2:                                             /* print */
        pc = mem + print_zstring(pc - mem);
        break;
    case 3:                                             /* print_ret */
        print_zstring(pc - mem);
        print_char(13);
        return_from_routine(1);
        break;
    case 4:                                             /* nop */
        break;
    case 5:                                             /* save */
        branch(save_game());
        break;
    case 6:                                             /* restore */
        branch(restore_game());
        break;
    case 7:                                             /* restart */
        restart_game();
        break;
    case 8:                                             /* ret_popped */
        return_from_routine(*sp--);
        break;
    case 9:                                             /* pop */
        sp--;
        break;
    case 10:                                            /* quit */
        exit(0);
    case 11:                                            /* new_line */
        print_char(13);
        break;
    case 12:                                            /* show_status */
        show_status();
        break;
    case 13:                                            /* verify */
        branch(1);
        break;
    default:
        exit(0);
    }
}

void variable_operand(int opcode)
{
    long a = operand[0], b = operand[1], c = operand[2], p, value;

    switch (opcode & 31) {
    case 0:                                             /* call */
        call_routine();
        break;
    case 1:                                             /* storew */
        write_word(a + 2 * b, c);
        break;
    case 2:                                             /* storeb */
        mem[a + b] = c;
        break;
    case 3:                                             /* put_prop */
        if (a) {
            p = find_property(a, b);
            if (mem[p] >> 5)
                write_word(p + 1, c);
            else
                mem[p + 1] = c;
        }
        break;
    case 4:                                             /* sread */
        read_line(a, b);
        break;
    case 5:                                             /* print_char */
        print_char(a);
        break;
    case 6:                                             /* print_num */
        print_number(a);
        break;
    case 7:                                             /* random */
        store_result(rand() % a + 1);
        break;
    case 8:                                             /* push */
        *++sp = a;
        break;
    case 9:                                             /* pull */
        value = *sp--;
        if (a)
            set_var(a, value);
        else
            *sp = value;
        break;
    default:
        exit(0);
    }
}

void execute(int opcode)
{
    if (opcode < 0x80 || (opcode >= 0xC0 && opcode < 0xE0))
        two_operand(opcode);
    else if (opcode < 0xB0)
        one_operand(opcode);
    else if (opcode < 0xC0)
        zero_operand(opcode);
    else
        variable_operand(opcode);
}

/*
 * Each instruction is an opcode byte followed by its operands.  Every
 * operand has a two bit type: 0 large constant, 1 small constant,
 * 2 variable, 3 no more operands.
 */
int main(int argc, char **argv)
{
    int opcode, types, n;

    (void)argc;
    args = argv;
    stack = (unsigned short *)(mem + STORY_SIZE);
    execute(0xB7);                              /* restart: load the story */

    for (;;) {
        opcode = *pc++;
        if (opcode < 0x80)                      /* long form: 2 operands */
            types = (opcode & 0x40 ? 0x80 : 0x40) |
                    (opcode & 0x20 ? 0x20 : 0x10) | 0x0F;
        else if (opcode < 0xC0)                 /* short form: 0 or 1 */
            types = (opcode & 0x30) << 2 | 0x3F;
        else                                    /* variable form */
            types = *pc++;

        for (n = 0; (types >> 6 & 3) != 3; n++, types = types << 2 | 3) {
            switch (types >> 6 & 3) {
            case 0:
                operand[n] = fetch_word();
                break;
            case 1:
                operand[n] = *pc++;
                break;
            case 2:
                operand[n] = get_var(*pc++);
                break;
            }
        }
        operand_count = n;
        execute(opcode);
    }
}
