# 2014/vik

Difficulty: Hard

1. `main` used as every function

The only function in the program is `main`, and it calls itself with a number in
place of `argc`. The value of `v` picks the job: `main(0,b)` analyses the sound
so far, while `main(8,b)`, `main(24,b)`, `main(9,b)` and `main(19,b)` play audio.
Each loop in `main` has a test such as `v>0&&v<5` or `!v`, so a call skips the
loops that are not its job. You have to work out which loops run for which `v`
before you can see that there are several separate functions here. `simple.c`
splits them into `play`, `encode`, `decode`, `estimate_unit_length` and
`decode_events`.

2. Meaningless names in one global list

`int j,w,h,f,u,d,g,e,n,i,l,a,r,p,c,m,o,t,s,...` makes every variable a global
with a single letter, even short lived loop counters. Some are used for
unrelated things: `u` is the running loudness while bytes are read, then the
length of a Morse unit inside `main(0,b)`, and `j` is a position in the tree when
encoding but a ratio when decoding. `simple.c` gives each job its own named
variable, and most of them become locals.

3. The whole program written inside `for` headers

Almost all of the work is in the three parts of the `for` statements. The second
loop does its averaging in the increment part, the audio loop has an empty body
(`...,i++);`), and the last loop prints letters from its increment part. Even a
function call hides in a loop test: `v==1&&(c*j||main(*X-Q?8:24,b))` plays the
gap after a letter when the test is about to fail. A reader expects a `for`
header to only count, so the real logic is easy to miss.

4. `?:` and commas in place of `if` and blocks

There is not a single `if` in the program. `x?a,b,c:0` runs `a`, `b` and `c` when
`x` is true and the `0` does nothing. These are nested, as in
`!(i++%Q)?a=c>a?u=s,c:a,c=s=0:0`, which is really an `if` holding another `if`.
The `&&` and `||` operators are used the same way to skip code. `simple.c` uses
normal `if` statements.

5. Two numbers packed into one

The value of `v` for a sound holds two numbers: `v/4` is the total length and
`v%4` is how much of it is tone, so 19 means 4 long with 3 of tone. The
expression `9+(c-j)/(j/2+1)%2*10` makes 9 for a dot and 19 for a dash, and the
loop `i<v/4*I` with `i/I<v%4` unpacks it. `simple.c` passes `play` both numbers
as separate arguments with named constants.

6. A lookup table that is secretly a tree

`X` looks like a random string with lots of `:`. It is a binary tree stored one
level after another, so the path of dots and dashes can be calculated from the
position in the string. Encoding finds the path from the position with
`(c-j)/(j/2+1)%2`, and decoding goes the other way with `X[p-1+(1<<n)]`.
`simple.c` writes each level of the tree on its own line and explains the layout.

7. The input stored inside the table

`fread(X,1,1,stdin)` reads each byte over the first element of `X`, so `*X` is
the current input and not part of the table. This is a trick: `strrchr` then
always finds something, because the byte is in the string it searches, so no
check for "not found" is needed. It also means `X[0]` has to be put back to a
space with `*X=Q` before decoding a letter. `simple.c` only does this when
encoding, and keeps the decoder input in its own variable.

8. Bit tricks instead of plain expressions

- `~(Q&*X&*X/2)&*X` changes lower case to upper case by checking bits 5 and 6.
- `g&1^1` is 1 for even numbers, so it multiplies odd bytes away.
- `1 - h%2*2` gives 1 or -1 to flip the sign of a comparison.
- `d|1` keeps a divisor from being zero.
- `putchar(...)&0` prints something and turns the result into 0.
- `(i/I<v%4)*i%2*85<<(i%176/88)` builds a square wave with no branch.
- `*1[b]` means `b[1][0]`, because `a[b]` is the same as `b[a]`, and `101` is `'e'`.

9. Constants disguised as variables

`Q=32` and `I=13000` are stored in variables with no hint of what they mean.
Worse, `Q` means different things in different places: it is the size of the
arrays, the character for a space (`putchar(Q)`, `*X-Q`) and the bit for lower
case letters. `Q*Q` is the number of pairs of array elements. `simple.c` uses
`' '`, `32` and named macros for these.

10. Hidden resets and side effects

Variables are cleared by code that looks like it does something else. `u=main(0,b)`
uses the fact that `main` returns 0 to clear the loudness, `g*=!!v` clears the
byte counter only when `v` is 0, and `i=0` at the top of `main` restarts the
global counter of whichever call is running. Every call also runs `a=u=0`, and
this matters: the value of `i` left behind by the last `main(0,b)` stops the
analysis loop from running at end of input, so `u` stays 0 for the last letter.
The `t` from `fread` is reused in `r+t!=h+1` to handle one more event once the
input has run out. `simple.c` does these things directly with named arguments.

11. Two loops flattened into one

The analysis compares every pair of array elements with a single counter `i`,
using `T[i/Q]` and `T[i%Q]` as the row and the column, and `!(i++%Q)` to notice
the end of each row. The test comes before the increment, so a row ends one step
later than it seems to.

12. Reading past the end of an array

The same loop reaches `T[32]` in an array of 32 elements. What this reads
depends on how the compiler places variables in memory. With the Makefile's
build it lands on `s`, which is always 0 in memory at that point, so nothing
happens. It looks like a real part of the calculation, and it can only be
explained by reading the compiled code. `simple.c` adds a spare element that
stays 0.

13. Code that does nothing

`m` is assigned and is never read for anything, and only serves to make the
`?:` look like a calculation. `r+=(h-r)/Q*Q` looks important but adds 0, because
`r` is never 32 behind `h`. Other code does real work for no result: with an
argument other than `e`, `strrchr` still runs for each byte and is ignored.
Removing these makes the rest easier to follow.

14. Misleading spacing

Spaces are placed at random, as in `(1 - h%2* 2  )*(d -f)/ (d<f  ?d|1 : 1|f) >  5?`,
and lines are broken in the middle of expressions such as `l=W[` at the end of a
line. The spacing suggests a grouping that the operators do not follow, so you
have to ignore the layout and work out the precedence yourself.
