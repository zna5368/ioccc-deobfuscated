# 2025/cesmoak

Difficulty: Catastrophic

1. Data hidden in the whitespace of the source

The program reads its own source `prog.c` as input.  After byte 465 every
space, tab, vertical tab, form feed and newline is data.  Two streams are
decoded at once: the length of each run of whitespace (1 to 4 characters)
gives 2 bits, and every group of 8 whitespace characters is read as a base 5
number that gives 18 bits.  Three 2-bit pieces or three 6-bit pieces make one
6-bit character.  The run lengths fill `B[0..218]` and the groups fill
`B[219..821]`, so the two streams join end to end.  The result is the card
drawing strings followed by a whole FORTRAN program.  This is why the
strange indentation in `prog.c` cannot be changed.  In simple.c it is the
end of `read_input()` and `whitespace_digit()`.

2. One program, four input formats, one loop

The first two bytes choose the job: `D=(S-1)*J%9` adds them and takes the
remainder by 9, so `#d` (source), two spaces (cards), `P2` (PGM image) and
` 0` (numbers) all give different values, and `D>>=2` later picks the job.
A single `while(getchar())` loop handles all four formats at the same time:
it counts dashes to find card borders, looks for the first byte of the chad
glyph, decodes whitespace and calls `atof` on every line, each guarded by a
different condition.  simple.c names the parts (`store_card()`,
`render_image()`, `run_program()`, `punch_cards()`).

3. 6-bit BCD instead of ASCII

Text is never kept as ASCII.  Card punches are turned into IBM "9 code"
values and the interpreter compares against numbers such as `27` (`.`),
`60` (`(`), `54` (`W`) and `48` (blank).  In this code `0` is the
character `'0'`, so things that look like end of string tests are not.  The
string `h` turns a code back into a character.  simple.c uses an `enum` of
`BCD_` names.

4. One lookup string with several jobs

`h` is the BCD to ASCII table, but it also holds the three bytes of the
UTF-8 glyph `▌` (spread out at positions 10, 26 and 42), the newline and
`|` used to draw cards, and the PGM header `"P%d %d %d %d "` at `h+62`.
The decoded card pieces are looked up in it, the FORTRAN number parser maps
through it, and `printf(h+62, ...)` prints the image header.  simple.c calls
it `CHARSET` and comments each part.

5. Bit twiddling punch patterns

`c(Z)` builds the 12-bit pattern of holes for a character with
`Z/16+Z/Y+!Z*4|(Z%16-9?Z/8%2<<E|(Z&7?4<<(Z&7):0):2048)`: the top bits pick
the zone rows 12, 11 or 0 and the low bits pick the digit rows 1 to 9.
Reading a card goes the other way with a search loop `r(64&&c(K)-P)`.
Chads are collected with `j[P-5]|=1<<m-1` where `m` is the line inside the
card.  In simple.c these are `punch_pattern()` and `character_for_punches()`.

6. Magic numbers that encode operators

The expression parser `g()` turns each operator into one number such as
`333`, `580`, `451` or `80`.  The low 3 bits are the precedence level, the
next 3 bits are how many extra characters to skip and the rest is the
position in a 13 way `?:` chain made of the macro `d` (`:!g--?`).  The
relational operators are found by hashing their letters: `e()%13` happens to
give 2 for `GT`, 3 for `LT`, 6 for `EQ` and so on.  In simple.c the
operators are an `enum` and a `switch`.

7. Function names chosen by a hash

A FORTRAN function call is found by `H%=28` of the name hash and a long
`?:` chain (`H-20 v 25 v 19 ...`, where `v` is `?H-`).  So 20 means `CMPLX`,
25 `AIMAG`, 19 `COS`, 15 `CABS` and so on, with no name stored anywhere.
`AIMAG` is written as `j=-I*p`, which takes the real part of `-i*z`.  In
simple.c the cases have comments.

8. Macros that hide the punctuation

`#define v ?H-`, `#define o q ==`, `#define d :!g--?`, `#define _ printf(`,
`#define t (q = *++l)` and `#define i [ K ]` turn code into short letters
that look like variable names.  `#define s [ 1 << 23],` declares a huge
array every time it appears, so `int z s x s w s X s V s A s F,...` is six
arrays.  `#define a q-33<9u|q-50<8u|q-17<9u` is "is a letter" using unsigned
wrap around to test three ranges.

9. A loop macro that declares hidden variables

`#define r(_) int K=0; for(;K<_;K++){ int N=K/S,r=K%S;` opens a loop that
always uses `K` and also creates `N` and `r` as row and column of `K` in a
grid of width `S`.  The loop body is closed by a `}` written far away, and
many loops nest with the same name `K` so the inner one hides the outer.
`r` is also a local variable in `main`, and the hidden `N` and `r` are only
meaningful in two places (the image and the card printer).

10. Variables with many jobs

Most globals change meaning between phases.  `A` holds card punches, then
the DO loop steps, then the image pixels.  `T` holds FORTRAN variables, the
input numbers (`U=T`) and the WRITE arguments.  `H`, `L`, `Q`, `R`, `O`,
`D` and `W` are each used for three or four unrelated things, for example
`Q` is the length parsed by `f()`, the column 6 punch, a bit counter and the
`INT` result.  `S` starts as `I` (which is 0 as an int), counts bytes, then
becomes the image width and finally 80.  `Z` is both the input line and the
number buffer.  simple.c gives each job its own name and only shares what
the program really depends on.

11. Shadowing and redeclaring names

The same name means different things in nested scopes: `J` is `argv` in
`main` and a counter in the inner block, `p` is a type and a variable
(`p p=...`), `g` is the function and an `int g` counter inside it, `E` is a
global constant and a local `char*E` and a local `unsigned E`, `W` is a
global `float` and a local complex, and `q`/`n`/`y`/`x` are hidden by locals
of other types inside the image loop.

12. Spiral used for two drawing styles

The same loop places points along a golden angle spiral
(`cexp((K*2.4+H*7.8)*I)`) around each dot.  With `F=0` (pen) the points
clear the pixels around a dot, giving a Poisson disk stipple; with `F=1`
(darkroom) the same code adds 1 to each pixel (`A[K]*F+F`), giving soft
round dots.  `A[K]=R%31?A[K]:R?!A[K]*6:W>0` both starts the pixels on the
first pass and inverts them on the last.  Complex numbers are used as pairs
of coordinates, and `unsigned n=1+p` then `--n` makes negative coordinates
wrap around to huge numbers so one `n<S` test checks both edges.

13. Tricky pointer and counter arithmetic

`P+=--O<0` counts the three bytes of the glyph `▌` as one column.  The card
reader writes the text of a continuation card starting at `n[-1]` so it is
glued to the previous card.  `l-=(Q<0||Z[Q]-46?0:a)+1` steps back over a
dot so that `1.EQ.2` is read as `1` and `.EQ.`.  `while(R--&&x[R]-V[m]);`
and `if(~R)` search the DO loop stack and test for -1 with `~`.

14. goto into and out of statements

`goto X` makes the binary operator loop, `goto V` runs the statement after
a true `IF(...)` by jumping back to the start of the statement code, and
`goto N` re-checks the DO loop end when nested loops share a label.  In
simple.c these are a `for` loop, a recursive call and a `do`/`while`.

15. Layout as a picture

The code is drawn as an IBM 7040 being torn apart by a black hole, so
statements are split over many lines at random places.  Words are
separated by vertical tabs and form feeds (`return\vh`, `char\tB`), which
most editors show as nothing, so the code looks like `returnh` or `charB`.
These invisible characters are also the data from technique 1.
