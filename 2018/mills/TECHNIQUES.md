# 2018/mills

Difficulty: Terrifying

1. Switch statement hidden in a macro

`# define L } if(!i -- ){` is the heart of the program. A block written as
`{ L A L B L C }` expands to `{ } if(!i--){ A } if(!i--){ B } if(!i--){ C }`.
Each test subtracts one from `i`, so only the test where `i` reaches zero
runs its code, which makes it a `switch (i)` with `case 0: A; case 1: B; ...`.
This "switch" picks the opcode (`i=k/f`), the EAE operation (`i=k/N&7`), the
I/O device (`i=k/N%N`) and the disk operation (`i=k/16&3`). simple.c uses real
`switch` statements and splits each part into its own function
(`execute_instruction`, `eae_setup`, `eae_step`, `io_transfer`, `rb09_disk`).

2. Changing the switch variable inside a case

Because the macro "switch" keeps testing `i`, a case can choose which case
runs next by setting `i`. Opcode 0 (`CAL`) does `i=1`, which skips the `DAC`
test and lands on the `JMS` case, so `CAL` acts as `JMS 20`. Setting `i=2`
at the end of the EAE and IOT code is a hidden `break` that steps over the
two remaining tests. In the device code the opening `{` of `if(i==57){` is
closed by the first `L`, so the disk code and the device switch share one
`i`. Disk operation 3 counts `i` down to zero and also runs the clock case.
simple.c writes `case CAL: case JMS:` and returns device 0 from
`rb09_disk` for operation 3 so this behavior is kept.

3. Single letter globals used for more than one job

Every variable is a global with one letter: `m` is the accumulator, `n` the
link, `J` the MQ register, `o` the program counter, `k` the instruction, `u`
the device flags, `v` the interrupt enables, `x`/`y`/`z`/`B`/`C` the disk
registers, and so on. Some letters do several jobs. `t` is an address, the
EAE operand and the IOT skip flag. `w` holds the pending interrupt enable and
is also the place value in the BCD conversion. `q` is the EAE step counter
but also the file descriptor 0 given to `tcgetattr`, and `y` and `B` are also
used as descriptor 0 because they are still zero. The disk code leaves its
BCD results in `t` and `r`, which the IOT code then checks as flags, so
simple.c passes those two to `rb09_disk` by pointer to show this.

4. Magic numbers made from other magic numbers

`enum{ N=64,a=N<<7,b=a-1,c=a*32,d=c-1,e=c/2,f=a*2,g=a/2,h=g/2,j=h/2,...}`
builds every bit mask from `N`. `a` is 020000, `b` the 13-bit address mask,
`c` 01000000, `d` the 18-bit word mask, `e` the sign bit, and so on. The same
value means different things in different places, like `h` being the clock
flag, the EAE absolute value bit and the disk interrupt enable. simple.c
names each use separately (`CLOCK_FLAG`, `DISK_INT_ON`, ...) and writes the
masks in octal, as PDP-7 manuals do.

5. Configuration passed in through the compiler

The program does not compile without `-D` flags. `I` is the disk image name,
`A` the load address, `S` the console switches, `W` the `mmap` mode, `T` the
`termios` fields, `X` the quit key, `Y` the parity bit, `V` the disk size and
`P` the boot program. The `Makefile` changes these to build several different
programs from one source. simple.c gives them long names with the `prog`
build values as defaults.

6. Program encoded as a printable string

`P` is the boot paper tape. Every three characters make one 18-bit word,
6 bits each, taken with `%N` (`%64`), which is why the string is made of
characters like `@`, `8` and `9`. The loop that reads it is hidden in the
`for` header: `o++[l]=k|=*s++%N` stores the word, and the last word stays in
`k` so it is the first instruction executed. simple.c writes the decoded
octal words in a comment.

7. Odd ways to write arithmetic

Shifts, masks and tests are written as multiply, divide and modulo:
`m/e%2` is the sign bit, `J|=m%N*c; J/=2` moves the low bit of the
accumulator into the top of MQ, `m|=m/a/N` is `m >> 19`, `(m^2*m)/e%2`
compares bits 17 and 16, `u%=c` and `v%=e` clear the top flag, and
`q=- --k%N` decrements the step count and negates it. `o++[l]` and `z[D]`
use the rule that `a[b]` is the same as `b[a]`. simple.c uses `>>`, `<<`
and `&` where they make the meaning clearer.

8. Instruction state stored in spare bits

When an EAE instruction needs more steps, `k+=c` sets bit 18 of the
instruction register and keeps the step count in its low 6 bits. The main
loop sees `k&c` and does one step per pass without fetching a new
instruction. A trap is shown by setting `k=-1`, which is noticed later by
`!++k`, and `o=1+!k` picks address 2 for a trap and 1 for an interrupt.
`XCT` jumps back to the top of the loop with `continue` to run the fetched
word. simple.c names the bit `EAE_BUSY` and uses the `enum result` return
values `EXECUTE_AGAIN` and `HALT` in place of `continue` and `break`.

9. Comment trick inside an expression

`t=t/80*/*/*/100+t% 80` looks broken, but `/*/*/` is a comment, so the
expression is `t/80*100+t%80`. Another empty comment `/*$ %*/` sits next to
`continue` only to fill the ASCII art.

10. Code shaped like ASCII art

The source is laid out as a torn strip of paper tape, with the columns of
holes made from tokens cut up across lines (`m\n/= 2`, `J\n|= m %N *c`).
This makes it hard to see where statements start and end.
