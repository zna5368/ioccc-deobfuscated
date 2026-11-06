#include<fcntl.h>
#include<unistd.h>
#include<termios.h>
#include <sys/time.h>
#include  <sys/mman.h>

/*
 * PDP-7 emulator (CPU, EAE, clock, teletype, RB09 disk).
 *
 * The original was configured entirely with -D flags on the compiler
 * command line.  The defaults below are the ones the Makefile uses for
 * ./prog (BSD 2.9 running on the PDP-11/40 emulator written in PDP-7 code).
 * They can still be overridden with -D, e.g. for the v0 or v6 builds.
 */
#ifndef DISK_IMAGE
#define DISK_IMAGE "rk05.bsd.fs"    /* -DI: disk image file              */
#endif
#ifndef ADDRESS_SWITCHES
#define ADDRESS_SWITCHES 4096       /* -DA: where the boot tape is loaded */
#endif
#ifndef DATA_SWITCHES
#define DATA_SWITCHES 0173030       /* -DS: console data switches (OAS)   */
#endif
#ifndef DISK_MAP_MODE
#define DISK_MAP_MODE MAP_SHARED    /* -DW: MAP_SHARED keeps disk changes */
#endif
#ifndef TERMIOS_INIT
#define TERMIOS_INIT                /* -DT: initializer for raw termios   */
#endif
#ifndef ABORT_CHAR
#define ABORT_CHAR 5                /* -DX: Control-E quits (0 = never)   */
#endif
#ifndef PARITY_BIT
#define PARITY_BIT 128              /* -DY: "mark" parity bit on console  */
#endif
#ifndef DISK_TRACKS
#define DISK_TRACKS 270             /* -DV: tracks on the RB09 disk       */
#endif
#ifndef BOOT_TAPE
/*
 * -DP: RIM paper tape, three 6-bit characters per 18-bit word:
 *   707141 760000 707124 750000 707142 707104 210013
 *   707144 707121 610010 600000 002000 610000
 * The last word (JMP 010000) is executed directly to start the program.
 */
#define BOOT_TAPE "89!>@@89T=@@89\"89DQ@K89$89Q1@H0@@@P@1@@"
#endif

enum {
    MEMORY_WORDS  = 8192,       /* 8K words of core                       */
    ADDRESS_MASK  = 017777,     /* 13-bit address                         */
    INDIRECT_BIT  = 020000,     /* bit 13: indirect addressing / LAW      */
    WORD_MASK     = 0777777,    /* 18-bit word                            */
    SIGN_BIT      = 0400000,    /* bit 17                                 */
    CARRY_BIT     = 01000000,   /* bit 18: just past the top of a word    */
    EAE_BUSY      = 01000000,   /* in ir: EAE instruction still stepping  */

    SECTOR_WORDS  = 64,
    TRACK_SECTORS = 80,
    DISK_WORDS    = DISK_TRACKS * TRACK_SECTORS * SECTOR_WORDS,

    /* io_flags: device flags (any of them requests an interrupt) */
    CLOCK_FLAG    = 04000,
    PRINTER_FLAG  = 020000,
    KEYBOARD_FLAG = 040000,
    DISK_FLAG     = 01000000,

    /* enables */
    CLOCK_ON      = 02000,
    PI_ON         = 0400000,    /* program interrupt (interrupts enabled) */

    TRAP_MODE     = 0100000,    /* stored in location 0 on interrupt      */

    /* RB09 disk status */
    DISK_DONE     = 010000,
    DISK_BAD_ADDR = 0100000,
    DISK_ERROR    = 0400000,
    /* RB09 disk control */
    DISK_INT_ON   = 04000,
    DISK_GO       = 02000,
    DISK_WRITE    = 01000,
};

enum opcode {
    CAL, DAC, JMS, DZM, LAC, XOR, ADD, TAD,
    XCT, ISZ, AND, SAD, JMP, EAE, IOT, OPR
};

enum eae_operation {
    EAE_REGISTERS, EAE_MULTIPLY, EAE_NONE, EAE_DIVIDE,
    EAE_NORMALIZE, EAE_LONG_RIGHT, EAE_LONG_LEFT, EAE_AC_LEFT
};

enum result { NORMAL, EXECUTE_AGAIN, HALT };

struct timeval last_tick, now;
struct termios saved_mode, raw_mode = { TERMIOS_INIT };

char *tape = BOOT_TAPE;
char output_char, input_char;

int memory[MEMORY_WORDS];
int *operand;               /* memory word the instruction refers to      */
int *disk;                  /* memory mapped disk image                   */

int ac, link_bit, mq;       /* accumulator, link, multiplier-quotient     */
int pc = ADDRESS_SWITCHES;
int ir;                     /* instruction being executed                 */
int trap_mode;

int eae_step_count;         /* minus the remaining EAE steps              */
int eae_operand;            /* multiplier or divisor                      */
int eae_negative;           /* the result must be negated                 */

int io_flags;               /* pending device flags                       */
int enables;                /* PI_ON and CLOCK_ON                         */
int pending_enable;         /* turned on after the next instruction       */

int disk_status, disk_control;
int disk_address;           /* word offset on the disk                    */
int disk_memory_address;
int disk_word_count;

int poll_countdown;         /* instructions until the next I/O poll       */
int poll_interval;
int instructions_since_event = 1024;

/* One step of a multi-cycle EAE instruction. */
void eae_step(void)
{
    --ir;
    eae_step_count = -(ir % 64);
    if (!eae_step_count)
        ir -= EAE_BUSY;

    switch (ir / 64 & 7) {
    case EAE_MULTIPLY:
        if (mq & 1)
            ac += eae_operand;
        mq = mq >> 1 | (ac & 1) << 17;
        ac /= 2;
        if (!eae_step_count && eae_negative != link_bit) {
            ac ^= WORD_MASK;
            mq ^= WORD_MASK;
            link_bit = 0;
        }
        break;

    case EAE_DIVIDE:
        mq += mq;
        mq |= ac >= 0;
        if (eae_step_count) {
            ac *= 2;
            ac |= mq / CARRY_BIT;
            ac += ac < 0 ? eae_operand : -eae_operand;
        } else {
            if (ac < 0)
                ac += eae_operand;
            if (eae_negative)
                ac ^= WORD_MASK;
            if (link_bit != eae_negative)
                mq ^= WORD_MASK;
            link_bit = 0;
        }
        break;

    case EAE_NORMALIZE:
        if ((ac >> 17 ^ ac >> 16) & 1) {
            ir &= WORD_MASK;            /* stop early */
        } else {
            mq += mq;
            ac = (ac * 2 | mq / CARRY_BIT) & WORD_MASK;
        }
        break;

    case EAE_LONG_RIGHT:
        mq = mq >> 1 | (ac & 1) << 17;
        ac = ac >> 1 | link_bit << 17;
        break;

    case EAE_LONG_LEFT:
        mq = mq * 2 | link_bit;
        ac = (ac * 2 | mq / CARRY_BIT) & WORD_MASK;
        break;

    case EAE_AC_LEFT:
        ac = (ac * 2 | link_bit) & WORD_MASK;
        break;
    }
    mq &= WORD_MASK;
}

/* First cycle of an EAE instruction. */
void eae_setup(void)
{
    if (ir & 020000)
        link_bit = ac / SIGN_BIT;
    if (ir & 010000)
        mq = 0;
    eae_negative = (ir & 04000) && (ac & SIGN_BIT);
    if (ir & 02000)
        mq |= ac;
    else if (eae_negative)
        ac ^= WORD_MASK;
    if (ir & 01000)
        ac = 0;

    switch (ir / 64 & 7) {
    case EAE_REGISTERS:
        if (ir & 4)
            mq ^= WORD_MASK;
        if (ir & 2)
            ac |= mq;
        if (ir & 1)
            ac |= eae_step_count;
        return;

    case EAE_MULTIPLY:
        if (ir % 64)
            ir += EAE_BUSY;
        eae_operand = memory[pc++];
        if (eae_negative)
            mq ^= WORD_MASK;
        return;

    case EAE_DIVIDE:
        if (ir % 64)
            ir += EAE_BUSY;
        eae_operand = memory[pc++];
        if (eae_negative)
            mq ^= WORD_MASK;
        ac -= eae_operand;
        if (ac >= 0) {                  /* divide overflow */
            ir -= EAE_BUSY;
            link_bit = 1;
            ++pc;
        }
        return;

    default:
        if (ir % 64)
            ir += EAE_BUSY;
        return;
    }
}

/*
 * RB09 fixed disk (device 071).  The track and sector are exchanged with
 * the program in packed BCD.  The temporaries used for the conversion are
 * the same variables the IOT code checks afterwards, so they are passed in.
 */
int rb09_disk(int *skip_mask, int *enable_mask)
{
    int operation = ir / 16 & 3, place, number, bcd;

    switch (operation) {
    case 0:
        place = 1;
        if (ir & 1)
            disk_status = 0;
        if (ir & 2) {                   /* read address as BCD */
            number = disk_address / SECTOR_WORDS;
            number = number / TRACK_SECTORS * 100 + number % TRACK_SECTORS;
            bcd = 0;
            while (number) {
                bcd += number % 10 * place;
                number /= 10;
                place <<= 4;
            }
            ac |= bcd;
            *skip_mask = 0;
            *enable_mask = bcd;
        }
        if (ir & 4) {                   /* load address from BCD */
            bcd = ac;
            number = 0;
            while (bcd) {
                number += bcd % 16 * place;
                place *= 10;
                bcd >>= 4;
            }
            *enable_mask = number / 100;    /* track */
            *skip_mask = number % 100;      /* sector */
            if (DISK_TRACKS <= *enable_mask || TRACK_SECTORS <= *skip_mask)
                disk_status |= DISK_BAD_ADDR;
            else
                disk_address = (*enable_mask * TRACK_SECTORS + *skip_mask)
                               * SECTOR_WORDS;
        }
        break;

    case 1:
        if (ir & 1 && disk_status & (DISK_ERROR | DISK_DONE))
            ++pc;
        if (ir & 2)
            ac |= disk_status | disk_control;
        if (ir & 4)
            disk_word_count = -ac & 0177777;
        break;

    case 2:
        if (ir & 1)
            disk_status = disk_control = 0;
        if (ir & 2)
            disk_memory_address = ac;
        if (ir & 4) {
            disk_control ^= ac & (DISK_INT_ON | DISK_GO | DISK_WRITE);
            if (disk_control & DISK_GO) {
                disk_control ^= DISK_GO;
                disk_status |= DISK_DONE;
                do {
                    disk_memory_address &= ADDRESS_MASK;
                    if (disk_control & DISK_WRITE)
                        disk[disk_address] = memory[disk_memory_address];
                    else
                        memory[disk_memory_address] = disk[disk_address];
                    disk_address = (disk_address + 1) % DISK_WORDS;
                    ++disk_memory_address;
                } while (--disk_word_count);
            }
        }
        break;
    }

    disk_status &= ~DISK_ERROR;
    if (disk_status >= INDIRECT_BIT)    /* any error bit */
        disk_status |= DISK_ERROR;
    if (disk_status & (DISK_ERROR | DISK_DONE) && disk_control & DISK_INT_ON)
        io_flags |= DISK_FLAG;
    else
        io_flags &= ~DISK_FLAG;

    /* operation 3 is also handled as device 0 */
    return operation == 3 ? 0 : -1;
}

/* I/O transfer instruction; sets ir to -1 when it traps. */
void io_transfer(void)
{
    int device, skip_mask = 0, enable_mask = 0;

    if (trap_mode) {
        ir = -1;
        return;
    }
    if (ir & 8)
        ac = 0;
    device = ir / 64 % 64;

    if (device == 033 && ir & 2)        /* CAF: clear all flags */
        io_flags = enables = pending_enable = trap_mode = 0;
    if (device == 071)
        device = rb09_disk(&skip_mask, &enable_mask);

    switch (device) {
    case 0:                             /* interrupts and clock */
        if (ir & 1)
            skip_mask = CLOCK_FLAG;
        if (ir & 2)
            enable_mask = PI_ON;
        if (ir & 4) {
            enable_mask = CLOCK_ON;
            io_flags &= ~CLOCK_FLAG;
        }
        if (ir & 16)
            trap_mode = TRAP_MODE;
        break;

    case 3:                             /* keyboard */
        skip_mask = KEYBOARD_FLAG;
        if (ir & 2)
            ac |= input_char | PARITY_BIT;
        if (ir & 4)
            ac |= io_flags | enables;
        break;

    case 4:                             /* teleprinter */
        skip_mask = PRINTER_FLAG;
        if (ir & 4) {
            output_char = ac & ~PARITY_BIT;
            write(1, &output_char, 1);
            io_flags |= PRINTER_FLAG;
            skip_mask = 0;
        }
        break;
    }

    if (skip_mask) {
        if (ir & 1 && io_flags & skip_mask)
            ++pc;
        if (ir & 2)
            io_flags &= ~skip_mask;
    }
    if (enable_mask) {
        if (ir & 32)
            pending_enable = enable_mask;
        else
            enables &= ~enable_mask;
    }
}

/* Operate instruction (microcoded) or LAW. */
enum result operate(void)
{
    int skip = 0;

    if (ir & INDIRECT_BIT) {            /* LAW: load the instruction */
        ac = ir;
        return NORMAL;
    }
    if (ir & 0100)                      /* SMA */
        skip |= ac / SIGN_BIT % 2;
    if (ir & 0200)                      /* SZA */
        skip |= !ac;
    if (ir & 0400)                      /* SNL */
        skip |= link_bit;
    if (ir & 01000)                     /* reverse skip sense */
        skip = !skip;
    pc += skip;
    if (ir & 04000)                     /* CLL */
        link_bit = 0;
    if (ir & 010000)                    /* CLA */
        ac = 0;
    if (ir & 1)                         /* CMA */
        ac ^= WORD_MASK;
    if (ir & 2)                         /* CML */
        link_bit ^= 1;
    if (ir & 4)                         /* OAS */
        ac |= DATA_SWITCHES;
    if (ir & 010) {                     /* RAL, RTL with 02000 */
        ac |= link_bit * CARRY_BIT;
        ac += ac;
        if (ir & 02000)
            ac += ac;
        ac |= ac / CARRY_BIT / 2;
        link_bit = ac / CARRY_BIT;
    }
    if (ir & 020) {                     /* RAR, RTR with 02000 */
        ac |= link_bit * CARRY_BIT;
        ac |= ac * 2 % 64 * CARRY_BIT;
        ac /= 2;
        if (ir & 02000)
            ac /= 2;
        link_bit = ac / CARRY_BIT;
    }
    if (ir & 040) {                     /* HLT */
        if (trap_mode)
            ir = -1;
        else
            return HALT;
    }
    return NORMAL;
}

enum result execute_instruction(void)
{
    int opcode = ir / 040000, address = opcode ? ir & ADDRESS_MASK : 020;
    int old_ac;

    operand = memory + address;
    if (ir & INDIRECT_BIT) {
        /* locations 010-017 increment before an indirect reference */
        *operand += opcode < EAE && address >= 010 && address <= 017;
        operand = memory + (*operand & ADDRESS_MASK);
    }

    switch (opcode) {
    case CAL:                           /* CAL is JMS 20 */
    case JMS:
        *operand++ = pc | link_bit * SIGN_BIT;
        pc = operand - memory;
        break;
    case DAC:
        *operand = ac;
        break;
    case DZM:
        *operand = 0;
        break;
    case LAC:
        ac = *operand;
        break;
    case XOR:
        ac ^= *operand;
        break;
    case ADD:                           /* one's complement add */
        old_ac = ac;
        ac += *operand;
        ac += ac > WORD_MASK;           /* end around carry */
        link_bit |= ((ac ^ old_ac) & (ac ^ *operand)) / SIGN_BIT;
        break;
    case TAD:                           /* two's complement add */
        ac += *operand;
        link_bit = ac / CARRY_BIT;
        break;
    case XCT:
        ir = *operand;
        if (!trap_mode || ir / 040000 != XCT)
            return EXECUTE_AGAIN;
        ir = -ir;
        break;
    case ISZ:
        ++*operand;
        pc += !(*operand &= WORD_MASK);
        break;
    case AND:
        ac &= *operand;
        break;
    case SAD:
        if (ac != *operand)
            ++pc;
        break;
    case JMP:
        pc = operand - memory;
        break;
    case EAE:
        eae_setup();
        break;
    case IOT:
        io_transfer();
        break;
    case OPR:
        return operate();
    }
    return NORMAL;
}

/*
 * Poll the keyboard and the 60 Hz clock.  The number of instructions
 * between polls is adjusted so that there are about four polls per event.
 * Returns 0 when the abort character is typed.
 */
int poll_devices(void)
{
    if (!(io_flags & KEYBOARD_FLAG) && read(0, &input_char, 1) > 0) {
        if (ABORT_CHAR && input_char == ABORT_CHAR)
            return 0;
        poll_countdown = 0;
        io_flags |= KEYBOARD_FLAG;
    }

    gettimeofday(&now, 0);
    now.tv_usec /= 16667;               /* sixtieths of a second */
    if (now.tv_sec > last_tick.tv_sec || last_tick.tv_usec < now.tv_usec) {
        last_tick = now;
        if (enables & CLOCK_ON) {
            memory[7] = (memory[7] + 1) & WORD_MASK;
            if (!memory[7])
                io_flags |= CLOCK_FLAG;
        }
        poll_countdown = 0;
    }

    if (!poll_countdown) {
        poll_interval = instructions_since_event / 4;
        instructions_since_event = 4;
    }
    instructions_since_event += poll_countdown = poll_interval;
    return 1;
}

int main()
{
    int trapped;

    disk = mmap(NULL, 4 * DISK_WORDS, PROT_READ | PROT_WRITE, DISK_MAP_MODE,
                open(DISK_IMAGE, O_RDWR), 0);

    /* load the RIM paper tape; the last word is executed first */
    while (*tape) {
        ir = tape[0] % 64 << 12 | tape[1] % 64 << 6 | tape[2] % 64;
        tape += 3;
        memory[pc++] = ir;
    }

    tcgetattr(0, &saved_mode);
    tcsetattr(0, TCSAFLUSH, &raw_mode);
    fcntl(0, F_SETFL, 4);               /* O_NONBLOCK on BSD */

    for (;; pc &= ADDRESS_MASK) {
        if (ir & EAE_BUSY) {
            eae_step();
        } else {
            enum result result = execute_instruction();
            if (result == EXECUTE_AGAIN)
                continue;
            if (result == HALT)
                break;
        }
        link_bit &= 1;

        if (ir >= EAE_BUSY)             /* EAE instruction not finished */
            continue;

        ac &= WORD_MASK;
        pc &= ADDRESS_MASK;
        if (!poll_countdown--) {
            if (!poll_devices())
                break;
        }

        trapped = ir == -1;
        if (trapped || (enables & PI_ON && io_flags)) {
            memory[0] = pc | link_bit * SIGN_BIT | trap_mode;
            enables &= ~PI_ON;
            pc = trapped ? 2 : 1;
            trap_mode = 0;
        }
        enables |= pending_enable;
        pending_enable = 0;
        ir = memory[pc++];
    }
    tcsetattr(0, TCSADRAIN, &saved_mode);
}
