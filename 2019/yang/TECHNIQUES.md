# 2019/yang

Difficulty: Hard

1. C and Ruby polyglot

`prog.c` is a C program and a Ruby script at the same time. To C, line 3 is inside
a `/* ... */` comment that opens at the end of line 2. To Ruby, lines starting with
`#` are comments, so it runs line 3 (a loop that prints its input slowly) and then
`%q{*/` starts a Ruby string that swallows all the C code. The string closes at the
`}` in the final `/*}#*/`. That is why the README can run `ruby prog.c`.

2. Whole program inside one macro

The program is the body of `#define T(_,S,C,R,A,M,B,L,E)`. The macro is only used
on the last line: `T(int,for,if,char,void,goto X,(swap),(fopen ...),fclose(f))`.
So in the body `_` means `int`, `S` means `for`, `C` means `if`, `R` means `char`
and `A` means `void`. `M` is `goto X` (exit), `B` swaps `U[x]` with `U[y]`, `L`
opens the next file named on the command line and `E` is `fclose(f)`. You must
read the end of the file before any of the code makes sense, and the lines end
in `\` so they can stay inside the macro.

3. Encoded width table

`I[22]` holds the Unicode ranges of characters that are two columns wide (like
Japanese characters). The numbers are not the ranges themselves but the gaps
between them: each pair is "distance from the end of the last range" and "length
of this range". For example `4352,95` is the range `0x1100` to `0x115F`, and
`4554,1` is `0x2329` to `0x232A`. Function `v()` rebuilds the ranges while it
searches, and then does `x += 2 - i/22`, which adds 2 if a match was found (the
loop stopped early so `i < 22`) and 1 otherwise.

4. One huge nested `?:` expression as a state machine

The input parser is the third part of a single `for` loop header. A variable `h`
holds the state: negative means "inside an escape code", positive means "this many
UTF-8 bytes still to come" and zero means "normal text". The new state is
calculated with about twenty nested `?:` operators, mixed with commas so that one
branch can run several things, like `y++,x=0` for a newline. Even the result
value is hidden: for the ESC character the state becomes `~(l=0)`, which is `-1`,
and for an escape code digit it becomes `~l`, which is negative as long as `l` is
not negative.

5. Bit tricks with character codes

Letters are tested by their bits instead of compared. `(e&2)-1` is `-1` for `A` and
`D` and `+1` for `B` and `C`, which gives the direction of the cursor movement.
`(e&224)==192`, `(e&g)==224` and `(248&e)==g` (with `g=240`) find UTF-8 lead bytes.
When writing UTF-8, `(g*8>>o&g)` builds the lead byte prefix `0xC0`, `0xE0` or `0xF0`
from the single constant `g` by shifting it.

6. Toggling global for the escape letter

`Q()` prints a cursor movement as `"\33[%d%c"` with the letter `V+(O>0)`, then flips
`V` with `V^=2`. With `V=65` the first call prints `A` or `B` (up/down) and the
second call prints `C` or `D` (left/right), then it is back to 65. The same
variable `V` is first used as a hash of the input that seeds `srand()`.

7. Reused one-letter globals

Almost every variable is a global with a one letter name, and the same variable
does different jobs in different places. `t` is first the array size and later the
output part number. `h` is the parser state, then an array index, then a newline
counter. `x` and `y` are the cursor, but also loop indexes for the shuffle and the
merge sort. Helper functions such as `v()` quietly overwrite `i`, `o` and `l`.

8. Hidden meanings in expressions

`-~x` is `x+1` (`~x` is `-x-1`), so `rand()%-~x` is `rand()%(x+1)` for the shuffle.
`1[*Z]` is `argv[1][1]` written with the index and the array swapped. `**++Z-45`
checks whether the argument is not `-` (45 is the code of `-`). The open mode
string `d` starts as `"r"` and becomes `"w"` with `*d|=5` because `'r'|5` is `'w'`.
`realloc(U, t*24)` allocates two structures (12 bytes each) per slot, so the
second half of the array can be used as scratch space for the merge sort.

9. Merge sort with swaps and a packed key

`m()` is a merge sort written with swaps (the `B` macro) instead of copies, and it
compares positions as one number `y*1e6+x` so it sorts by row and then column in a
single comparison. The call `m(0,U[H].x=H)` also hides an unneeded assignment
inside the function argument.

10. Error handling folded into conditions

`main` starts with `if(--O&&(**++Z-45||1[*Z])&&--Z&&L)M;` which counts the
arguments, checks for `-`, moves the argument pointer back and forth and opens
the file all in one condition. `L` evaluates to `-1` (true) only when `fopen`
failed, after calling `perror` with the comma operator.
