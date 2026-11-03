/*
 * A bytecode interpreter for OC (Obfuscated C).
 *
 * Usage: cat august.c program.oc | ./august > program.oo
 *        ./august < program.oo
 *
 * The bytecode program is read from standard input.  It is either a raw
 * compiled .oo file, or the compressed OC compiler that is hidden in the
 * comment at the top of august.c.  What remains of standard input is left
 * for the interpreted program to read with getchar().
 */

#define MEMORY_SIZE 240000

/*
 * One big int array holds everything:
 *   memory[0 .. code_size-1]  the bytecode
 *   memory[code_size ...]     global variables
 *   ... memory[MEMORY_SIZE]   the stack, growing downwards
 */
int memory[MEMORY_SIZE];

int sp;        /* stack pointer, memory[sp] is the top of the stack */
int fp;        /* frame pointer of the current function call */
int pc;        /* program counter */
int code_size; /* size of the bytecode, globals start right after it */

/* Opcodes that do not take an operand */
enum {
    OP_ASSIGN = 1,      /* *addr = value, leave value on the stack */
    OP_EQ = 6,
    OP_NE = 7,
    OP_LT = 8,
    OP_ASSIGN_POP = 9,  /* *addr = value, remove both from the stack */
    OP_RETURN = 10,
    OP_ADD = 12,
    OP_SUB = 13,
    OP_MUL = 14,
    OP_DIV = 15,
    OP_MOD = 16,
    OP_NOT = 17,
    OP_NEG = 18,
    OP_LOAD_INC = 19,   /* x = (*addr)++ */
    OP_DUP = 21,
    OP_POP = 22,
    OP_LOAD = 23,       /* x = *addr */
    OP_LOAD_DEC = 25,   /* x = (*addr)-- */
    OP_HALT = 27
};

/* Opcodes that have an operand: the high nibble is the opcode */
enum {
    OP_ARG_ADDR = 2,    /* push address of argument */
    OP_ARG = 3,         /* push value of argument */
    OP_LOCAL_ADDR = 4,  /* push address of local variable */
    OP_LOCAL = 5,       /* push value of local variable */
    OP_GLOBAL_ADDR = 6, /* push address of global variable */
    OP_GLOBAL = 7,      /* push value of global variable */
    OP_CONST = 8,       /* push constant */
    OP_SKIP = 9,        /* push address of inline data and jump over it */
    OP_CALL = 11,
    OP_CONST2 = 12,     /* push constant (same as OP_CONST) */
    OP_JUMP = 13,
    OP_JUMP_FALSE = 14,
    OP_JUMP_TRUE = 15
};

/* Built in functions are called through these "addresses" */
#define BUILTIN_GETCHAR 0
#define BUILTIN_LIMIT 3 /* addresses 1 and 2 are putchar */

/* Size of a call frame: saved pc, fp, call operand and 16 locals */
#define FRAME_SIZE 19

/*
 * Bytes that are encoded in the compressed bytecode by a sequence of
 * whitespace characters (so they do not count towards the size limit).
 */
unsigned char escape_table[] = {
    ' ',  '\t', '\n', ';',  '{',  '}',  '\0', 'A',  0xca, 'P',  0x80, 0xb3,
    0x06, 0xb2, 0x89, 0x8a, '@',  0x13, 0xc9, 0xb5, 0x81, 'g',  0x04, 'E',
    0xcf, 0xcd, 0xb1, 0xb0, 0x8c, '0',  0x17, 0x8d, 0x8e, 0x8b, 'q',  '\0'};

void push(int value)
{
    sp--;
    memory[sp] = value;
}

int pop()
{
    return memory[sp++];
}

int fetch()
{
    return memory[pc++];
}

/*
 * Unpack the LZ compressed bytecode in the comment of august.c.
 *   ';' '{' '}'        back reference: copy 2, 5 or 8 (+0..3) earlier bytes
 *   ' ' '\t' '\n'      an escaped byte looked up in escape_table
 *   anything else      a literal byte
 */
void load_compressed(int *dest)
{
    int remaining, ch, next, length, offset, index;

    remaining = code_size;
    while (remaining) {
        ch = getchar();

        if (ch == ';')
            length = 2;
        else if (ch == '{')
            length = 5;
        else if (ch == '}')
            length = 8;
        else
            length = 0;

        if (length) {
            /* back reference to bytes already unpacked */
            length = length + getchar() % 4;
            offset = 1 - getchar() - length;
            while (length--) {
                *dest = dest[offset];
                dest++;
                remaining--;
            }
        } else {
            if (ch == ' ' || ch == '\t' || ch == '\n') {
                /* two or three whitespace characters select a table entry */
                next = getchar();
                index = ch % 4 + (next + (next == '{')) % 8 * 3;
                if (8 < index)
                    index = index + getchar() % 4 * 9;
                ch = escape_table[index];
            }
            *dest++ = ch % 256;
            remaining--;
        }
    }

    /* skip the rest of august.c, it ends with a form feed and a newline */
    while (getchar() != '\f')
        ;
    getchar();
}

/* Read the bytecode of a compiled .oo file as it is */
void load_raw(int *dest)
{
    int remaining;

    remaining = code_size;
    while (remaining--)
        *dest++ = getchar();
}

/* Pop the arguments of a call and leave the result in the function's slot */
void finish_call(int operand, int result)
{
    int argc = operand / 2;
    int discard_result = operand % 2;

    sp = sp + argc;
    memory[sp] = result;
    if (discard_result)
        sp++;
}

void call(int operand)
{
    int function, result;

    /* the function address is below the arguments on the stack */
    function = memory[sp + operand / 2];

    if (function < BUILTIN_LIMIT) {
        if (function != BUILTIN_GETCHAR)
            result = putchar(memory[sp]);
        else
            result = getchar();
        finish_call(operand, result);
    } else {
        memory[sp - 1] = pc;
        memory[sp - 2] = fp;
        memory[sp - 3] = operand;
        fp = sp;
        sp = sp - FRAME_SIZE;
        pc = function;
    }
}

void return_from_call()
{
    int result, operand;

    result = memory[sp];
    sp = fp;
    pc = memory[sp - 1];
    fp = memory[sp - 2];
    operand = memory[sp - 3];
    finish_call(operand, result);
}

/* Execute an instruction whose high nibble is the opcode */
void execute_with_operand(int op)
{
    int operand;

    operand = op % 16;
    op = op / 16;

    if (operand == 7) {
        /* 16 bit operand follows */
        operand = fetch();
        operand = operand + fetch() * 256;
    } else if (7 < operand) {
        /* 8 more bits follow */
        operand = operand + fetch() * 8 - 1;
    }

    switch (op) {
    case OP_ARG_ADDR:
        push(fp + operand);
        break;
    case OP_LOCAL_ADDR:
        push(fp - operand - 4);
        break;
    case OP_ARG:
        push(memory[fp + operand]);
        break;
    case OP_LOCAL:
        push(memory[fp - operand - 4]);
        break;
    case OP_GLOBAL_ADDR:
        push(code_size + operand);
        break;
    case OP_GLOBAL:
        push(memory[code_size + operand]);
        break;
    case OP_CONST2:
    case OP_CONST:
        push(operand);
        break;
    case OP_SKIP:
        push(pc);
        pc = pc + operand;
        break;
    case OP_CALL:
        call(operand);
        break;
    case OP_JUMP:
        pc = operand;
        break;
    case OP_JUMP_TRUE:
        if (pop())
            pc = operand;
        break;
    case OP_JUMP_FALSE:
        if (!pop())
            pc = operand;
        break;
    }
}

int main()
{
    int compressed, value, address;

    getchar(); /* '/' of the comment in august.c, or 0 in a .oo file */
    compressed = getchar(); /* '*' of the comment, or 0 in a .oo file */
    code_size = getchar();
    code_size = code_size + getchar() * 256;

    if (compressed)
        load_compressed(memory);
    else
        load_raw(memory);

    sp = MEMORY_SIZE;
    pc = 0;
    while (1) {
        int op = fetch();

        switch (op) {
        case OP_NOT:
            memory[sp] = !memory[sp];
            break;
        case OP_NEG:
            memory[sp] = -memory[sp];
            break;
        case OP_ADD:
            value = pop();
            memory[sp] = memory[sp] + value;
            break;
        case OP_SUB:
            value = pop();
            memory[sp] = memory[sp] - value;
            break;
        case OP_MUL:
            value = pop();
            memory[sp] = memory[sp] * value;
            break;
        case OP_DIV:
            value = pop();
            memory[sp] = memory[sp] / value;
            break;
        case OP_MOD:
            value = pop();
            memory[sp] = memory[sp] % value;
            break;
        case OP_EQ:
            value = pop();
            memory[sp] = memory[sp] == value;
            break;
        case OP_NE:
            value = pop();
            memory[sp] = memory[sp] != value;
            break;
        case OP_LT:
            value = pop();
            memory[sp] = memory[sp] < value;
            break;
        case OP_ASSIGN:
            value = pop();
            memory[memory[sp]] = value;
            memory[sp] = value;
            break;
        case OP_ASSIGN_POP:
            value = pop();
            memory[memory[sp]] = value;
            sp++;
            break;
        case OP_RETURN:
            return_from_call();
            break;
        case OP_POP:
            sp++;
            break;
        case OP_HALT:
            return memory[sp];
        case OP_DUP:
            push(memory[sp]);
            break;
        case OP_LOAD:
            memory[sp] = memory[memory[sp]];
            break;
        case OP_LOAD_INC:
            address = memory[sp];
            memory[sp] = memory[address]++;
            break;
        case OP_LOAD_DEC:
            address = memory[sp];
            memory[sp] = memory[address]--;
            break;
        default:
            execute_with_operand(op);
            break;
        }
    }
}
