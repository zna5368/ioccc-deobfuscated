#include<SDL3/SDL.h>

/*
 * A virtual machine for a one instruction computer (SUBLEQ with an
 * indirect addressing mode), with a memory mapped framebuffer, keyboard,
 * real time clock and timer interrupt.
 */

#define MEMORY_WORDS     (3 << 27)          /* 1.5 GB of 32-bit words */
#define MAX_IMAGE_BYTES  0x66726f67         /* 'frog' as a number */

#define SCREEN_WIDTH     800
#define SCREEN_HEIGHT    512
#define SCREEN_BYTES     (SCREEN_WIDTH * SCREEN_HEIGHT * 4)

#define TIME_SLICE       800000             /* instructions between interrupts */

/* memory mapped registers (word indexes into mem[]) */
#define INTERRUPT_VECTOR 0      /* where to jump on an interrupt, 0 = disabled */
#define SAVED_PC         1      /* return address saved by an interrupt */
#define FRAMEBUFFER      6      /* word index of the framebuffer in mem[] */
#define CLOCK            64     /* struct timespec, updated when read */
#define IO_PORT          (-1)   /* as source: keyboard, as destination: none */

int cycles;                     /* instructions since the last interrupt */
int src;                        /* operand a */
int dst;                        /* operand b */
int target;                     /* operand c */
int pc;                         /* program counter (word index) */
int mem[MEMORY_WORDS];
SDL_Event event;
SDL_Window *window;
SDL_Surface *surface;

/*
 * Fetch the next operand. Operands are byte addresses, so they are divided
 * by 4 to get a word index. If the low bit is set, the operand is indirect
 * and the word it points to is used as the operand instead.
 */
int fetch_operand(void)
{
    int addr = pc++;

    if (mem[addr] & 1)
        addr = mem[addr] / 4;
    return mem[addr] / 4;
}

int main(void)
{
    /* load the boot image from stdin, use fullscreen if it was empty */
    int bytes = read(0, mem, MAX_IMAGE_BYTES);

    window = SDL_CreateWindow(0, SCREEN_WIDTH, SCREEN_HEIGHT,
                              bytes == 0 ? SDL_WINDOW_FULLSCREEN : 0);
    surface = SDL_GetWindowSurface(window);

    for (;;) {
        src = fetch_operand();
        dst = fetch_operand();
        target = fetch_operand();

        /* a jump target of 0 halts the machine */
        if (target == 0)
            break;

        /* reading the clock register refreshes it */
        if (src == CLOCK)
            timespec_get((struct timespec *)&mem[CLOCK], TIME_UTC);

        if (src == IO_PORT) {
            /* store +scancode for a key press and -scancode for a release */
            if (SDL_PollEvent(&event))
                mem[dst] = event.key.scancode *
                           (2 * SDL_EVENT_KEY_DOWN + 1 - 2 * (int)event.type);
        } else if (dst != IO_PORT) {
            /* subtract and branch if less than or equal to zero */
            mem[dst] -= mem[src];
            if (mem[dst] <= 0) {
                pc = target;
            } else if (mem[INTERRUPT_VECTOR] && ++cycles > TIME_SLICE) {
                /* timer interrupt: draw the screen and call the handler */
                memcpy(surface->pixels, mem + mem[FRAMEBUFFER], SCREEN_BYTES);
                cycles = SDL_UpdateWindowSurface(window); /* true (1) */
                mem[cycles] = 4 * pc;       /* mem[SAVED_PC] on success */
                pc = mem[INTERRUPT_VECTOR] / 4;
            }
        }
    }
    return 0;
}
