# 1989/tromp

Difficulty: Hard

1. Whole board in a one dimensional array

The playing field is `q[276]`, a 12 x 23 grid stored row after row. Moving
left or right is `-1`/`+1` and moving up or down is `-12`/`+12`, so all the
numbers like `12`, `17`, `252`, `263` and `264` are really rows and columns
(`17` is row 1 column 5, `252` is the start of the floor). Columns 0 and 11 and
the last two rows hold `7` so they act as walls and floor and pieces stop on
them without any special checks. The setup loop
`for(...;i;i--)*n++=i<25||i%12<2?7:0;` even counts backwards from 276, so the
wall test `i%12<2` is really `column is 0 or 11`. `simple.c` uses `WIDTH`,
`FLOOR`, `SPAWN`, `LEFT`, `RIGHT`, `UP` and `DOWN`.

2. Tetris pieces as a table of numbers

All 19 shapes (the 7 pieces in every rotation) are in the array `f`, four
numbers per shape. The first number is which shape you get after rotating, and
the other three are offsets of the other cells from the centre cell. For
example `7,-13,-12,1` means "rotates to shape 7, cells up-left, up and right",
which is the Z piece. Rotating is just `n=f+4**n`. `simple.c` writes this as a
table of `struct shape` with named shapes such as `Z_90` and offsets such as
`UP + LEFT`.

3. Index 0 means something different

In `G` and `g` the expression `q[i?b+n[i]:b]` uses `b` itself when `i` is 0
and `b+n[i]` otherwise, because `n[0]` holds the rotation number and not an
offset. One loop handles the centre cell and the three other cells. In
`simple.c`, `fits()` and `paint()` check the centre first and then loop over
`offset[0..2]`.

4. Cell value is also the colour

Empty cells are `0` and full cells are `7`, and `7` is also the VT100 code for
reverse video (`ESC[7m`) while `0` is normal video. The drawing code just
prints `"\033[%dm"` with the cell value, so the board needs no colour table.

5. Skipping part of a format string

`printf("\033[%dm  "+(K-k?0:5),k)` adds 5 to the string pointer when the video
attribute has not changed, which skips over the 5 characters `ESC [ % d m` and
only prints the two spaces. `simple.c` uses an `if` and two `printf` calls.

6. Minimal screen updates with a shadow board

`Q` is a copy of what is currently on the terminal and `I` is where the cursor
is. `u()` only prints the cells where `q` and `Q` differ, and only sends a
cursor move when the next changed cell is not right after the last one
(`i-++I`). The pause code wipes `Q` to make the next update redraw everything.
`simple.c` calls these `screen`, `cursor` and `refresh()`.

7. Keyboard input stored in the board copy

`Q[263]=c=getchar()` writes the key into the shadow copy of the bottom right
wall cell. That cell is always `7` on the board, so it nearly never matches and
is redrawn on every update. This parks the cursor in the corner and keeps the
terminal in reverse video (`K` is always `7` afterwards), which is why the
pause code can print `ESC[7m` when it resumes.

8. Timer signal turns into a fake key

The game never checks the clock. A `SIGALRM` timer interrupts the blocking
`getchar()`, which then returns `EOF` (`c<0`), and that is treated as "move the
piece down". The handler `t()` re-arms a one shot timer and takes 1/3000 off the
delay each time, so the game slowly speeds up. The timer is the array `long
h[4]`, which has the same layout as `struct itimerval` (`h[3]` is
`it_value.tv_usec`), and the signal action is the array `v[]={0,0,2}` with the
handler address put in `v[0]`. It is passed to `sigaction` as if it were a
`struct sigaction` (it used to be a BSD `struct sigvec`, where `2` meant
"interrupt system calls"). `simple.c` uses real `struct itimerval` and
`struct sigaction` variables.

9. Magic numbers for signals and timers

`sigaction(14,...)` is `SIGALRM`, `setitimer(0,...)` is `ITIMER_REAL` and
`sigblock(8192)` is `1<<13`, the mask bit of signal 14. These are named in
`simple.c`.

10. Clever loops for clearing rows

`for(j=0;j<252;j=12*(j/12+1))for(;q[++j];)if(j%12==10){...}` walks along each
row until it finds a hole; reaching column 10 means the row is full. The row is
blanked by `for(;j%12;q[j--]=0);`, which leaves `j` at the start of the row so
`for(;--j;q[j+12]=q[j]);` can copy everything above it down one row. That leaves
`j` at 0, so the scan quietly starts again from the top. `simple.c` splits this
into `row_is_full()`, `remove_row()` and `remove_full_rows()`.

11. Short circuit operators instead of if statements

Moves are written as "try it and undo if it does not fit": `G(--x)||++x` moves
left and moves back if the piece does not fit. Rotation uses the comma operator
in `n=f+4**(m=n),G(x)||(n=m)`, and game over is `G(x=17)||(c=a[5])`, which
pretends the quit key was pressed. `simple.c` uses `if` statements.

12. Loop increment does the real work

The main loop is `for(...;;g(7),u(),g(0))`: draw the piece onto the board,
update the screen and read a key, then erase the piece again. Several other
loops such as `for(i=4;i--;q[...]=b);` and `for(j=264;j--;Q[j]=0);` also have
an empty body with the work done in the increment part.

13. Single letter names and shared globals

Every variable is a global with a one letter name, and the same `i` is the loop
counter in `u()`, `G()`, `g()` and `main()`. `c` is the key, `w` the score, `x`
the piece position, `n` first fills the board and then points at the current
shape, and `a` is the string of command keys. `simple.c` uses names like `key`,
`score`, `position`, `piece` and `keys`.

14. Old style C

There are no `#include` lines, functions have no return type, `main` uses a K&R
parameter list and `t()` is used as a signal handler without taking an `int`.
The high score file is handled by a shell pipeline in `popen`. `simple.c` uses
modern function definitions with return types but keeps the same calls.
