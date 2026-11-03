# 1991/rince

Difficulty: Easy

1. Graphics packed into one string with hidden NUL bytes

`*s="######\0######\0######\0      \0..."` is one long string that holds
the pictures of all 15 tile types. Every line of a picture is 6 characters
followed by `\0`, so each line takes exactly 7 bytes, and every tile has 3
lines. The drawing code finds a line with the arithmetic
`s+((tile*3+a)*7)`: skip `tile*3` lines to reach the tile, then `a` more
lines, then multiply by 7 bytes per line. Because the `\0` ends each piece,
`mvaddstr` prints just that one line. In `simple.c` this became a
`tile_art[15][3]` array of separate strings.

2. Map stored as letters that are also array indexes

The level `M[12][22]` is written as rows of letters such as
`"ALCABBBBBBBABBCBBCABBA"`. Nothing says what the letters mean: `A` is a
wall, `B` is empty floor, `C` a hollow boulder, `L` a diamond and so on.
The letter minus `'A'` is used directly as the tile number in the picture
string. Each row is exactly 22 characters, so the array has no room for the
string's ending `\0` and C silently drops it. `simple.c` gives every letter a
`#define` name like `WALL` or `DIAMOND`.

3. Three nested loops squeezed into one `for` with an empty body

The screen is drawn by
`for(i=j=a=0;j<7;!(mvaddstr(...),a^3)&&(a=0,!(++i^12))&&(a=i=0,j++));`.
All the work happens in the "increment" part of the `for`. `a` counts the
3 lines of a tile, `i` the 12 tile columns and `j` the 7 tile rows. The
`&&` operator only evaluates its right side if the left side is true, so it
works like a chain of `if` statements: "if `a` reached 3, reset it and step
`i`; if `i` reached 12, reset it and step `j`". `x^n` (exclusive or) is zero
only when `x == n`, so `!(a^3)` means `a == 3`. The comma operator makes
`mvaddstr` run first but throws away its result.

4. Lookup tables instead of `if` statements for the keys

`*k="hjkl"` and `strchr(k,getch())` turn the pressed key into a direction
number `d` by finding its position in the string (`t-k`). The same number
then indexes `X[]={-1,0,0,1}` and `Y[]={0,1,-1,0}` for the step to take and
`O="MNOH"` for which way the player picture should face. The order of the
letters is the only thing that ties these four tables together.

5. Hidden player position

The player is not drawn separately: its picture letter (`M`, `N`, `O` or
`H`) is written into the map just before drawing and replaced with `B`
(empty) right after. The starting `N` hidden in the fifth map row is where
the game begins, which is why `x=y=4`.

6. Hard to read coordinate juggling

`f=(e=x+X[d])+X[d];h=(g=y+Y[d])+Y[d];` sets `e,g` to the square next to
the player and `f,h` to the square after that, all inside one expression.
The code then works on `M[g][e]` and `M[h][f]` (the original used the
macros `W` and `U` for these), so pushing a boulder becomes `U=W`.
`simple.c` names them `next` and `beyond`.

7. Magic numbers and unused variables

`D=39` is the number of diamonds to collect, `u=484` the number of moves
allowed, `((y-1)/5)*5` and `((x-1)/10)*10` pick the top-left corner of the
visible part of the map, and `j*3+a`, `i*6` turn tiles into screen
positions. The variable `m` is set twice but never read.

8. Repeated code generated with macros (original version)

In `rince.orig.c` the five almost identical `case` blocks for pushing
boulders and keys were written with the macros `z(a)` and `Z(a,b)`, and the
`#` and `##` preprocessor operators built the wall and arrow lines of the
picture string (`K(k(#))` makes 18 `#` signs). Even `d[X]` is used instead
of `X[d]`, which is legal because `a[b]` means `*(a+b)`.

9. Implicit declarations and `int` return type

`main()` has no return type and `strchr` is used without including
`<string.h>`, relying on old C rules that assume an `int` result. The cast
`(char*)strchr(...)` hides the mismatch.
