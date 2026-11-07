#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/*
 * OpenRISC 1000 (or1k) emulator that boots the Linux image stored in the
 * file "a".
 *
 * All machine state lives in one big int array.  Each special purpose
 * register (SPR) of the CPU sits at the array index equal to its SPR number,
 * so l.mfspr / l.mtspr simply index the array.  The general purpose
 * registers, the UART registers and physical RAM are placed in otherwise
 * unused parts of the same array.
 */

/* special purpose register numbers (= index into machine[]) */
#define SPR_VR          0           /* version register, 0 stops emulator */
#define SPR_SR          17          /* supervision register              */
#define SPR_EPCR        32          /* exception program counter         */
#define SPR_EEAR        48          /* exception effective address       */
#define SPR_ESR         64          /* exception saved SR                */
#define SPR_DTLB_MATCH  (5 << 9)    /* data TLB match regs (way 0)       */
#define SPR_ITLB_MATCH  (9 << 9)    /* instruction TLB match regs        */
#define TLB_TRANSLATE   128         /* translate regs follow match regs  */
#define TLB_SETS        64
#define SPR_PICMR       18432       /* interrupt controller mask         */
#define SPR_PICSR       18434       /* interrupt controller status       */
#define SPR_TTMR        (5 << 12)   /* tick timer mode register          */
#define SPR_TTCR        (SPR_TTMR + 1) /* tick timer count register      */

/* bits of SR */
#define SR_SM   1       /* supervisor mode               */
#define SR_TEE  2       /* tick timer exception enable   */
#define SR_IEE  4       /* interrupt exception enable    */
#define SR_DME  32      /* data MMU enable               */
#define SR_IME  64      /* instruction MMU enable        */
#define SR_F    512     /* compare flag                  */

/* bits of TTMR */
#define TTMR_IP 0x10000000u   /* interrupt pending */
#define TTMR_IE 0x20000000u   /* interrupt enable  */

/* exception vectors (address = vector << 8) */
#define EXC_PAGE_FAULT  3
#define EXC_TICK        5
#define EXC_INTERRUPT   8
#define EXC_DTLB_MISS   9
#define EXC_ITLB_MISS   10
#define EXC_SYSCALL     12

/* where the other pieces live inside machine[] */
#define UART_BASE       800
#define GPR_BASE        1234
#define RAM_OFFSET      0x40000     /* in bytes */
#define RAM_SIZE        0x10000000u /* maximum bytes read from the image */

/* 16550 UART register indices */
#define UART_DATA   0   /* RBR / THR */
#define UART_IER    1
#define UART_IIR    2
#define UART_LSR    5

int machine[0x7FFFFF];
int *spr  = machine;
int *gpr  = machine + GPR_BASE;
int *uart = machine + UART_BASE;
char *ram = (char *)machine + RAM_OFFSET;

#define SR      spr[SPR_SR]
#define EPCR    spr[SPR_EPCR]
#define EEAR    spr[SPR_EEAR]
#define ESR     spr[SPR_ESR]
#define PICMR   spr[SPR_PICMR]
#define PICSR   spr[SPR_PICSR]
#define TTMR    spr[SPR_TTMR]
#define TTCR    spr[SPR_TTCR]

int pc;                 /* program counter                          */
int in_delay_slot;      /* nonzero while executing a branch delay slot */
int reservation;        /* address remembered by word loads (l.lwa)  */
int data_addr;          /* last physical data address                */
int jump_target;
char uart_rx;           /* received byte waiting to be read, 0 = none */
struct termios saved_tty, raw_tty;

/* enter an exception: save state, switch to supervisor mode, jump to vector */
#define EXCEPTION(vector) {                                  \
        EPCR = pc - (in_delay_slot ? 4 : 0);                 \
        ESR = SR;                                            \
        SR = SR_SM;                                          \
        pc = (vector) << 8;                                  \
        return 0;                                            \
    }

/* execute one instruction, returns 0 if an exception was raised */
int step()
{
    int insn, imm, uimm, ra, rb, phys, ea, store_ea, is_store, *dest, *tlb;
    unsigned ura, opcode, op;
    int i, bit, cc;

    /* tick timer: count, and signal when the period is reached */
    TTMR |= !((TTMR ^ ++TTCR) << 4) && TTMR & TTMR_IE ? TTMR_IP : 0;

    /* instruction fetch through the instruction MMU */
    phys = pc;
    if (SR & SR_IME) {
        tlb = &spr[SPR_ITLB_MATCH | pc >> 13 & (TLB_SETS - 1)];
        if (!(*tlb & 1) || (*tlb ^ pc) >> 19) {
            EEAR = pc;
            EXCEPTION(EXC_ITLB_MISS)
        }
        tlb += TLB_TRANSLATE;
        phys = *tlb & ~8191 | pc & 0x1FFF;
    }
    insn = *(int *)(ram + phys);
    imm = (short)insn;              /* sign extended 16 bit immediate */
    uimm = imm & 0xFFFF;            /* zero extended 16 bit immediate */
    rb = gpr[insn >> 11 & 31];
    ura = ra = gpr[insn >> 16 & 31];

    /* the very first fetch finds empty memory: load the Linux image */
    if (!insn) {
        tcgetattr(0, &saved_tty);
        fread(ram, 1, RAM_SIZE, fopen("a", "r"));
        tcsetattr(0, TCSAFLUSH, &raw_tty);
        return RAM_SIZE;
    }

    /* poll the keyboard now and then; raise a UART receive interrupt */
    if (!(pc & 4095) && !uart_rx && read(0, &uart_rx, 1) > 0) {
        uart[UART_LSR] |= 1;
        if (uart[UART_IER] & 1)
            uart[UART_IIR] = PICSR = 4;
    }

    ea = ra + imm;
    opcode = (unsigned)insn >> 26;

    if ((SR & SR_TEE) && (TTMR & TTMR_IP))
        EXCEPTION(EXC_TICK)

    /* stores split their immediate into bits 25..21 and 10..0 */
    store_ea = ra + (short)(insn & 2047 | insn >> 10 & 0xF800);
    dest = opcode != 0x30 ? &gpr[insn >> 21 & 31] : spr + store_ea;

    if ((SR & SR_IEE) && (PICMR & PICSR))
        EXCEPTION(EXC_INTERRUPT)

    is_store = opcode > 0x30;

    /* l.jal and l.jalr save the return address in r9 */
    if (opcode == 0x01 || opcode == 0x12)
        gpr[9] = pc + 8;

    /* loads (0x1a..0x26) and stores (0x32..0x37) */
    if (opcode - 0x1a < 13 || opcode - 0x32 < 6) {
        ea = is_store ? store_ea : ea;
        data_addr = ea;
        if (SR & SR_DME) {
            tlb = &spr[SPR_DTLB_MATCH | ea >> 13 & (TLB_SETS - 1)];
            if (!(*tlb & 1) || (*tlb ^ ea) >> 19) {
                EEAR = ea;
                EXCEPTION(EXC_DTLB_MISS)
            }
            tlb += TLB_TRANSLATE;
            data_addr = *tlb & ~8191 | ea & 0x1FFF;
            /* user or supervisor read/write permission bits */
            if (~(*tlb >> ((SR & SR_SM) * 2 + 6)) & (is_store ? 2 : 1)) {
                EEAR = ea;
                EXCEPTION(EXC_PAGE_FAULT)
            }
        }

        if (data_addr < 0) {
            /* addresses 0x80000000 and up are the UART */
            int reg = data_addr & 7;
            if (is_store) {
                if (reg)
                    uart[reg] = rb;
                else {
                    write(0, &rb, 1);
                    if (uart[UART_IER] & 2)
                        uart[UART_IIR] = 2, PICSR = 4;
                }
            } else {
                *dest = uart[reg];
                if (!reg) {
                    *dest = uart_rx;
                    uart_rx = PICSR = 0;
                    uart[UART_LSR] &= ~1;
                }
                if (reg == UART_IIR) {
                    PICSR = 0;
                    *dest = (uart[UART_IIR] & 15) | 192;
                    uart[UART_IIR] = 1;
                }
            }
            opcode = 0x05;          /* done, treat the rest as l.nop */
        } else if (is_store) {
            /* memory holds big endian data in host ints: flip addresses */
            if (opcode > 0x35) {
                if (opcode & 1)
                    *(short *)(ram + (data_addr ^ 2)) = rb;  /* l.sh */
                else
                    ram[data_addr ^ 3] = rb;                 /* l.sb */
            } else if (opcode & 2) {                         /* l.swa */
                SR &= ~SR_F;
                if (!data_addr ^ reservation) {
                    SR |= SR_F;
                    *(int *)&ram[data_addr] = rb;
                }
            } else
                *(int *)&ram[data_addr] = rb;                /* l.sw */
            opcode = 0x05;
        }
    }

    pc += 4;

    /* l.sys / l.trap and l.rfe */
    if (pc && opcode - 0x08 < 2) {
        if (~opcode & 1) {
            if (!(insn >> 24 & 3))
                EXCEPTION(EXC_SYSCALL)
            else
                opcode = 0x05;
        } else {
            pc = EPCR;
            SR = ESR;
            return 1;
        }
    }

    /* l.j, l.jal, l.bnf, l.bf, l.jr, l.jalr */
    if (opcode == 0x00 || opcode == 0x01 || opcode == 0x03 ||
        opcode == 0x04 || opcode == 0x11 || opcode == 0x12) {
        int taken;
        jump_target = opcode > 0x10 ? rb : pc - 4 + (insn << 6 >> 4);
        if (opcode == 0x03)
            taken = !(SR & SR_F);
        else if (opcode == 0x04)
            taken = (SR & SR_F) != 0;
        else
            taken = 1;
        /* run the delay slot instruction, then jump unless it trapped */
        if (taken && (in_delay_slot = spr[SPR_VR], step()))
            pc = jump_target;
        return 1;
    }

    in_delay_slot = 0;

    if (opcode == 0x38) {
        /* register-register ALU instructions */
        op = imm & 0x3CF;
        if (~imm & 15) {
            if (op == 0)
                *dest = ra + rb;                    /* l.add   */
            else if (op < 3)
                *dest = ra - rb;                    /* l.sub   */
            else if (op == 3)
                *dest = ra & rb;                    /* l.and   */
            else if (op == 4)
                *dest = ra | rb;                    /* l.or    */
            else if (op == 5)
                *dest = ra ^ rb;                    /* l.xor   */
            else if (op < 9)
                *dest = ra << (rb & 31);            /* l.sll   */
            else if (op < 13)
                *dest = (short)ra;                  /* l.exths */
            else if (op < 15)
                *dest = (SR & SR_F) ? ra : rb;      /* l.cmov  */
            else if (op < 73)
                *dest = ura >> (rb & 31);           /* l.srl   */
            else if (op < 77)
                *dest = (char)ra;                   /* l.extbs */
            else if (op < 137)
                *dest = ra >> (rb & 31);            /* l.sra   */
            else if (op < 775)
                *dest = ra * rb;                    /* l.mul   */
            else if (op < 778)
                *dest = ra / rb;                    /* l.div   */
            else
                *dest = ura / ((unsigned)rb);       /* l.divu  */
        } else {
            /* l.ff1 / l.fl1: find first / last set bit (1 based) */
            for (i = 0; bit = op & 256 ? 31 - i : i,
                 *dest = ra & (1 << bit) ? bit + 1 : 0,
                 i < 32 && !*dest; i++)
                ;
        }
    } else if (opcode != 0x2f && opcode != 0x39) {
        if (opcode < 0x06)
            ;                                       /* l.nop, done */
        else if (opcode == 0x06)
            *dest = imm << 16;                      /* l.movhi */
        else if (opcode < 0x22) {                   /* l.lwz / l.lwa */
            reservation = opcode & 2 ? reservation : data_addr;
            *dest = *(int *)(ram + data_addr);
        } else if (opcode < 0x24)
            *dest = ram[data_addr ^ 3] & 255;       /* l.lbz   */
        else if (opcode == 0x24)
            *dest = ram[data_addr ^ 3];             /* l.lbs   */
        else if (opcode < 0x27)
            *dest = *(short *)(ram + (data_addr ^ 2)); /* l.lhz / l.lhs */
        else if (opcode == 0x27)
            *dest = ea;                             /* l.addi  */
        else if (opcode < 0x2a)
            *dest = ra & uimm;                      /* l.andi  */
        else if (opcode == 0x2a)
            *dest = ra | uimm;                      /* l.ori   */
        else if (opcode == 0x2b)
            *dest = ra ^ imm;                       /* l.xori  */
        else if (opcode == 0x2c)
            *dest = ra * imm;                       /* l.muli  */
        else if (opcode == 0x2d)
            *dest = spr[ra | uimm];                 /* l.mfspr */
        else if (opcode == 0x2e) {                  /* shift by immediate */
            int amount = insn & 31;
            switch (imm >> 6 & 3) {
            case 0:  *dest = ra << amount; break;   /* l.slli */
            case 2:  *dest = ra >> amount; break;   /* l.srai */
            default: *dest = ura >> amount; break;  /* l.srli */
            }
        } else
            *dest = rb;                             /* l.mtspr */
    } else {
        /* l.sfXX / l.sfXXi: set the compare flag */
        if (opcode & 2)
            rb = imm;
        cc = dest - gpr;        /* condition lives in the rD field */
        SR &= ~SR_F;
        if ((cc > 11 ? ra < rb :                    /* lts, les */
             cc > 9  ? ra > rb :                    /* gts, ges */
             cc > 3  ? ura < (unsigned)rb :         /* ltu, leu */
             cc > 1  ? ura > (unsigned)rb :         /* gtu, geu */
             cc & 1  ? ra != rb : 0)                /* ne       */
            || ((10281 & (1 << cc)) && ra == rb))   /* eq, geu, leu, ges, les */
            SR |= SR_F;
    }
    return spr[SPR_VR];
}

int main()
{
    /* run until the guest clears the version register (halt) */
    while (step(), step(), step(), spr[SPR_VR])
        ;
    tcsetattr(0, TCSAFLUSH, &saved_tty);
}
