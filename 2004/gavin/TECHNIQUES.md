# 2004/gavin

Difficulty: Insane

1. Macros hidden in the Makefile

Most of the macros are not in `gavin.c` at all. They are `-D` options in
`CDEFINE` in the Makefile: `Y` (read an `int` at an address), `E` (call an
address as a function), `M` (loop over every screen pixel), `Z` (declare the
local variables), `V` (the base address `0x90200`), `K`, `R` and `L`. Without
the Makefile the source cannot be read or compiled. The `sh` and `vi` programs
are built from the same file with two more options: `-DB=_start` makes
function `B` the program entry point and `'-Dputchar(a)='` deletes every
`putchar` call.

2. Macros with unbalanced braces

`F` is `if(!--q){` and the matching `}` is written later in the source, so
the code between looks like it is outside any `if`. `R` is a `while(...){`
whose `}` is many lines away: in `cain` the line `for (; 1;) R n = ...` opens
the body of the main loop, which only ends near the end of the function. `L`
ends with `R}` twice, so it writes two bytes to the keyboard controller and
waits after each one. The indented layout of `gavin.c` follows the visible
braces, not the real ones, so it is misleading.

3. Counting down a message number

Each function gets a message or system call number in `q`. Every `F` block
subtracts one from `q` and runs only when it reaches zero, so the first `F`
handles 1, the second 2, the third 3. In `_` this is mixed with
`if (q > 0)`: numbers 3 and 4 both search the file system, and only 3 reaches
the last `F` that runs the file. In `simple.c` these are `if` chains with
named numbers such as `SYS_EXEC` and `MSG_RENDER`.

4. The program prints its own machine code

The last loop prints 720 bytes from the string and then `Y(I(D))` with `D`
going negative. `I(D)` is `(int)cain - D`, so this reads the compiled machine
code of `cain` and the functions after it (`o` and `_`) and writes it out. The
output is the operating system kernel. When the boot loader starts it, the
same `cain` runs again with a zero first argument and takes the other branch.
This is why the kernel code can only use local variables and fixed addresses:
it runs at a different address than it was compiled for.

5. Machine code and data in a string literal

The long string is the start of the kernel: a Linux boot header, small `inb`
and `outb` machine code routines, the 16 bit setup code that switches the
video mode and enters 32 bit mode, a GDT and a keyboard map. It is stored
backwards (printed from index 720 down to 1) and every byte is XOR 32, which
turns most zero bytes into spaces and makes the keyboard map read as
`;lkjhgfdsa` and `][poiuytrewq`. The string is only 226 bytes long, so the
first 495 bytes printed come from whatever memory follows it. `simple.c`
writes the bytes forwards in an array with each part labelled and the
instructions written next to their bytes.

6. Fixed memory addresses instead of variables

Data is reached through addresses near `V`. `Y(V)` holds the address of the
system call function, `V-8` and `V+8` are the `inb` and `outb` routines,
`Y(V-12)` is the list of windows, `Y(V-16)` is the memory allocation pointer,
`Y(V+24)` is the RAM disk loaded by the boot loader, `Y(V+36)` is the BIOS
font and `V+131` is the keyboard map. `Y(0x11028)` is the video memory address
filled in by the VESA BIOS. Several of these overlap the Linux boot header, so
one byte has two jobs, such as the GDT size also being the `setup_move_size`
field. `simple.c` gives each address a name.

7. Structures as integer offsets

Everything is an `int`, including pointers. A window's task is read with
`Y(t + 4)`, `Y(t + 12)` and so on, and a file is read with offsets like
`Y(i + 32)` (the ELF section table) or `t + 124` (the tar file size). The
meaning of each offset has to be worked out from how it is used. `simple.c`
uses a `struct task` with named fields and comments on the file format
offsets.

8. Packed numbers

Screen positions are one number: row * 1024 + column, and window sizes are
height * 1024 + width. So `442890` is a 432 by 522 window, `26628` is 26 rows
down and 4 columns right, `393728` is the middle of the screen and `8196` moves
the mouse pointer up 8 and left 4. `26739` is the bytes `"sh"` stored in an
`int`. `simple.c` writes them with an `AT(row, col)` macro.

9. One counter for a two dimensional loop

`P` draws a 16 by 8 character with a single loop `h += 129`. Because
129 = 128 + 1, `h / 1024` gives the row and `h & 7` gives the column, and
`h & 15367` keeps only those bits to make the screen offset. The `: k` at the
end of the `?:` does nothing; it is there because a `?:` needs both branches.
`simple.c` uses two nested `for` loops.

10. Nested conditionals for drawing

The window frame is one long chain `X(...) ? X(...) ? ... : 20 : 28 : 0 :
U(d + D)` where each `X` tests if a pixel is inside a slightly smaller
rectangle. The colour depends on how many rectangles the pixel is inside,
which draws the black edge, the raised border and the title bar. `simple.c`
writes this as an `if`/`else if` chain with named colours.

11. Arithmetic tricks instead of conditions

- `p |= n * 56 - 497` turns a newline (10) into 63 and a tab (9) into 7, so
  the text is padded to the next 64 or 8 column boundary.
- `Y(Y(t + 4)) & 1` tells the shell from the viewer by whether the first
  letter of the program name is odd (`s`) or even (`v`).
- `62 & -n` is `'>'` for the shell and 0 for the viewer.
- `((d >> 4) ^ (d >> 5)) - 3` is -1 for the up arrow and page up keys and +1
  for down arrow and page down.
- `1 - k * 1025` is 1 for the x byte and -1024 (one row up) for the y byte of
  a mouse packet.
- `j &= -i` clears the dragged window when the button is released.

12. Doing work inside a loop condition

When the mouse button is pressed, the `for` loop condition in `cain` does the
whole job: if the pointer is inside window `n` the comma expression removes it
from the list, puts it on top, remembers it in `j` and gives 0 to stop the
loop, otherwise it gives `n` to keep going. The fake list entry at `V-20` has
its "next" field exactly at the list head `V-12`, so the first window can be
removed the same way as the others.

13. Writing to address 0 on purpose

When no window is being dragged, `j` is 0 and the mouse code still does
`Y(j) -= p` and `Y(j) += p += ...`. This changes memory address 0, which the
OS does not use, so no `if` is needed.

14. A `goto` into the middle of a block

After the y byte of a mouse packet, `goto y` jumps into the `else if` block
for key presses to reuse the screen redraw code. `simple.c` sets a `redraw`
flag instead.

15. The same variables everywhere

`Z` declares `i, j, h, n, p, s, C, D` with odd starting values in every
function, and they are reused for unrelated things. In `cain` `C` and `D` are
the keyboard controller status and data, `p` is the mouse position, `j` is the
dragged window and `s` holds `"sh"`. In `_`, `D` starts as the RAM disk address
and the file size loop shifts it left 33 bits so the old value disappears.

16. Meaningless names

The functions are `cain` (looks like `main`), `_`, `o` and `B`, and `G(n)`
gives them all the same signature so they can be called through a pointer
with `E`. `simple.c` names them `kernel_main`, `kernel_syscall`,
`draw_windows` and `application`.
