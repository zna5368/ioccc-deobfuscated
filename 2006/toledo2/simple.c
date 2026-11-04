#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Intel 8080 emulator with a teletype (stdin/stdout), an import file
 * (argv[1]) and two 128-byte-sector disk drives (files "A" and "B").
 * The boot image is loaded at address 0 from the file "C".
 */

#define MEMORY_SIZE 78114 /* 64K plus slack so 16-bit overruns stay inside */
#define DISK_PARAMS 0xFBFA /* DMA low/high, sector, cylinder low/high, drive */

/* register file; the 3-bit register code 6 means memory at HL instead */
enum {
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_H,
    REG_L,
    REG_FLAGS,
    REG_A,
    REG_SP_HIGH,
    REG_SP_LOW
};
#define CODE_MEMORY 6

#define FLAG_CARRY 0x01
#define FLAG_ALWAYS 0x02
#define FLAG_PARITY 0x04
#define FLAG_AUX 0x10
#define FLAG_ZERO 0x40
#define FLAG_SIGN 0x80

#define OP_HLT 0x76
#define OP_ED 0xED
#define DISK_READ 2
#define IMPORT_PORT 2

unsigned char regs[10];
unsigned char memory[MEMORY_SIZE];
unsigned char *pc = memory;
unsigned char *disk = memory + DISK_PARAMS;

FILE *drive_b, *drive_a, *boot_file, *import_file;
int pending_key;

#define FLAGS regs[REG_FLAGS]
#define ACC regs[REG_A]

/* 16-bit value of a register pair given the index of its low byte */
static int pair(int low)
{
    return regs[low] + 256 * regs[low - 1];
}

#define BC pair(REG_C)
#define DE pair(REG_E)
#define HL pair(REG_L)
#define SP pair(REG_SP_LOW)

/* pointer to register selected by a 3-bit code (B,C,D,E,H,L,M,A) */
static unsigned char *reg_ptr(int code)
{
    code &= 7;
    return code == CODE_MEMORY ? &memory[HL] : &regs[code];
}

/* fetch a little-endian 16-bit immediate operand */
static int fetch_word(void)
{
    pc += 2;
    return pc[-2] | pc[-1] << 8;
}

static void push(int value)
{
    regs[REG_SP_LOW] -= 2;
    if (regs[REG_SP_LOW] >= 254)
        regs[REG_SP_HIGH]--;
    memory[SP] = value;
    memory[SP + 1] = value >> 8;
}

static int pop(void)
{
    int value = memory[SP] | memory[SP + 1] << 8;
    regs[REG_SP_LOW] += 2;
    if (regs[REG_SP_LOW] < 2)
        regs[REG_SP_HIGH]++;
    return value;
}

/* set S, Z, P, AC and C from a result; bits 3 and 5 are cleared */
static void set_flags(int carry, int result, int aux)
{
    int ones = 0, bit;

    FLAGS = carry | (result & FLAG_SIGN) | ((result & 255) ? 0 : FLAG_ZERO) |
            (aux & FLAG_AUX) | FLAG_ALWAYS;
    for (bit = 0; bit < 8; bit++)
        ones += result >> bit & 1;
    if (!(ones & 1))
        FLAGS |= FLAG_PARITY;
}

/* condition code: 0=NZ 1=Z 2=NC 3=C 4=PO 5=PE 6=P 7=M */
static int condition(int code)
{
    static const int flag_bit[4] = {6, 0, 2, 7};
    int set = FLAGS >> flag_bit[code / 2] & 1;
    return code & 1 ? set : !set;
}

/* ADD/ADC/SUB/SBB/ANA/XRA/ORA/CMP on the accumulator */
static void alu(int operation, int operand)
{
    int result, aux;
    int carry_in = FLAGS & FLAG_CARRY;

    switch (operation) {
    case 0: /* ADD */
    case 1: /* ADC */
        if (operation == 0)
            carry_in = 0;
        aux = ACC % 16 + operand % 16 + carry_in;
        result = ACC + operand + carry_in;
        ACC = result;
        break;
    case 2: /* SUB */
    case 3: /* SBB */
    case 7: /* CMP */
        if (operation != 3)
            carry_in = 0;
        aux = ACC % 16 - operand % 16 - carry_in;
        result = ACC - operand - carry_in;
        if (operation != 7)
            ACC = result;
        break;
    case 4: /* ANA */
        aux = ACC * 2 | operand * 2;
        result = ACC &= operand;
        break;
    case 5: /* XRA */
        aux = 0;
        result = ACC ^= operand;
        break;
    default: /* ORA */
        aux = 0;
        result = ACC |= operand;
        break;
    }
    set_flags(result >> 8 & 1, result, aux);
}

static void disk_operation(void)
{
    FILE *drive;
    int is_read;
    long offset;
    unsigned char *dma;

    if (*pc++ != OP_ED)
        return;
    is_read = *pc++ == DISK_READ;
    drive = disk[5] != 1 ? drive_b : drive_a;
    offset = ((disk[3] | disk[4] << 8) << 7 | disk[2]) << 7;
    dma = memory + disk[0] + disk[1] * 256;
    ACC = 0;
    fseek(drive, offset, SEEK_SET);
    if (is_read)
        fread(dma, 128, 1, drive);
    else
        fwrite(dma, 128, 1, drive);
}

static void input(void)
{
    int port = *pc++;
    unsigned char byte;
    int ch;

    if (port == IMPORT_PORT) {
        ch = import_file ? fgetc(import_file) : -1;
        ACC = ch;
        FLAGS = (FLAGS & ~FLAG_CARRY) | (ch < 0);
        return;
    }
    if (!pending_key && read(0, &byte, 1) > 0)
        pending_key = byte;
    if (port & 1) { /* read key */
        ACC = pending_key;
        pending_key = 0;
    } else { /* key available status */
        ACC = pending_key != 0;
    }
}

static void execute(int op)
{
    int y = op / 8 % 8;              /* destination register or condition */
    int pair_high = y > 5 ? y + 2 : y; /* register pair index (SP is 8/9) */
    unsigned char *reg;
    int value, address, temp_low, temp_high, bit;

    if ((op & 0xC7) == 0x04 || (op & 0xC7) == 0x05) { /* INR / DCR */
        int delta = (op & 0xC7) == 0x04 ? 1 : -1;
        reg = reg_ptr(y);
        value = *reg;
        *reg = value + delta;
        set_flags(FLAGS & FLAG_CARRY, value + delta, value % 16 + delta);
    } else if ((op & 0xC7) == 0x06) { /* MVI r,imm */
        *reg_ptr(y) = *pc++;
    } else if ((op & 0xC7) == 0xC0) { /* Rcc */
        if (condition(y))
            pc = memory + pop();
    } else if ((op & 0xC7) == 0xC2) { /* Jcc */
        address = fetch_word();
        if (condition(y))
            pc = memory + address;
    } else if ((op & 0xC7) == 0xC4) { /* Ccc */
        address = fetch_word();
        if (condition(y)) {
            push(pc - memory);
            pc = memory + address;
        }
    } else if ((op & 0xC7) == 0xC7) { /* RST */
        push(pc - memory);
        pc = memory + y * 8;
    } else if ((op & 0xCF) == 0x01) { /* LXI rp,imm */
        regs[pair_high + 1] = *pc++;
        regs[pair_high] = *pc++;
    } else if ((op & 0xCF) == 0x03) { /* INX rp */
        if (++regs[pair_high + 1] == 0)
            regs[pair_high]++;
    } else if ((op & 0xCF) == 0x09) { /* DAD rp (here pair_high is the low byte) */
        value = regs[REG_L] + regs[pair_high];
        regs[REG_L] = value;
        value = regs[REG_H] + regs[pair_high - 1] + value / 256;
        regs[REG_H] = value;
        FLAGS = (FLAGS & ~FLAG_CARRY) | value >> 8;
    } else if ((op & 0xCF) == 0x0B) { /* DCX rp (here pair_high is the low byte) */
        if (regs[pair_high]-- == 0)
            regs[pair_high - 1]--;
    } else if ((op & 0xCF) == 0xC1) { /* POP rp / POP PSW */
        value = pop();
        if (pair_high != REG_SP_HIGH) {
            regs[pair_high + 1] = value;
            regs[pair_high] = value >> 8;
        } else {
            FLAGS = (value & ~0x28) | FLAG_ALWAYS;
            ACC = value >> 8;
        }
    } else if ((op & 0xCF) == 0xC5) { /* PUSH rp / PUSH PSW */
        push(pair_high != REG_SP_HIGH ? pair(pair_high + 1) : FLAGS | ACC << 8);
    } else if ((op & 0xC0) == 0x80) { /* ALU with register */
        alu(y, *reg_ptr(op));
    } else if ((op & 0xC7) == 0xC6) { /* ALU with immediate */
        alu(y, *pc++);
    } else if ((op & 0xC0) == 0x40) { /* MOV r,r (0x76 HLT never gets here) */
        *reg_ptr(y) = *reg_ptr(op);
    } else {
        switch (op) {
        case 0x02: /* STAX B */
            memory[BC] = ACC;
            break;
        case 0x07: /* RLC */
            FLAGS = (FLAGS & ~FLAG_CARRY) | ACC >> 7;
            ACC = ACC * 2 | ACC >> 7;
            break;
        case 0x0A: /* LDAX B */
            ACC = memory[BC];
            break;
        case 0x0F: /* RRC */
            FLAGS = (FLAGS & ~FLAG_CARRY) | (ACC & 1);
            ACC = ACC / 2 | ACC << 7;
            break;
        case 0x12: /* STAX D */
            memory[DE] = ACC;
            break;
        case 0x17: /* RAL */
            bit = ACC / 128;
            ACC = ACC * 2 + FLAGS % 2;
            FLAGS = (FLAGS & ~FLAG_CARRY) | bit;
            break;
        case 0x1A: /* LDAX D */
            ACC = memory[DE];
            break;
        case 0x1F: /* RAR */
            bit = ACC % 2;
            ACC = ACC / 2 + FLAGS % 2 * 128;
            FLAGS = (FLAGS & ~FLAG_CARRY) | bit;
            break;
        case 0x22: /* SHLD */
            address = fetch_word();
            memory[address] = regs[REG_L];
            memory[address + 1] = regs[REG_H];
            break;
        case 0x27: /* DAA */
            if ((FLAGS & FLAG_AUX) || ACC % 16 > 9) {
                ACC += 6;
                FLAGS |= FLAG_AUX;
            }
            if ((FLAGS & FLAG_CARRY) || ACC > 159) {
                ACC += 96;
                FLAGS |= FLAG_CARRY;
            }
            set_flags(FLAGS & FLAG_CARRY, ACC, FLAGS);
            break;
        case 0x2A: /* LHLD */
            address = fetch_word();
            regs[REG_L] = memory[address];
            regs[REG_H] = memory[address + 1];
            break;
        case 0x2F: /* CMA */
            ACC = ~ACC;
            break;
        case 0x32: /* STA */
            memory[fetch_word()] = ACC;
            break;
        case 0x37: /* STC */
            FLAGS |= FLAG_CARRY;
            break;
        case 0x3A: /* LDA */
            ACC = memory[fetch_word()];
            break;
        case 0x3F: /* CMC */
            FLAGS ^= FLAG_CARRY;
            break;
        case 0xC3: /* JMP */
            pc = memory + fetch_word();
            break;
        case 0xC9: /* RET */
            pc = memory + pop();
            break;
        case 0xCD: /* CALL */
            push(pc + 2 - memory);
            pc = memory + fetch_word();
            break;
        case 0xD3: /* OUT: write accumulator to console */
            pc++;
            write(1, &ACC, 1);
            break;
        case 0xDB: /* IN */
            input();
            break;
        case 0xE3: /* XTHL */
            address = SP;
            temp_low = memory[address];
            temp_high = memory[address + 1];
            memory[address + 1] = regs[REG_H];
            memory[address] = regs[REG_L];
            regs[REG_L] = temp_low;
            regs[REG_H] = temp_high;
            break;
        case 0xE9: /* PCHL */
            pc = memory + HL;
            break;
        case 0xEB: /* XCHG */
            temp_low = regs[REG_L];
            temp_high = regs[REG_H];
            regs[REG_L] = regs[REG_E];
            regs[REG_H] = regs[REG_D];
            regs[REG_E] = temp_low;
            regs[REG_D] = temp_high;
            break;
        case OP_ED: /* ED ED 02 = read sector, ED ED 03 = write sector */
            disk_operation();
            break;
        case 0xF9: /* SPHL */
            regs[REG_SP_LOW] = regs[REG_L];
            regs[REG_SP_HIGH] = regs[REG_H];
            break;
        default: /* NOP and unsupported opcodes */
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int op;

    if (argc == 2)
        import_file = fopen(argv[1], "rb+");
    boot_file = fopen("C", "rb+");
    system("stty raw -echo min 0");
    fread(memory, MEMORY_SIZE, 1, boot_file);
    fclose(boot_file);
    drive_b = fopen("B", "rb+");
    drive_a = fopen("A", "rb+");

    while ((op = *pc++) != OP_HLT)
        execute(op);

    system("stty cooked echo");
    if (import_file)
        fclose(import_file);
    fclose(drive_b); /* crashes when the drive files do not exist */
    fclose(drive_a);
}
