#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*
 * Obfuscated Gaming Virtual Machine (X11 version).
 *
 * A 32-bit little-endian virtual machine with 64 MiB of word addressed RAM.
 * The bytecode file named on the command line is loaded at address 0,
 * followed by the command line arguments (last one first, each NUL
 * terminated).  Then instructions are fetched and executed until a halt.
 *
 * Memory layout:
 *   word 0   program counter (a word index, always advanced by 4 words)
 *   word 1   on start: number of bytes loaded; afterwards a scratch
 *            register that also receives keyboard/mouse event data
 *
 * Every instruction is four 32-bit words:
 *
 *   word 0  control word
 *             bits  0..6   addressing mode of operand 1
 *             bits  7..13  addressing mode of operand 2
 *             bits 14..20  addressing mode of the destination
 *             bits 21..22  store size: 0 = 32, 1 = 24, 2 = 16, 3 = 8 bits
 *             bits 23..31  opcode
 *   word 1  operand 1
 *   word 2  operand 2
 *   word 3  destination
 *
 * Addressing modes for operands:
 *   0       immediate: the value itself
 *   1       [value]: 32-bit read from byte address value (may be unaligned)
 *   2       register: memory word number value
 *   3..127  [register + (mode - 65)]: 32-bit read from the byte address
 *           held in word number value plus a small signed displacement
 *
 * The destination uses the same modes, except that mode 0 means
 * "conditional jump": if the result is nonzero the program counter is
 * moved by the destination word (in words).
 */

#define MEM_WORDS       (1 << 24)
#define ADDR_MASK       (MEM_WORDS - 1)     /* wraps word indexes */

#define REG_PC          0
#define REG_SCRATCH     1

#define INSN_WORDS      4
#define MODE_IMMEDIATE  0
#define MODE_ABSOLUTE   1
#define MODE_REGISTER   2
#define DISP_BIAS       65                  /* mode 65 means displacement 0 */

#define NUM_FILES       8
#define NAME_LEN        32
#define MAX_WINDOW_SIZE 4096

/* system call opcodes */
enum {
    SYS_HALT = 0,
    SYS_FILE_CHAR = 1,      /* read (fd < 0) or write a character */
    SYS_TIME_MS = 2,
    SYS_SLEEP_MS = 3,
    SYS_SET_NAME_CHAR = 4,
    SYS_OPEN_CLOSE = 5,
    SYS_SEEK_TELL = 6,
    SYS_OPEN_WINDOW = 7,
    SYS_POLL_EVENT = 8,
    SYS_DRAW_FRAME = 9,
    SYS_NOP = 10
};

/* event codes returned by SYS_POLL_EVENT */
enum {
    EVENT_KEY_PRESS = 0,
    EVENT_KEY_RELEASE = 1,
    EVENT_NONE = 2,
    EVENT_QUIT = 3,
    EVENT_BUTTON_PRESS = 4,
    EVENT_BUTTON_RELEASE = 5,
    EVENT_MOTION = 6
};

/* arithmetic opcodes: a group base selects the operand type */
enum {
    GROUP_UINT = 32,
    GROUP_INT = 48,
    GROUP_USHORT = 64,
    GROUP_SHORT = 80,
    GROUP_UCHAR = 96,
    GROUP_SCHAR = 112
};

/* the operator is the opcode minus its group base */
#define ARITHMETIC(base, type) \
    case base +  0: result = (type)arg1 +  (type)arg2; break; \
    case base +  1: result = (type)arg1 -  (type)arg2; break; \
    case base +  2: result = (type)arg1 |  (type)arg2; break; \
    case base +  3: result = (type)arg1 ^  (type)arg2; break; \
    case base +  4: result = (type)arg1 &  (type)arg2; break; \
    case base +  5: result = (type)arg1 *  (type)arg2; break; \
    case base +  6: result = (type)arg1 /  (type)arg2; break; \
    case base +  7: result = (type)arg1 %  (type)arg2; break; \
    case base +  8: result = (type)arg1 << (type)arg2; break; \
    case base +  9: result = (type)arg1 >> (type)arg2; break; \
    case base + 10: result = (type)arg1 == (type)arg2; break; \
    case base + 11: result = (type)arg1 != (type)arg2; break; \
    case base + 12: result = (type)arg1 <  (type)arg2; break; \
    case base + 13: result = (type)arg1 <= (type)arg2; break; \
    case base + 14: result = (type)arg1 >  (type)arg2; break; \
    case base + 15: result = (type)arg1 >= (type)arg2; break;

static unsigned *mem;

/* read 32 bits from any byte address, little-endian, possibly unaligned */
static unsigned read_bytes(unsigned byte_addr)
{
    unsigned shift = byte_addr % 4 * 8;
    unsigned word = byte_addr / 4 & ADDR_MASK;

    /* "* 2 << (31 - shift)" avoids an undefined shift by 32 when aligned */
    return mem[word] >> shift | mem[word + 1] * 2 << (31 - shift);
}

static unsigned fetch_operand(unsigned mode, unsigned value)
{
    if (mode == MODE_IMMEDIATE)
        return value;
    if (mode != MODE_ABSOLUTE)
        value = mem[value & ADDR_MASK] + (mode > 2 ? mode - DISP_BIAS : 0);
    if (mode != MODE_REGISTER)
        value = read_bytes(value);
    return value;
}

/* set the bits of a memory word selected by mask to those of value */
static void merge_bits(unsigned word, unsigned mask, unsigned value)
{
    mem[word] = (mem[word] | mask) ^ (~value & mask);
}

static void store_result(unsigned mode, unsigned dest, unsigned size_shift,
                         unsigned value)
{
    unsigned mask = 0xFFFFFFFFu >> size_shift;
    unsigned addr;
    int shift = 0;

    addr = mode > 2 ? mem[dest & ADDR_MASK] + mode - DISP_BIAS : dest;
    if (mode != MODE_REGISTER) {
        shift = addr % 4 * 8;
        addr = addr / 4;
    }
    addr &= ADDR_MASK;

    merge_bits(addr, mask << shift, value << shift);
    addr++;
    if (shift)
        merge_bits(addr, mask >> (32 - shift), value >> (32 - shift));
}

/* keep only letters, digits and dots in file names */
static char safe_name_char(unsigned ch)
{
    if (ch == 0 || ch - '0' <= 9 || (ch | 32) - 'a' <= 25 || ch == '.')
        return ch;
    return '_';
}

int main(int argc, char **argv)
{
    FILE *program = argc > 1 ? fopen(argv[1], "rb") : NULL;
    FILE *files[NUM_FILES] = { stdin, stdout, stderr };
    unsigned frame_width = 0, frame_height = 0;
    unsigned insn, arg2, dest, result, pc, byte_count, ch;
    int arg1, arg_index, opcode;
    unsigned str_pos;
    struct timeval tv;
    char file_name[NAME_LEN + 1] = { 0 };
    Window window = 0;
    GC gc = 0;
    Display *display = NULL;
    XEvent event;
    XImage *image = NULL;
    Atom wm_delete = 0;

    mem = calloc(MEM_WORDS + 3, 4);
    if (!mem || !program)
        return 1;

    /* load the program file, then the arguments from last to first */
    arg_index = argc - 1;
    str_pos = 0;
    for (byte_count = 0; byte_count <= ADDR_MASK; byte_count++) {
        if ((ch = fgetc(program)) >= 256) {
            ch = argv[arg_index][str_pos++] & 255;
            if (ch == 0) {
                /* end of this argument: move on to the previous one */
                str_pos = 0;
                if (--arg_index == 0)
                    break;
            }
        }
        mem[byte_count / 4] |= ch << byte_count % 4 * 8;
    }
    fclose(program);
    mem[REG_SCRATCH] = byte_count;

    for (;;) {
        pc = mem[REG_PC] & ADDR_MASK;
        insn = mem[pc];
        mem[REG_PC] += INSN_WORDS;

        arg1 = fetch_operand(insn & 127, mem[pc + 1]);
        arg2 = fetch_operand(insn >> 7 & 127, mem[pc + 2]);
        dest = mem[pc + 3];
        opcode = insn >> 23;
        result = arg2;

        switch (opcode) {
        default:
            /* SYS_HALT, or an unknown opcode */
            if (image)
                image->data = NULL;
            return insn >> 14 ? 1 : arg1;

        case SYS_FILE_CHAR:
            if (!files[arg1 & 7])
                result = -1;
            else if (arg1 < 0)
                result = fgetc(files[arg1 & 7]);
            else
                result = fputc(arg2, files[arg1 & 7]);
            break;

        case SYS_TIME_MS:
            gettimeofday(&tv, 0);
            result = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            break;

        case SYS_SLEEP_MS:
            tv.tv_sec = 0;
            tv.tv_usec = arg2 * 1000;
            select(0, 0, 0, 0, &tv);
            break;

        case SYS_SET_NAME_CHAR:
            file_name[arg1 & 31] = safe_name_char(arg2);
            break;

        case SYS_OPEN_CLOSE:
            /* close an open handle, or open file_name (arg2: 1 = write) */
            if (files[arg1 & 7]) {
                fclose(files[arg1 & 7]);
                files[arg1 & 7] = NULL;
            } else {
                files[arg1 & 7] = fopen(file_name, arg2 ? "wb" : "rb");
            }
            result = !files[arg1 & 7];
            break;

        case SYS_SEEK_TELL:
            /* arg1: bits 0..2 handle, sign bit = tell, others = whence */
            if (!files[arg1 & 7])
                result = -1;
            else if (arg1 < 0)
                result = ftell(files[arg1 & 7]);
            else
                result = fseek(files[arg1 & 7], (int)arg2, arg1 / 8);
            break;

        case SYS_OPEN_WINDOW:
            /* arg1 = width, arg2 = height, file_name = title */
            if (!image && arg1 >= 1 && arg1 <= MAX_WINDOW_SIZE &&
                arg2 >= 1 && arg2 <= MAX_WINDOW_SIZE) {
                XSizeHints hints;

                hints.flags = PMinSize | PMaxSize;
                hints.min_width = hints.max_width = arg1;
                hints.min_height = hints.max_height = arg2;
                display = XOpenDisplay(0);
                gc = XDefaultGC(display, 0);
                window = XCreateSimpleWindow(display, RootWindow(display, 0),
                                             0, 0, arg1, arg2, 1, 0, 0);
                XSetStandardProperties(display, window, file_name, 0, 0, 0, 0,
                                       &hints);
                XSelectInput(display, window,
                             KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | ButtonReleaseMask |
                             PointerMotionMask);
                wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 0);
                XSetWMProtocols(display, window, &wm_delete, 1);
                XMapWindow(display, window);
                frame_width = arg1;
                frame_height = arg2;
                image = XCreateImage(display, DefaultVisual(display, 0), 24,
                                     ZPixmap, 0, 0, frame_width, frame_height,
                                     32, 0);
            }
            result = image ? 6 : 0;
            break;

        case SYS_POLL_EVENT:
            /* the event data goes into the scratch register */
            result = EVENT_NONE;
            while (image && XPending(display)) {
                int type;

                XNextEvent(display, &event);
                type = event.type;
                if (type == ClientMessage &&
                    event.xclient.data.l[0] == (long)wm_delete) {
                    result = EVENT_QUIT;
                    break;
                }
                if (type == KeyPress || type == KeyRelease) {
                    mem[REG_SCRATCH] = XLookupKeysym(&event.xkey, 0);
                    result = type == KeyRelease ?
                             EVENT_KEY_RELEASE : EVENT_KEY_PRESS;
                    break;
                }
                if (type == ButtonPress || type == ButtonRelease) {
                    mem[REG_SCRATCH] = event.xbutton.x |
                                       event.xbutton.y << 14 |
                                       event.xbutton.button << 28;
                    result = type == ButtonRelease ?
                             EVENT_BUTTON_RELEASE : EVENT_BUTTON_PRESS;
                    break;
                }
                if (type == MotionNotify) {
                    mem[REG_SCRATCH] = event.xmotion.x |
                                       event.xmotion.y << 14 |
                                       event.xmotion.x << 28;
                    result = EVENT_MOTION;
                    break;
                }
            }
            break;

        case SYS_DRAW_FRAME:
            /* arg2 = word address of a 32-bit pixel framebuffer */
            result = arg2 & ADDR_MASK;
            if (image && result + frame_width * frame_height < ADDR_MASK) {
                image->data = (char *)(mem + result);
                XPutImage(display, window, gc, image, 0, 0, 0, 0,
                          frame_width, frame_height);
                XSync(display, 0);
            }
            break;

        case SYS_NOP:
            break;

        ARITHMETIC(GROUP_UINT, unsigned)
        ARITHMETIC(GROUP_INT, int)
        ARITHMETIC(GROUP_USHORT, unsigned short)
        ARITHMETIC(GROUP_SHORT, short)
        ARITHMETIC(GROUP_UCHAR, unsigned char)
        ARITHMETIC(GROUP_SCHAR, signed char)
        }

        if (insn >> 14 & 127)
            store_result(insn >> 14 & 127, dest, insn >> 18 & 24, result);
        else if (result)
            mem[REG_PC] += dest;    /* conditional jump */
    }
}
