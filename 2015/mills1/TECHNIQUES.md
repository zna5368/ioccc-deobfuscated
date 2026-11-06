# 2015/mills1

Difficulty: Easy

1. Source code drawn as a picture

The whole of `prog.c` is spread out with spaces so that the text forms ASCII
art. Single statements are cut in half across lines and gaps, such as `y` and
`>>8` in `mvaddstr(y` / `>>8,`. The compiler ignores spaces and line breaks, so
the layout changes nothing but makes the code very hard to read. `simple.c` uses
normal indentation.

2. Function names built by token pasting

`#define P(a,b,c) a##b##c` glues three pieces of text into one word before the
compiler sees it. `P(cb,rea,k)()` becomes `cbreak()`, `P(usl,eep,)(U)` becomes
`usleep(U)` and `P(m,vpr,intw)` becomes `mvprintw`. Searching the source for
those names finds nothing. `simple.c` calls each function by its real name.

3. Comments used as camouflage

`#include/*++**++*/<curses.h>` puts a comment inside the directive, and the
comments `/*&%   &*/`, `/*     */`, `///` and `//` are only there to fill out the
picture. Comments vanish before compiling, so they are pure noise. The comment
in the include is enough to stop a simple text search for `#include <...>` from
finding it.

4. Settings hidden in the build command

`U`, `W`, `D`, `H`, `A`, `I`, `B`, `F` and `G` are never defined in `prog.c`.
They come from the Makefile as `-DU=99999`, `-DW=28` and so on. Reading the
source alone, `usleep(U)` or `F:B` tell you nothing about what is being used.
`simple.c` gives each one a long name such as `STEP_DELAY_USEC` and `FLAP_SPRITE`
and shows its default value, while still accepting the same `-D` options.

5. One letter variables, reused for different jobs

`int c,h,v,x,y,s,i,b;` hides all the game state behind single letters. `i` is
worst, because it is the row counter in the drawing loop, then the bird's row in
the crash test, and then the score. `simple.c` has names like `pipe_timer`,
`gap_top` and `best_score`, and uses a separate local variable for each job.

6. Program logic stuffed into `for` headers

The three parts of a `for` usually set up, test and advance a counter. Here they
hold real work. `for(curs_set(0); s=x=COLS/2; flushinp())` starts the game,
assigns two variables in the test, and clears the keyboard between rounds. The
inner loop has the instruction message as its setup and `usleep` as its step,
and the drawing loop has an empty body, because `mvinsch` runs as the step.
`simple.c` puts these in ordinary statements and splits them into functions.

7. Assignments hidden inside other expressions

`timeout(y=c=v=0)` resets three variables while passing 0 to `timeout`. The crash
test is one line: `if((i=(y+=v=getch()>0?I:v+A)>>8)>=LINES||mvinch(i*=0<i,x)...`.
That single condition reads a key, sets the velocity, moves the bird, works out
its row and checks for a collision. `simple.c` does each of these on its own line.

8. Sign bit tricks instead of comparisons

`0>(~c|i-h-H&h-i)` decides if a cell is empty. A number is negative when its top
bit is 1. `|` gives a negative result if either side is negative, and `&` only
if both sides are. `~c` flips all the bits, so it is negative when `c>=0`.
`i-h-H&h-i` is negative only when `i<h+H` and `h<i`, meaning the row is inside
the gap. `(i-h|h-i+H)<0` works the same way to test for a row outside of the gap.
`simple.c` writes plain comparisons joined by `||` and `&&`.

9. Multiplying by a comparison

A comparison in C gives 1 or 0, so it can be used as a number. `i*=0<i` keeps `i`
if it is positive and turns it into 0 otherwise. `(0<i)*i` does the same for the
score, so it can never be shown as negative. `simple.c` uses `if` statements.

10. The "goes to" operator

`i-->0` looks like an arrow, but it is really `i--` followed by `> 0`. The test
uses the old value and the body sees the new one, so the rows run from `LINES-1`
down to 0. `simple.c` writes a normal counting loop.

11. Reversed comparisons

`0>v`, `' '!=mvinch(...)` and `0>(...)` put the constant first, which is the
opposite of how most people write and read them. `simple.c` always writes the
variable first, as in `bird_velocity < 0`.

12. Fixed point numbers

`y` and `v` are integers that really hold fractions of a row, with 256 units per
row. `y>>8` shifts away the lower 8 bits, which is the same as dividing by 256,
and gives the screen row. This allows smooth gravity without floating point, but
nothing in the source says so. `simple.c` names the shift `SUBROW_BITS`.

13. Score calculated from a negative division

`i=--s/-W` has no visible link to passing pipes. `s` starts at half the screen
width, which is how far a pipe must travel to reach the bird, and goes down by
one each step. Dividing by `-W` makes the result positive only once `s` is below
zero, and it goes up by 1 for each `W` steps after that. `simple.c` names this
`score_countdown` and explains the formula in a comment.

14. One counter with two meanings

`c` tracks both pipes and spaces using its sign. `if(++c==D){c-=W;...}` jumps it
to a negative number when a pipe should start. It stays negative for `W-D` steps
while the pipe is being drawn, and then counts up from 0 during the space.
`~c` in technique 8 is how the sign gets tested. `simple.c` calls it `pipe_timer`
and tests `pipe_timer >= 0`.

15. The screen used as the only game data

There is no array holding the pipes. `mvinsch` inserts a character at column 0,
which pushes the whole row right, so the world scrolls without any code seeming
to move it. Collisions are found by reading characters back from the screen with
`mvinch`, so the code never mentions where the pipes are. `simple.c` keeps this
design, but explains it in `scroll_world_right` and `bird_has_crashed`.
