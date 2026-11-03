# 1998/chaos

Difficulty: Medium

1. Single-letter macros for types and keywords

`#define A int`, `#define F float` and `#define O sizeof` turn common words
into single letters, so `A main(A N,char ** Y)` is really
`int main(int argc, char **argv)` and `O(F)` is `sizeof(float)`. `U(a,b)` is a
short `for (a = 0; a < b; a++)` loop. Replacing each letter with what it stands
for makes the code readable again.

2. Single-letter variable names reused for different jobs

Every variable is one capital letter and most do several jobs. `N` starts as
`argc`, then holds the number of floats read from the data file. `M` is the key
pressed, a loop counter inside the rotation code, and the y distance of a line.
`E` is a loop counter, the x distance of a line, and the rounded x pixel. Giving
each job its own named variable (`count`, `key`, `dx`, `px`, ...) shows what is
going on.

3. One buffer used for two different things

`char Q[3792]` first holds the text `"AlWuzEre %d"`, which is used as the
`fscanf` format to find the data. Later the same array is cleared and used as a
79 by 48 grid of lit pixels (79 * 48 = 3792). The clean version uses a small
`format` array and a separate `lit[48][79]` grid.

4. One memory block split into two arrays of different types

`malloc` gets one block big enough for twice as many numbers as needed. `X`
uses the first half for the 3D points as floats. `#define V ((A *)(X+N))` casts
the second half to an `int` pointer to store the projected 2D screen positions.
Two separate `malloc` calls, one for `float` and one for `int`, make this
clear.

5. The program reads its own executable

`fopen(Y[N<2?0:1],"r")` opens `argv[1]` if it was given, but otherwise opens
`argv[0]`, which is the program file itself. A default cube is stored in the
string `P` starting with the marker `"AlWuzEre 72 ..."`. The program searches
the file one byte at a time (`fseek(W,1,SEEK_CUR)`) until `fscanf` matches the
marker followed by a number, so it finds its own string inside the compiled
binary. The data files start with the same marker, so the same search works for
them. The format `"AlWuzEre %d"` is built at run time with
`strcpy(Q,P); strcpy(Q+9,"%d");` so that the format text itself never appears
in the binary.

6. Code-generating macro for rotations

`_(e,f,a,b,c)` expands into a whole `if` block that builds a rotation matrix
and multiplies it into the view matrix `H`. It is used six times, once for
each rotation key. The axes are passed as numbers (`1,2,0`), and the sign
argument `f` is either empty or `-`, which is pasted right in front of the
number `.1045284632` to make it positive or negative. The constants are the
sine and cosine of 6 degrees, written without explanation. A `rotate()`
function with named constants `SIN_STEP` and `COS_STEP` replaces the macro.

7. Hidden side effects that switch macros off

Inside the rotation macro the key variable `M` is reused as a loop counter, so
after one rotation runs `M` is 3 and none of the other key tests match. The
statement `!(T[c*5] = 1)` sets a diagonal entry to 1 and at the same time
produces 0 for the four other entries in the same chain of `=`. A `switch` on
the key does the same job clearly.

8. Flattened 2D arrays with stride tricks

The 3x3 matrices are stored as flat 12-entry arrays with rows of 4, so
`H[a*4+b]` is row `a`, column `b`, and `T[c*5]` is the diagonal entry `T[c][c]`
(because `c*4+c = c*5`). The fourth column is never used. `L(a)` is a macro that
multiplies the current point by row `a` of `H`. Real `[3][3]` arrays and a
`view()` function make the math visible.

9. Pre/post increments hidden inside expressions

In the line drawing loop, `C = V[B++]; D = V[B++]; E = V[++B] - C; M = V[++B] - D; B++;`
walks the index `B` through six slots for one line, skipping the unused z slots.
`J=M/S++` divides and then adds one to the step count in the same statement,
and the projection loop adds 2 to `K` inside a loop that already adds 1, so it
moves 3 floats per point. Writing loops that step by 3 and 6 and passing the
four end-point values to a `draw_line()` function removes these tricks.

10. Two half-rows per text line

The screen is treated as 48 rows tall while the terminal only has 24. The
expression `Q[E+(M+(M&1?-1:1))*79]?':':(M&1?'.':'`')` checks the other half of
the same character cell: an even row is drawn as `` ` `` (top), an odd row as
`.` (bottom), and when both halves are lit it draws `:`. The clean version puts
this in a `plot()` function with comments.

11. `goto` used as the main loop and magic timer values

The main loop is a label `CH:` with `goto CH;` at the end. `halfdelay(M==ERR?1:30)`
followed by `M = M==ERR?'u':M;` makes a timeout pretend that the `u` key was
pressed, which starts the automatic spinning. A `for (;;)` loop and named
constants (`IDLE_DELAY`, `SPIN_DELAY`, `AUTO_ROTATE_KEY`) replace these.
