# 2001/herrmann2

Difficulty: Horrific

1. The source code is a stereogram

The file is a "magic eye" picture made by the program itself from
`herrmann2.ioccc`. Each line is built from parts that repeat a few columns to
the right, so `char*d,A[9876];` shows up five times on the first line and
`return _+= !a;}` six times on the last. A lot of the code is copies that only
exist to fit the picture. The extra `char*d,A[9876];` lines declare the same
globals again, and the extra `{-!har  ;` and `return _+= !a;}` lines open
blocks and add return statements that never run.

2. Filler names and expressions that do nothing

Because so few characters can be chosen freely, the author fills the gaps with
words like `te`, `wri`, `and`, `har`, `ad`, `on`, `i(long V)` and
`mai(long n,long V)`. They are unused globals or function declarations that are
pieces of `write`, `and`, `char`, `read`, `main` cut apart. Expressions like
`te-*A`, `(d==d)`, `!wri`, `a >T^l`, `o-95` and `A + te` only stand inside
comma lists and their values are thrown away.

3. The whole program is one expression

Nearly all of `main` is a single statement made of `&&`, `||`, `?:` and the
comma operator, nested many levels deep. `read(...) && ... && (...)` is used
as an `if`, `a ? b : c` chains replace `if`/`else`, and long comma lists like
`(x, y, z)` run steps in order and keep only the last value. The comma lists
are nested so that the wanted value is hidden at the end of a group of
parentheses that spans several lines.

4. One-time setup hidden in the length argument of `read`

The third argument of `read(0,&e, o||n -- +(...))` is a boolean, so it is
always 1 (read one byte). On the first call `o` is 0, so `||` evaluates the
right side, which hides the setup: `b=8` (start position), `srand(~time(0))`
(called twice), `o-=94` and `o--` (making `o` equal -95), `d=A` (an empty
string so the next new character starts at `argv[1]`) and `n -- ... n --`
(which turns `argc` into `argc-2`, so `!n` is true when exactly one argument
was given). After that `o` is never 0 again and the setup is skipped.

5. Magic numbers built from `o`, `_` and constants

The offset 95 is written as `+ +95>e`, `-95+e`, `o-=+95`, `r=e+o`, and the
random character `rand()%95 + 32` is spelled
`and%9- +7+! rand%9-85 +7+! rand%95 +7+!!  rand%95 +7+  rand()%95 +7+  r-(r+o)`.
Here `and` is 0, `!rand` is 0 and `!!rand` is 1 because the address of a
function is never 0, and `r-(r+o)` is `-o`, which is 95. The global `_` is
always 0 while characters are being made but is added to many values, for
example `c = e + _` and `A[... + _] + _`.

6. Calling a function through pointer arithmetic

`(write+(o)+b-b+*h)(1,A+b,!!-b+*h)` calls `write`. Just before it, `o=_` sets
`o` to 0, `b-b` is 0 and `*h` reads four zero bytes at the start of `A` (`h`
points to `A` as an `int *`), so the function pointer is not moved at all. The
byte count `!!-b+*h` is `1+0`. After the call `o-=+95` puts `o` back to -95.

7. Reading an array through casts

`d= *( (char**)+(long)V+ *( ... har) + ... -r+8)` looks like a long pointer
calculation, but the pieces cancel (`+r ... -r`, `*d-7 ... +8` with `*d` being
0, `*har` being 0). It is only `d = V[1]`: when the argument string runs out,
start again at its beginning.

8. Comparisons that are really simpler tests

`2-1<-95+e-r +_-1<-95+e-r ++` compares a boolean with a number:
`(depth - r > 2) < (depth - r)`. Working out the cases shows it is only true
when `r < depth`, so it means "there is nothing yet to copy from the left".
The same expression also does `r++`.

9. Recursion of `main` instead of a loop

There is no loop. After printing one character `main` calls
`main(n*n,V)`, `main(n,V)`, `main(+-n,V)` twice and `main(n,V)`: five calls
that each read and print the next character. `n*n`, `n` and `-n` are all zero
or all non-zero, so they only pass on whether `argv[1]` is used. The globals
`T` (current nesting), `a` (allowed nesting) and `w` (inside a nested call)
decide whether to go deeper. `a` grows by one each time an outer call gets
control back, so the stack only needs to be about as deep as the logarithm of
the input length. A nested call that just read a line break (`r` is 0) does not
recurse.

10. Shifts used as conditions that are always true

`b<<2+a ++ &&b<<2+a+w ++ &&b<<2+w ++` and `&&b<<2+w ++ &&b<<((...)` look
like tests, but `b` is a buffer position of at least 8 and the shift amounts
stay small, so every shift is non-zero. They exist only to increase `a` and
`w` and to make the recursive calls the right operand of `<<`. The return
value of `main(n,V)` is used as a shift amount and thrown away.

11. Variables that change meaning and dead stores

`w` is assigned five times in a row (`w= +T-->o +l`, `w= +T>o +l`,
`w=o+ +T>o +l`, `w=o+ +T>o`, `w &=o+!a`) and only the last two matter, giving
`w = T > 0`. The first of them also hides `T--`. `l` holds the random seed but
is later used in these dead comparisons. `o` is 0, then -95, then `_`, then
-95 again within one call.

12. Unsequenced side effects and implicit `int`

Globals and parameters such as `e;b;*ad,a,c;` and `mai(long n,long V)` have no
type, so they are `int` (or `int *`). Expressions like `o-- + (... o-=94 ...)`,
`n -- + (... n -- ...)` and `... -r ... r++` change and read the same variable
without a sequence point, which only works because of the order gcc happens
to use.
