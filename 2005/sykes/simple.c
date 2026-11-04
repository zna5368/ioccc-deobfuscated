#include <stdio.h>
#include <string.h>
#include <curses.h>

/*
 * A MOS 6502 processor emulator that can also fake enough of the
 * Commodore PET 2001 hardware to run the PET ROMs.
 */

#define ADDRESS_MASK 0xFFFF
#define LOAD_ADDRESS 0xC000   /* program file is loaded and started here */
#define STACK_PAGE 0x100
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR 0xFFFE

/* screen: 40 x 25 characters starting at $8000 */
#define SCREEN_START 0x8000
#define SCREEN_COLUMNS 40
#define SCREEN_CELLS 1000

/* plain 6502 mode: the last key pressed is stored here, ORed with $80 */
#define KEY_INPUT 0xA2

/* PET hardware (PIA 1): keyboard row select and keyboard column read */
#define PIA1_PORT_A 0xE810    /* low nybble = keyboard row, bit 7 = diagnostic sense */
#define PIA1_PORT_B 0xE812    /* key bits of selected row (0 = pressed) */
#define SHIFT_ROW 8           /* left shift key is row 8, column 0 */

/* PET BASIC / kernal locations used to fake LOAD and SAVE */
#define KERNAL_LOAD 0xFFD5    /* LOAD is trapped at $FFD5, SAVE at $FFD8 */
#define TXTPTR_LO 0x77        /* BASIC text pointer (low byte) */
#define INPUT_BUFFER 0x200    /* BASIC line input buffer */
#define BASIC_START 0x401     /* start of BASIC program text */
#define VARTAB 0x2A           /* VARTAB, ARYTAB, STREND: end of program */
#define OPCODE_RTS 0x60

/* the status flags are kept apart, each one holding its own bit value */
#define FLAG_CARRY 1
#define FLAG_ZERO 2
#define FLAG_INTERRUPT 4
#define FLAG_DECIMAL 8
#define FLAG_BREAK 16
#define FLAG_UNUSED 32
#define FLAG_OVERFLOW 64
#define FLAG_NEGATIVE 128

int pc, carry, zero, interrupt_disable, decimal, break_flag, overflow, negative,
    load_address = LOAD_ADDRESS, temp, group, instruction_count, key, speed,
    address, rom_end;
unsigned char *operand, memory[65536], a, x, y, sp, *quote;

/* ---------------------------------------------------------------------- */
/* Lookup tables                                                          */
/* ---------------------------------------------------------------------- */

/* PET keyboard matrix: (row + 5) * 8 + column, so that unused entries
 * (0) give a negative row and never match. */
#define KEY(row, column) (((row) + 5) * 8 + (column))

static const unsigned char keyboard_matrix[128] = {
    [1] = KEY(0, 6),    /* ^A HOME */
    [2] = KEY(0, 7),    /* ^B cursor back (shifted cursor right) */
    [5] = KEY(9, 0),    /* ^E OFF (shifted RVS) */
    [6] = KEY(0, 7),    /* ^F cursor forward */
    [8] = KEY(1, 7),    /* ^H DEL */
    ['\n'] = KEY(6, 5), /* RETURN */
    [12] = KEY(0, 6),   /* ^L CLR (shifted HOME) */
    [14] = KEY(1, 6),   /* ^N cursor down */
    [16] = KEY(1, 6),   /* ^P cursor up (shifted cursor down) */
    [18] = KEY(9, 0),   /* ^R RVS */
    [24] = KEY(9, 4),   /* ^X STOP */
    [25] = KEY(1, 7),   /* ^Y INST (shifted DEL) */
    [' '] = KEY(9, 2),  ['!'] = KEY(0, 0),  ['"'] = KEY(1, 0),
    ['#'] = KEY(0, 1),  ['$'] = KEY(1, 1),  ['%'] = KEY(0, 2),
    ['&'] = KEY(0, 3),  ['\''] = KEY(1, 2), ['('] = KEY(0, 4),
    [')'] = KEY(1, 4),  ['*'] = KEY(5, 7),  ['+'] = KEY(7, 7),
    [','] = KEY(7, 3),  ['-'] = KEY(8, 7),  ['.'] = KEY(9, 6),
    ['/'] = KEY(3, 7),  ['0'] = KEY(8, 6),  ['1'] = KEY(6, 6),
    ['2'] = KEY(7, 6),  ['3'] = KEY(6, 7),  ['4'] = KEY(4, 6),
    ['5'] = KEY(5, 6),  ['6'] = KEY(4, 7),  ['7'] = KEY(2, 6),
    ['8'] = KEY(3, 6),  ['9'] = KEY(2, 7),  [':'] = KEY(5, 4),
    [';'] = KEY(6, 4),  ['<'] = KEY(9, 3),  ['='] = KEY(9, 7),
    ['>'] = KEY(8, 4),  ['?'] = KEY(7, 4),  ['@'] = KEY(8, 1),
    ['A'] = KEY(4, 0),  ['B'] = KEY(6, 2),  ['C'] = KEY(6, 1),
    ['D'] = KEY(4, 1),  ['E'] = KEY(2, 1),  ['F'] = KEY(5, 1),
    ['G'] = KEY(4, 2),  ['H'] = KEY(5, 2),  ['I'] = KEY(3, 3),
    ['J'] = KEY(4, 3),  ['K'] = KEY(5, 3),  ['L'] = KEY(4, 4),
    ['M'] = KEY(6, 3),  ['N'] = KEY(7, 2),  ['O'] = KEY(2, 4),
    ['P'] = KEY(3, 4),  ['Q'] = KEY(2, 0),  ['R'] = KEY(3, 1),
    ['S'] = KEY(5, 0),  ['T'] = KEY(2, 2),  ['U'] = KEY(2, 3),
    ['V'] = KEY(7, 1),  ['W'] = KEY(3, 0),  ['X'] = KEY(7, 0),
    ['Y'] = KEY(3, 2),  ['Z'] = KEY(6, 0),  ['['] = KEY(9, 1),
    ['\\'] = KEY(1, 3), [']'] = KEY(8, 2),  ['^'] = KEY(2, 5),
    ['_'] = KEY(0, 5),  ['a'] = KEY(4, 0),  ['b'] = KEY(6, 2),
    ['c'] = KEY(6, 1),  ['d'] = KEY(4, 1),  ['e'] = KEY(2, 1),
    ['f'] = KEY(5, 1),  ['g'] = KEY(4, 2),  ['h'] = KEY(5, 2),
    ['i'] = KEY(3, 3),  ['j'] = KEY(4, 3),  ['k'] = KEY(5, 3),
    ['l'] = KEY(4, 4),  ['m'] = KEY(6, 3),  ['n'] = KEY(7, 2),
    ['o'] = KEY(2, 4),  ['p'] = KEY(3, 4),  ['q'] = KEY(2, 0),
    ['r'] = KEY(3, 1),  ['s'] = KEY(5, 0),  ['t'] = KEY(2, 2),
    ['u'] = KEY(2, 3),  ['v'] = KEY(7, 1),  ['w'] = KEY(3, 0),
    ['x'] = KEY(7, 0),  ['y'] = KEY(3, 2),  ['z'] = KEY(6, 0),
    ['~'] = KEY(1, 5),  [127] = KEY(1, 7),
};

/* keys that must be typed with SHIFT held down on the PET */
static const unsigned char needs_shift[128] = {
    [2] = 1, [5] = 1, [12] = 1, [16] = 1, [25] = 1,
};

/* operations; the conditional branches and flag instructions share one
 * implementation each, and the top two opcode bits select the flag */
enum operation {
    ___, ADC, AND, ASL, BRANCH_IF_SET, BRANCH_IF_CLEAR, BIT, BRK, SET_FLAG,
    CLEAR_FLAG, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP, JSR,
    LDA, LDX, LDY, LSR, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI, RTS, SBC, STA,
    STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
    BPL = BRANCH_IF_CLEAR, BMI = BRANCH_IF_SET,
    BVC = BRANCH_IF_CLEAR, BVS = BRANCH_IF_SET,
    BCC = BRANCH_IF_CLEAR, BCS = BRANCH_IF_SET,
    BNE = BRANCH_IF_CLEAR, BEQ = BRANCH_IF_SET,
    CLC = CLEAR_FLAG, CLI = CLEAR_FLAG, CLV = CLEAR_FLAG, CLD = CLEAR_FLAG,
    SEC = SET_FLAG, SEI = SET_FLAG, SED = SET_FLAG,
};

static const unsigned char operations[256] = {
/*        x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF */
/* 0x */ BRK, ORA, ___, ___, ___, ORA, ASL, ___, PHP, ORA, ASL, ___, ___, ORA, ASL, ___,
/* 1x */ BPL, ORA, ___, ___, ___, ORA, ASL, ___, CLC, ORA, ___, ___, ___, ORA, ASL, ___,
/* 2x */ JSR, AND, ___, ___, BIT, AND, ROL, ___, PLP, AND, ROL, ___, BIT, AND, ROL, ___,
/* 3x */ BMI, AND, ___, ___, ___, AND, ROL, ___, SEC, AND, ___, ___, ___, AND, ROL, ___,
/* 4x */ RTI, EOR, ___, ___, ___, EOR, LSR, ___, PHA, EOR, LSR, ___, JMP, EOR, LSR, ___,
/* 5x */ BVC, EOR, ___, ___, ___, EOR, LSR, ___, CLI, EOR, ___, ___, ___, EOR, LSR, ___,
/* 6x */ RTS, ADC, ___, ___, ___, ADC, ROR, ___, PLA, ADC, ROR, ___, JMP, ADC, ROR, ___,
/* 7x */ BVS, ADC, ___, ___, ___, ADC, ROR, ___, SEI, ADC, ___, ___, ___, ADC, ROR, ___,
/* 8x */ ___, STA, ___, ___, STY, STA, STX, ___, DEY, ___, TXA, ___, STY, STA, STX, ___,
/* 9x */ BCC, STA, ___, ___, STY, STA, STX, ___, TYA, STA, TXS, ___, ___, STA, ___, ___,
/* Ax */ LDY, LDA, LDX, ___, LDY, LDA, LDX, ___, TAY, LDA, TAX, ___, LDY, LDA, LDX, ___,
/* Bx */ BCS, LDA, ___, ___, LDY, LDA, LDX, ___, CLV, LDA, TSX, ___, LDY, LDA, LDX, ___,
/* Cx */ CPY, CMP, ___, ___, CPY, CMP, DEC, ___, INY, CMP, DEX, ___, CPY, CMP, DEC, ___,
/* Dx */ BNE, CMP, ___, ___, ___, CMP, DEC, ___, CLD, CMP, ___, ___, ___, CMP, DEC, ___,
/* Ex */ CPX, SBC, ___, ___, CPX, SBC, INC, ___, INX, SBC, ___, ___, CPX, SBC, INC, ___,
/* Fx */ BEQ, SBC, ___, ___, ___, SBC, INC, ___, SED, SBC, ___, ___, ___, SBC, INC, ___,
};

/* addressing modes, selected by opcode bits 4,3,2 and 0 */
enum addressing_mode {
    ACCUMULATOR, /* also implied */
    INDIRECT_X,
    IMMEDIATE,   /* also relative branches and BRK/JSR/RTI/RTS */
    ZERO_PAGE,
    ABSOLUTE,
    INDIRECT_Y,
    ZERO_PAGE_X, /* zero page,Y for STX and LDX */
    ABSOLUTE_Y,
    ABSOLUTE_X,  /* absolute,Y for LDX */
};

static const unsigned char addressing_modes[16] = {
    IMMEDIATE,   INDIRECT_X, ZERO_PAGE,   ZERO_PAGE,
    ACCUMULATOR, IMMEDIATE,  ABSOLUTE,    ABSOLUTE,
    IMMEDIATE,   INDIRECT_Y, ZERO_PAGE_X, ZERO_PAGE_X,
    ACCUMULATOR, ABSOLUTE_Y, ABSOLUTE_X,  ABSOLUTE_X,
};

/* ---------------------------------------------------------------------- */
/* CPU helpers                                                            */
/* ---------------------------------------------------------------------- */

#define READ(addr) memory[ADDRESS_MASK & (addr)]
#define PUSH(value) (memory[ADDRESS_MASK & (sp-- + STACK_PAGE)] = (value))
#define PULL() memory[ADDRESS_MASK & (++sp + STACK_PAGE)]
#define STATUS() (carry | zero | interrupt_disable | decimal | break_flag | \
                  overflow | negative | FLAG_UNUSED)

/* little-endian 16-bit read */
int read16(int addr)
{
    return READ(addr) + READ(addr + 1) * 256;
}

/* set the zero and negative flags from a result */
void set_zero_negative(int value)
{
    zero = value ? 0 : FLAG_ZERO;
    negative = value & FLAG_NEGATIVE;
}

/* CMP, CPX, CPY */
void compare(int reg)
{
    set_zero_negative(reg - *operand);
    carry = reg < *operand ? 0 : 1;
}

/* PLP and RTI: pull the status byte off the stack */
void pull_status(void)
{
    set_zero_negative(temp = PULL());
    carry = temp & FLAG_CARRY;
    zero = temp & FLAG_ZERO;
    interrupt_disable = temp & FLAG_INTERRUPT;
    decimal = temp & FLAG_DECIMAL;
    break_flag = temp & FLAG_BREAK;
    overflow = temp & FLAG_OVERFLOW;
}

void push_pc(void)
{
    PUSH(pc / 256);
    PUSH(pc);
}

/* IRQ and BRK: push state and jump through the IRQ vector */
void interrupt(void)
{
    push_pc();
    PUSH(STATUS());
    interrupt_disable = FLAG_INTERRUPT;
    pc = read16(IRQ_VECTOR);
}

/* group (opcode >> 6) selects the flag tested by a branch:
 * 0 = negative, 1 = overflow, 2 = carry, 3 = zero */
int branch_flag(void)
{
    switch (group) {
    case 0: return negative;
    case 1: return overflow;
    case 2: return carry;
    default: return zero;
    }
}

/* group selects the flag changed by CLC/SEC, CLI/SEI, CLV, CLD/SED */
int *flag_to_change(void)
{
    switch (group) {
    case 0: return &carry;
    case 1: return &interrupt_disable;
    case 2: return &overflow;
    default: return &decimal;
    }
}

int flag_set_value(void)
{
    switch (group) {
    case 0: return FLAG_CARRY;
    case 1: return FLAG_INTERRUPT;
    default: return 8;
    }
}

void branch(int taken)
{
    if (taken)
        pc += *operand & 128 ? *operand - 256 : *operand;
}

/* ---------------------------------------------------------------------- */
/* PET kernal LOAD and SAVE replacement                                   */
/* ---------------------------------------------------------------------- */

/* trap 0 = LOAD, anything else = SAVE.  The file name is the quoted
 * string at the BASIC text pointer in the input buffer. */
void kernal_load_save(int trap)
{
    FILE *file;
    int countdown, ch;

    operand = memory + INPUT_BUFFER + memory[TXTPTR_LO];
    if (*operand && (quote = strchr((char *)++operand, '"'))) {
        *quote = 0;
        memory[TXTPTR_LO] = quote - memory + 1;
        file = fopen((char *)operand, trap ? "w" : "r+");
        if (file) {
            int addr = BASIC_START - trap;
            if (!trap) {
                /* skip the two byte load address, then read the program */
                fgetc(file);
                fgetc(file);
                while ((ch = fgetc(file)) != EOF)
                    memory[ADDRESS_MASK & addr++] = ch;
                /* VARTAB, ARYTAB and STREND all point at the program end */
                for (operand = memory + VARTAB; operand < memory + VARTAB + 5;) {
                    *operand++ = addr % 256;
                    *operand++ = addr / 256;
                }
            }
            /* SAVE: write from $3FF on until three zero bytes in a row */
            for (countdown = trap; countdown; fputc(READ(addr), file))
                countdown = READ(++addr) ? 3 : countdown - 1;
            fclose(file);
        }
    }
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    FILE *file;
    int ch, row, keybits, irq_period, opcode, trap;

    memory[0] = time(0); /* random seed */

    file = fopen(argc > 1 ? argv[1] : "", "r+");
    if (!file) {
        perror(*argv);
        return 0;
    }

    for (temp = load_address; (ch = fgetc(file)) != EOF; temp++)
        memory[ADDRESS_MASK & temp] = ch;

    /* a 16384 byte file ends exactly at $10000: it is a PET ROM */
    rom_end = temp & ADDRESS_MASK;
    pc = rom_end ? load_address : read16(RESET_VECTOR);

    /* speed parameter; -1 also leaves the PET diagnostic sense line low */
    speed = argc > 2 ? atoi(argv[2]) + 1 : 4;
    irq_period = speed ? ADDRESS_MASK * speed / 4 : ADDRESS_MASK;

    nodelay(initscr(), 1);
    curs_set(0);
    cbreak();
    noecho();

    for (;;) {
        /* poll the keyboard now and then */
        if (instruction_count++ % (irq_period * 4) == 0) {
            key = getch();
            if (key != ERR)
                memory[KEY_INPUT] = key | 128;
        }

        /* PET keyboard: put the bits of the held key for the row
         * selected in port A into port B (active low) */
        if (!rom_end) {
            row = (memory[PIA1_PORT_A] |= (speed ? 128 : 0)) & 15;
            keybits = 0;
            if (key != ERR && key < 128) { /* ASCII keys only */
                if (row == SHIFT_ROW)
                    keybits |= needs_shift[key] & 1;
                if (keyboard_matrix[key] / 8 - 5 == row)
                    keybits |= 1 << keyboard_matrix[key] % 8;
            }
            memory[PIA1_PORT_B] = ~keybits;
        }

        /* the 60Hz jiffy interrupt */
        if (instruction_count % irq_period == 0 && !interrupt_disable)
            interrupt();

        /* fetch */
        trap = pc % KERNAL_LOAD;
        opcode = READ(pc++);
        if (rom_end + trap < 4) {
            kernal_load_save(trap);
            opcode = OPCODE_RTS;
        }

        /* decode the operand address */
        switch (addressing_modes[(opcode >> 1 & 14) | (opcode & 1)]) {
        case ABSOLUTE_X:
            address = read16(pc) + (opcode != 0xBE ? x : y);
            pc += 2;
            break;
        case ABSOLUTE_Y:
            address = read16(pc) + y;
            pc += 2;
            break;
        case ZERO_PAGE_X:
            address = (READ(pc++) + (opcode != 0x96 && opcode != 0xB6 ? x : y)) & 255;
            break;
        case INDIRECT_Y:
            temp = READ(pc);
            pc += 1;
            address = read16(temp) + y;
            break;
        case ABSOLUTE:
            address = read16(pc);
            pc += 2;
            break;
        case ZERO_PAGE:
            address = READ(pc++);
            break;
        case IMMEDIATE:
            address = pc++;
            break;
        case INDIRECT_X:
            temp = (READ(pc) + x) & 255;
            pc += 1;
            address = read16(temp);
            break;
        default:
            address = -1;
            break;
        }
        operand = address < 0 ? &a : memory + address;
        group = opcode >> 6;

        /* execute */
        switch (operations[opcode]) {
        case TYA: set_zero_negative(a = y); break;
        case TXS: sp = x; break;
        case TXA: set_zero_negative(a = x); break;
        case TSX: set_zero_negative(x = sp); break;
        case TAY: set_zero_negative(y = a); break;
        case TAX: set_zero_negative(x = a); break;
        case STY: *operand = y; break;
        case STX: *operand = x; break;
        case STA: *operand = a; break;
        case SBC:
            temp = a - *operand - 1 + carry;
            carry = temp & 256 ? 0 : 1;
            overflow = ((a ^ *operand) & (a ^ temp) & 128) / 2;
            set_zero_negative(a = temp & 255);
            break;
        case RTS:
            pc = PULL();
            pc += PULL() * 256 + 1;
            break;
        case RTI:
            pull_status();
            pc = PULL();
            pc |= PULL() * 256;
            break;
        case ROR:
            temp = *operand;
            set_zero_negative(*operand = *operand / 2 | carry * 128);
            carry = temp & 1;
            break;
        case ROL:
            temp = *operand;
            set_zero_negative(*operand = *operand * 2 | carry);
            carry = temp / 128;
            break;
        case PLP: pull_status(); break;
        case PLA: set_zero_negative(a = PULL()); break;
        case PHP: PUSH(STATUS()); break;
        case PHA: PUSH(a); break;
        case ORA: set_zero_negative(a |= *operand); break;
        case LSR:
            carry = *operand & 1;
            set_zero_negative(*operand /= 2);
            break;
        case LDY: set_zero_negative(y = *operand); break;
        case LDX: set_zero_negative(x = *operand); break;
        case LDA: set_zero_negative(a = *operand); break;
        case JSR: {
            int target = read16(pc - 1);
            push_pc();
            pc = target;
            break;
        }
        case JMP: /* $6C is JMP (indirect) */
            pc = opcode & 32 ? read16(address) : address;
            break;
        case INY: set_zero_negative(++y); break;
        case INX: set_zero_negative(++x); break;
        case INC: set_zero_negative(++*operand); break;
        case EOR: set_zero_negative(a ^= *operand); break;
        case DEY: set_zero_negative(--y); break;
        case DEX: set_zero_negative(--x); break;
        case DEC: set_zero_negative(--*operand); break;
        case CPY: compare(y); break;
        case CPX: compare(x); break;
        case CMP: compare(a); break;
        case CLEAR_FLAG: *flag_to_change() = 0; break;
        case SET_FLAG: *flag_to_change() = flag_set_value(); break;
        case BRK:
            pc--;
            break_flag = FLAG_BREAK;
            interrupt();
            break;
        case BIT:
            set_zero_negative(a & *operand);
            overflow = *operand & 64;
            negative = *operand & 128;
            break;
        case BRANCH_IF_CLEAR: branch(!branch_flag()); break;
        case BRANCH_IF_SET: branch(branch_flag()); break;
        case ASL:
            carry = *operand / 128;
            set_zero_negative(*operand *= 2);
            break;
        case AND: set_zero_negative(a &= *operand); break;
        case ADC:
            temp = a + *operand + carry;
            carry = temp & 256 ? 1 : 0;
            overflow = (~(a ^ *operand) & (a ^ temp) & 128) / 2;
            set_zero_negative(a = temp & 255);
            break;
        }

        /* redraw the screen cell if the operand was in screen memory */
        if (address >= SCREEN_START && address < SCREEN_START + SCREEN_CELLS) {
            int cell = address - SCREEN_START;
            ch = *operand % 128;
            if (*operand > ch)
                attron(A_REVERSE);
            else
                attroff(A_REVERSE);
            /* PET screen codes to ASCII; plain 6502 mode prints ASCII */
            mvaddch(cell / SCREEN_COLUMNS, cell % SCREEN_COLUMNS,
                    ch + rom_end < 32 ? ch + 64 : ch > 95 + rom_end ? ch - 32 : ch);
        }
    }
}
