/*
 * The original entry is not C code at all: "main" is an array of shorts that
 * holds raw machine code for both the PDP-11 and the VAX-11.  Both versions
 * of the machine code run the same algorithm, written out here in C.
 *
 * Layout of the original 81 word array:
 *
 *   word  0       PDP-11 "br" instruction that jumps over the VAX code
 *                 (on the VAX this word is the register save mask)
 *   words 1-14    VAX code: write(1, smiley, 9) then a 32767 step delay loop
 *   words 15-17   VAX stub for the write() system call (chmk $4; ret)
 *   bytes 35-43   the shared 9 byte string "  :-)\b\b\b\b"
 *   words 22-37   PDP-11 code: sys write, then 512 calls to the
 *                 nonexistent system call 55 as a delay
 *   words 38-80   leftover relocation and symbol table from the object file
 *                 ("vax", "pdp", "str", "write")
 */

extern int write();

#define STDOUT 1

/*
 * Two spaces wipe out the previous smiley and move one column to the right,
 * then four backspaces put the cursor back where the next smiley will start.
 */
char smiley[] = "  :-)\b\b\b\b";

#define SMILEY_LENGTH 9

/* the VAX version counted a register down from the largest short */
#define DELAY_STEPS 32767

/*
 * A busy wait.  The counter is volatile so that the compiler does not remove
 * the empty loop, just like the decl/jneq loop of the original VAX code.
 */
void delay() {
  volatile int steps;

  for (steps = DELAY_STEPS; steps != 0; steps--)
    ;
}

int main() {
  for (;;) {
    write(STDOUT, smiley, SMILEY_LENGTH);
    delay();
  }
}
