# 2000/anderson

Difficulty: Hard

1. Recursion instead of loops

The program uses no keywords except `char`, so there is no `for`, `while` or
`if`. Every loop is a function that calls itself at the end, for example
`b(){(j(A=...),++x<Y)&&b();}` prints one character and then calls `b()` again
while `x` is less than 8. `l()`, `X()`, `B()`, `t()`, `R()`, `O()` and `C()` all
loop this way. In simple.c these are ordinary `for`, `while` and `do`/`while`
loops.

2. Short-circuit operators instead of `if`

`a && b` only runs `b` when `a` is true and `a || b` only runs `b` when `a` is
false, so they can stand in for `if`. `A||X(x=0,g=J)` means "if A is zero, call
X". In `B()` a long chain of `cond && (action) || cond && (action) || ...` is an
if/else-if ladder that picks how to translate one input character.

3. Comma operator and unused function arguments

Each function is one statement, so many steps are joined with commas. Work is
also hidden inside the arguments of calls to functions that take no
parameters: `C((B(g=K[2]),*r=...,O()))` runs all of that work first and then
calls `C()` again, and `j((r[2]=0,R()))` calls `R()` and then prints a newline.
The arguments themselves are thrown away.

4. One string holds every table

The string `T` holds four tables. The first 28 characters are the semaphore
codes, with the left arm position in bits 0-2 and the right arm position in
bits 3-5 (bit 6 is set only so the character can be typed). The rest is reached
through pointers set in `main`: `Q=T+28` holds the arm characters `\|/_`,
`M[1]=T+32` and `M[0]=T+48` hold the flag characters `<[` and `>]`, `k=T+34`
holds where each flag goes and `F=T+41` holds where each arm goes. Those
positions are stored as characters with 32 added, so `F[x]-W-x` and `k[x]-W`
turn them back into numbers. simple.c spells out each table by name.

5. All pointers set up in one nested expression

`main` sets the numbers `W=32`, `N=4`, `Y=28` and `q=28` and every pointer into
`T` in a single nested assignment: `Q=T+(q=(Y=(W=32)-(N=4)))`, then `M[1]=Q+N`,
`k=M[1]+2` and so on. `Y` is later reused as 8 (`Y=N<<1`) for the number of
columns and pictures per line.

6. XOR used to mirror the drawing

Only the left arm is stored. The right arm is drawn with the same table and
then mirrored with XOR. `index ^ 7` flips the column inside the 8 wide row,
`Q[x&3]^'s'` swaps `\` and `/` (because `'\\'^'s'=='/'`), and the flag is moved
back one cell with `- A`. `A` is 0 for the left arm and 1 for the right, so
`A*7` and `A*'s'` are zero for the left arm and the left arm is left alone.

7. Odd ways to index arrays and set variables

C lets `a[b]` also be written `b[a]`, so the code uses `x[F]` for `F[x]`,
`0[K]` for `K[0]`, `A[M]` for `M[A]` and `(*r)[r]` for `r[*r]`. Some indexes
also have side effects, like `M[T+=A,1]`, which is just `M[1]` but moves `T`
to the next code. `X()` sets `r[1]`, `r[2]` and `r[3]` in one line through
nested indexes, `r[r[r[3]=...,2]=...,1]=...`. Then `r[x>>3]` uses the row
number to pick that row's body character. Other tricks are `*r=!-A` (which
sets 1 because A is 0), `g-=-q` (which means `g+=q`) and `!(D-W&&D-9&&...)`,
which checks whether D is one of several characters.

8. Variables reused for different jobs

The few global variables mean different things at different times. `r[0]`
remembers whether the last character was white space, `r[1]` is the number
of codes left to print, and `r[2]` is the current row. When the pictures are
built, `r[1]`..`r[3]` hold the body characters. `A` is the arm side when
building and the character to print when printing. `q` counts the recursion in
`l()` and later is the picture index. `K[0]` holds all 28 drawn pictures and
`K[2]` holds the input line, which is turned into codes in place.

9. Pictures drawn into a flat buffer

Each picture is 32 bytes in `K[0]` (4 rows of 8). `X()` fills it with the
body by checking a bit pattern of the column (`(((x&7)-1)>>1)-1` is zero only
for columns 3 and 4). `b()` reads `(*K)[D*W + r[2]*Y + x]`, which is picture D,
row `r[2]`, column x. simple.c uses a three dimensional array and a template
for the body.

10. Replacing the input while reading it

`B()` writes the translated codes into the same buffer it reads from. A digit
gives two codes (the numeral sign and a letter), so the writes can get ahead of
the reads and overwrite characters that have not been read yet. That is why
`echo 12` shows only the numeral sign and "1". simple.c keeps this behavior on
purpose so its output stays the same.

11. Implicit int and missing return types

None of the functions have a return type or a `return`, so they default to
`int` (old K&R C), which the compiler warns about. That is how the program
gets by without the `int`, `void` or `return` keywords.
