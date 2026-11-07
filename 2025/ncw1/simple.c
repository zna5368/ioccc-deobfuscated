/*
 * Terminal Game Boy (DMG) emulator.
 *
 * Usage: ./simple 3< rom_file
 *
 * The ROM is read from file descriptor 3, keys come from stdin (which
 * try.sh puts in raw, non-blocking mode) and the screen is drawn on stdout
 * using 256-colour ANSI escapes and the upper-half block character, so
 * every terminal cell shows two Game Boy pixels.
 */
#include <iso646.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Keyboard map in the order Right, Left, Up, Down, A, B, Select, Start
 * (may be overridden with -DK=...) */
#ifndef K
#define K "dawskj \r"
#endif

/* Register values after the boot ROM in the order B, C, D, E, H, L, F, A
 * (may be overridden with -DR=...) */
#ifndef R
#define R { 0x00, 0x13, 0x00, 0xD8, 0x01, 0x4D, 0xB0, 0x01 }
#endif

typedef unsigned char u8;

/* 8-bit register indexes as used in opcodes; 6 means the byte at (HL) */
enum { REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_F, REG_A };
#define OPERAND_HL_MEM 6

/* 16-bit register pair indexes as used in opcodes */
enum { PAIR_BC, PAIR_DE, PAIR_HL, PAIR_SP_OR_AF };

/* ALU operations as used in opcodes */
enum { ALU_ADD, ALU_ADC, ALU_SUB, ALU_SBC, ALU_AND, ALU_XOR, ALU_OR, ALU_CP };

/* Flag bits (the half-carry flag is not emulated) */
#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_C 0x10

/* Memory map */
#define ROM_SIZE   0x8000
#define VRAM       0x8000
#define TILES_8800 0x9000   /* base for signed tile numbers */
#define TILE_MAP   0x9800
#define OAM        0xFE00
#define IO_JOYP    0xFF00
#define IO_DIV     0xFF04
#define IO_IF      0xFF0F
#define IO_LCDC    0xFF40
#define IO_SCY     0xFF42
#define IO_SCX     0xFF43
#define IO_LY      0xFF44
#define IO_DMA     0xFF46
#define IO_IE      0xFFFF

/* Video timing */
#define CYCLES_PER_STEP 8
#define CYCLES_PER_LINE 456
#define VBLANK_LINE     144
#define LAST_LINE       153
#define SCREEN_W        160
#define SCREEN_H        144
#define KEY_HOLD_FRAMES 9

u8 ime;            /* interrupt master enable */
u8 halted;         /* CPU executed HALT */
u8 ei_pending;     /* EI takes effect after the next instruction */
u8 memory[1 << 16];
u8 regs[] = R;
u8 buttons = 255;  /* joypad state, a cleared bit means pressed */
unsigned short sp = 0xFFFE;
unsigned short pc = 0x0100;

#define FLAGS regs[REG_F]
#define ACC   regs[REG_A]

int read_byte(int addr)
{
    int value = memory[addr];
    int keys;

    if (addr != IO_JOYP)
        return value;

    /* bit 4 set means the direction keys are not selected, so report the
     * action buttons from the high nibble instead */
    keys = (value & 0x10) ? buttons >> 4 : buttons;
    return (keys & 0x0F) | (value & 0xF0);
}

void write_byte(int addr, int value)
{
    int i;

    if (addr < 0x8000)     /* the ROM is read only */
        return;

    /* writing LY resets it to zero */
    memory[addr] = (addr != IO_LY) ? value : 0;

    /* OAM DMA: copy 160 bytes from value*0x100 to the sprite table */
    if (addr == IO_DMA)
        for (i = 160; i--; )
            memory[OAM | i] = memory[value << 8 | i];
}

int fetch8(void)
{
    return read_byte(pc++);
}

int fetch16(void)
{
    int low = fetch8();
    int high = fetch8();
    return low | high << 8;
}

int pop16(void)
{
    int low = read_byte(sp++);
    int high = read_byte(sp++);
    return low | high << 8;
}

void push16(int value)
{
    write_byte(--sp, value >> 8);
    write_byte(--sp, value);
}

int get_pair(int pair, int use_af)
{
    if (pair < PAIR_SP_OR_AF)
        return regs[pair * 2] << 8 | regs[pair * 2 + 1];
    if (use_af)
        return ACC << 8 | FLAGS;
    return sp;
}

void set_pair(int pair, int value, int use_af)
{
    if (pair < PAIR_SP_OR_AF) {
        regs[pair * 2] = value >> 8;
        regs[pair * 2 + 1] = value;
    } else if (use_af) {
        ACC = value >> 8;
        FLAGS = value & 0xF0;
    } else {
        sp = value;
    }
}

int get_reg(int index)
{
    if (index != OPERAND_HL_MEM)
        return regs[index];
    return read_byte(get_pair(PAIR_HL, 0));
}

void set_reg(int index, int value)
{
    if (index != OPERAND_HL_MEM)
        regs[index] = value;
    else
        write_byte(get_pair(PAIR_HL, 0), value);
}

void set_flags(int zero, int subtract, int carry)
{
    FLAGS = (zero ? FLAG_Z : 0) | (subtract ? FLAG_N : 0) | (carry ? FLAG_C : 0);
}

/* conditions NZ, Z, NC, C */
int condition_met(int cc)
{
    switch (cc) {
    case 0:  return !(FLAGS & FLAG_Z);
    case 1:  return FLAGS & FLAG_Z;
    case 2:  return !(FLAGS & FLAG_C);
    default: return FLAGS & FLAG_C;
    }
}

/* ADD, ADC, SUB, SBC, AND, XOR, OR, CP of the accumulator with value */
void alu(int op, int value)
{
    int carry_in = (op == ALU_ADC || op == ALU_SBC) ? (FLAGS >> 4 & 1) : 0;
    int is_subtract = op > ALU_ADC;
    int is_logical = op >= ALU_AND && op != ALU_CP;
    int result;

    if (op == ALU_AND)
        result = ACC & value;
    else if (op == ALU_XOR)
        result = ACC ^ value;
    else if (op == ALU_OR)
        result = ACC | value;
    else if (is_subtract)
        result = ACC - value - carry_in;
    else
        result = ACC + value + carry_in;

    FLAGS = (!(result & 255)) << 7
          | (!is_logical && is_subtract) << 6
          | (!is_logical && (is_subtract ? result < 0 : result > 255)) << 4;

    if (op != ALU_CP)
        ACC = result;
}

/* CB prefixed instructions: rotates, shifts, BIT, RES and SET */
void cb_instruction(void)
{
    int opcode = fetch8();
    int group = opcode >> 6;
    int bit = opcode >> 3 & 7;     /* bit number or shift operation */
    int target = opcode & 7;
    int value = get_reg(target);
    int result;
    int carry;

    if (group == 1) {              /* BIT b,r */
        set_flags(!(value & (1 << bit)), 0, FLAGS & FLAG_C);
        return;
    }

    if (group == 2) {              /* RES b,r */
        result = value & ~(1 << bit);
    } else if (group == 3) {       /* SET b,r */
        result = value | 1 << bit;
    } else {
        /* operations: 0 RLC, 1 RRC, 2 RL, 3 RR, 4 SLA, 5 SRA, 6 SWAP, 7 SRL */
        int shift_right = bit & 1;

        if (bit == 6)
            carry = 0;
        else
            carry = shift_right ? value & 1 : value >> 7;

        if (bit == 6)
            result = value << 4 | value >> 4;
        else
            result = shift_right ? value >> 1 : value << 1;

        if (bit < 4)               /* rotate in old carry (RL/RR) or the bit shifted out */
            result |= ((bit & 2) ? !!(FLAGS & FLAG_C) : carry) << (shift_right * 7);
        if (bit == 5)              /* SRA keeps the sign bit */
            result |= value & 0x80;

        set_flags(!(u8)result, 0, carry);
    }
    set_reg(target, result);
}

/* execute one instruction */
void cpu_step(void)
{
    int opcode, group, y, z, pair, odd;
    int addr, carry, offset, result;

    if (halted) {
        if (memory[IO_IE] & memory[IO_IF])
            halted = 0;
        return;
    }
    if (ei_pending) {
        ime = 1;
        ei_pending = 0;
    }

    opcode = fetch8();
    group = opcode >> 6;
    y = opcode >> 3 & 7;
    z = opcode & 7;
    pair = y >> 1;
    odd = y & 1;

    if (group == 1) {
        if (opcode == 0x76)                     /* HALT */
            halted = 1;
        else                                    /* LD r,r' */
            set_reg(y, get_reg(z));
        return;
    }

    if (group == 2) {                           /* ALU A,r */
        alu(y, get_reg(z));
        return;
    }

    if (group == 0) {
        switch (z) {
        case 0:
            if (y == 0 || y == 2)               /* NOP, STOP */
                return;
            if (y == 1) {                       /* LD (a16),SP */
                addr = fetch16();
                write_byte(addr, sp);
                write_byte(addr + 1, sp >> 8);
                return;
            }
            offset = (signed char)fetch8();     /* JR e / JR cc,e */
            if (y == 3 || condition_met(y - 4))
                pc += offset;
            return;

        case 1:
            if (!odd) {                         /* LD rr,d16 */
                set_pair(pair, fetch16(), 0);
            } else {                            /* ADD HL,rr */
                result = get_pair(PAIR_HL, 0) + get_pair(pair, 0);
                set_flags(FLAGS & FLAG_Z, 0, result > 0xFFFF);
                set_pair(PAIR_HL, result, 0);
            }
            return;

        case 2:                                 /* LD (rr),A / LD A,(rr) with HL+ and HL- */
            addr = pair > 1 ? get_pair(PAIR_HL, 0) : get_pair(pair, 0);
            if (odd)
                ACC = read_byte(addr);
            else
                write_byte(addr, ACC);
            if (pair == 2)
                set_pair(PAIR_HL, get_pair(PAIR_HL, 0) + 1, 0);
            else if (pair == 3)
                set_pair(PAIR_HL, get_pair(PAIR_HL, 0) - 1, 0);
            return;

        case 3:                                 /* INC rr / DEC rr */
            set_pair(pair, get_pair(pair, 0) + (odd ? -1 : 1), 0);
            return;

        case 4:                                 /* INC r */
        case 5: {                               /* DEC r */
            u8 value = get_reg(y) + (z == 4 ? 1 : -1);
            set_reg(y, value);
            set_flags(!value, z == 5, FLAGS & FLAG_C);
            return;
        }

        case 6:                                 /* LD r,d8 */
            set_reg(y, fetch8());
            return;

        default:
            if (y < 4) {                        /* RLCA, RRCA, RLA, RRA */
                carry = (odd ? ACC : ACC >> 7) & 1;
                ACC = odd ? ACC >> 1 : ACC << 1;
                ACC |= ((y & 2) ? !!(FLAGS & FLAG_C) : carry) << (odd * 7);
                set_flags(0, 0, carry);
            } else if (y == 4) {                /* DAA (no half carry) */
                if (FLAGS & FLAG_N) {
                    if (FLAGS & FLAG_C)
                        ACC -= 0x60;
                } else {
                    if ((FLAGS & FLAG_C) | (ACC > 0x99)) {
                        ACC += 0x60;
                        FLAGS |= FLAG_C;
                    }
                    if ((ACC & 15) > 9)
                        ACC += 6;
                }
                set_flags(!ACC, FLAGS & FLAG_N, FLAGS & FLAG_C);
            } else if (y == 5) {                /* CPL */
                ACC = ~ACC;
                set_flags(FLAGS & FLAG_Z, 1, FLAGS & FLAG_C);
            } else {                            /* SCF / CCF */
                set_flags(FLAGS & FLAG_Z, 0, odd ? !(FLAGS & FLAG_C) : 1);
            }
            return;
        }
    }

    /* group == 3 */
    switch (z) {
    case 0:
        if (y < 4) {                            /* RET cc */
            if (condition_met(y))
                pc = pop16();
            return;
        }
        if (y == 4) {                           /* LDH (a8),A */
            write_byte(IO_JOYP | fetch8(), ACC);
            return;
        }
        if (y == 6) {                           /* LDH A,(a8) */
            ACC = read_byte(IO_JOYP | fetch8());
            return;
        }
        offset = (signed char)fetch8();         /* ADD SP,e / LD HL,SP+e */
        result = sp + offset;
        set_flags(0, 0, (sp ^ offset ^ result) & 0x100);
        if (y == 5)
            sp = result;
        else
            set_pair(PAIR_HL, result, 0);
        return;

    case 1:
        if (!odd) {                             /* POP rr (AF instead of SP) */
            set_pair(pair, pop16(), 1);
            return;
        }
        if (pair & 2) {
            if (pair & 1)                       /* LD SP,HL */
                sp = get_pair(PAIR_HL, 0);
            else                                /* JP HL */
                pc = get_pair(PAIR_HL, 0);
            return;
        }
        pc = pop16();                           /* RET / RETI */
        ime |= pair;
        return;

    case 2:
        if (y < 4 || odd)
            addr = fetch16();
        else
            addr = IO_JOYP | regs[REG_C];
        if (y < 4) {                            /* JP cc,a16 */
            if (condition_met(y))
                pc = addr;
        } else if (y & 2) {                     /* LD A,(C) / LD A,(a16) */
            ACC = read_byte(addr);
        } else {                                /* LD (C),A / LD (a16),A */
            write_byte(addr, ACC);
        }
        return;

    case 3:
        if (y == 0) {                           /* JP a16 */
            pc = fetch16();
            return;
        }
        if (y == 6) {                           /* DI */
            ime = ei_pending = 0;
            return;
        }
        if (y == 7) {                           /* EI */
            ei_pending = 1;
            return;
        }
        cb_instruction();                       /* CB prefix */
        return;

    case 4:
        if (y < 4) {                            /* CALL cc,a16 */
            addr = fetch16();
            if (condition_met(y)) {
                push16(pc);
                pc = addr;
            }
        }
        return;

    case 5:
        if (!odd) {                             /* PUSH rr (AF instead of SP) */
            push16(get_pair(pair, 1));
        } else if (pair == 0) {                 /* CALL a16 */
            addr = fetch16();
            push16(pc);
            pc = addr;
        }
        return;

    case 6:                                     /* ALU A,d8 */
        alu(y, fetch8());
        return;

    default:                                    /* RST */
        push16(pc);
        pc = y * 8;
        return;
    }
}

/* 2-bit colour of one bitplane pair at pixel column x (0 = leftmost) */
int tile_pixel(u8 *row_data, int x)
{
    return (row_data[0] >> (7 - x) & 1) | (row_data[1] >> (7 - x) & 1) * 2;
}

/* terminal colour (grey ramp) of screen pixel (x, y) */
int pixel_color(int x, int y)
{
    int lcdc = memory[IO_LCDC];
    int color = 0;
    int px, py, tile, sprite_color, k;
    u8 *data;

    if (lcdc & 1) {                             /* background */
        px = x + memory[IO_SCX];
        py = y + memory[IO_SCY];
        tile = memory[TILE_MAP + (py / 8 & 31) * 32 + (px / 8 & 31)];
        if (lcdc & 16)
            data = memory + VRAM + tile * 16;
        else
            data = memory + TILES_8800 + (signed char)tile * 16;
        data += (py & 7) * 2;
        color = tile_pixel(data, px & 7);
    }

    if (lcdc & 2) {                             /* 8x8 sprites, no flipping */
        for (k = 0; k < 40; k++) {
            u8 *sprite = memory + OAM + k * 4;
            py = y - sprite[0] + 16;
            px = x - sprite[1] + 8;
            if ((unsigned)py < 8 && (unsigned)px < 8) {
                data = memory + VRAM + sprite[2] * 16 + py * 2;
                sprite_color = tile_pixel(data, px);
                if (sprite_color)
                    color = sprite_color;
            }
        }
    }

    return 255 - 7 * (lcdc & 128 ? color : 0);
}

int main()
{
    static u8 hold[9];          /* frames each key stays pressed */
    u8 *ly = memory + IO_LY;
    u8 c;
    char *keys = K, *match, *out, screen[1 << 20];
    int cycles = 0, frame_drawn = 0, i, row, col;

    read(3, memory, ROM_SIZE);
    memory[IO_JOYP] = 0xCF;
    memory[IO_LCDC] = 0x91;
    memory[IO_IE] = 1;          /* enable the V-blank interrupt */

    for (;;) {
        cpu_step();
        memory[IO_DIV]++;

        cycles += CYCLES_PER_STEP;
        while (cycles >= CYCLES_PER_LINE) {
            cycles -= CYCLES_PER_LINE;
            if (++*ly == VBLANK_LINE) {
                memory[IO_IF] |= 1;
                frame_drawn = 0;
            } else if (*ly > LAST_LINE) {
                *ly = frame_drawn = 0;
            }
        }

        if (*ly == VBLANK_LINE && !frame_drawn) {
            frame_drawn = 1;

            /* read keys, each one is held down for a few frames */
            while (read(0, &c, 1) > 0) {
                match = strchr(keys, c);
                if (match)
                    hold[match - keys] = KEY_HOLD_FRAMES;
            }
            buttons = 255;
            for (i = 0; i < 8; i++) {
                if (hold[i]) {
                    hold[i]--;
                    buttons ^= 1 << i;
                }
            }

            /* draw two pixel rows per text line using an upper half block */
            out = strcpy(screen, "\x1b[H") + 3;
            for (row = 0; row < SCREEN_H; row += 2) {
                for (col = 0; col < SCREEN_W; col++)
                    out += sprintf(out, "\x1b[38;5;%dm\x1b[48;5;%dm▀",
                                   pixel_color(col, row), pixel_color(col, row + 1));
                *out++ = '\n';
            }
            write(1, screen, out - screen);
            usleep(14000);
        }

        /* V-blank interrupt */
        if (ime && (memory[IO_IE] & memory[IO_IF] & 1)) {
            ime = 0;
            memory[IO_IF] &= ~1;
            push16(pc);
            pc = 0x40;
        }
    }
}
