# 2001/jason

Difficulty: Horrific

1. The source code is also the data

The macro `G(g,l,k,o,e,d,n,c,i,b,a,f,j,m,p)` gets 15 pieces of the program as
arguments. Its body uses each piece twice. `a#b#c#d...` turns the pieces into
strings with `#`, and those strings are glued into one long string literal
that becomes `char *p`. The same pieces are also pasted into the body in a
different order, where they become the real functions. Run the code through
`cc -E` and you can see the program's own source text inside a string. That is
why there are only two string literals in the file.

2. Text hidden in the whitespace

The game's words are not in the source as letters. The macro
`B` is `*p++ - ' ' || !p++`. It reads one character from the string made in
technique 1. A character that is not a space means "1" and uses up one
character. A space means "0" and uses up two characters (the space and the
one after it). So the layout of the source, where the spaces and the other
characters are, holds a stream of bits. This is why the judges warn that a
code beautifier or renaming a variable (even swapping `Y` and `Z`) breaks the
game: it changes the bits.

3. A Huffman decoder written as nested `?:`

The big block of `B?B?B? ... :36: 59 * 89 *5 * 2 : ...` in `l()` is a decoding
tree. Each `B` reads a bit and picks the left or right side of a `?:`, until it
lands on a number. Codes that show up often are short. The `for` loop in
`main()` (the only loop in the program) runs this tree until the string ends.

4. Numbers that hold several letters (base 38)

Each number from the tree is passed to `h()`, which splits it into base 38
digits with `i % 38 - 1` and `i / 38` and stores one digit per byte in `M`.
For example `431*4*59*17` is many letters packed into one number. The numbers
are written as small sums and products (`5 * 5`, `6 + +17`, `82 *+ 576`) so
they do not look like data.

5. A tiny message language

The bytes in `M` form 36 messages, each ending with a 0. `o(n)` prints
message `n` with `r()`: 11 to 36 are the letters `a` to `z` (`*p + 7 + 79`),
small numbers are looked up in `"Y\n : ! ,.?>"` (newline, space, `:`, `!`,
`,`, `.`, `?`, `>`). Two codes print another message: 4 prints item name
`26 + j` and 6 prints word `j + 1`. So `"   a %s"` and `"you do not %s that"`
are built by setting the global `j` before calling `o()`. In `simple.c` the
messages are plain `printf` strings and two name tables.

6. Message numbers written as letters

`#define _(k) o(*#k - 64)` turns a letter into a number: `_(W)` is
`o('W' - 64)`, which is `o(23)`. The letters look like variable names, but
they only pick which message to print. `_(Y+ s)` is message 25, because only
the first character of the string `"Y+ s"` is used.

7. Commands found from the message table

The command letters are not written in the code. `f(n)` returns the first
letter of message `n`. `f(9)` is `'i'` (from "it is pitch black"), so typing
`i` shows the inventory. `f(5)` is `'g'` (get), and `U()` compares the input
with the first letter of "left", "forward", "right" and "back". Items are
found the same way by the first letter of their names.

8. Token pasting to build names and numbers

`#define e(a,b,c,d) a##b##c##d` and `##` in `E` and `G` glue pieces into one
word. `e(r,a,n,d)` is `rand`, `e(l,o,caltim,e)` is `localtime`,
`e(tm_h,o,u,r)` is `tm_hour`, `retur` and `n` become `return`, `f` and `gets`
become `fgets`, and `6` and `4` become the array size `64`. So no word in the
source spells the function it calls.

9. Macros that shuffle their arguments

`#define E(a,b,c,d,e) b##d,c,e,a` moves its first argument to the end and pastes
the second and fourth together. The global declarations are written in an
order that only makes sense after `E` puts them back together, for example
`E(*p=+1 ,char( m)[6 , co , 4 ],ol)` becomes
`char(m)[64], co, ol, *p = +1 + ...`.

10. Every function is one `return` with recursion instead of loops

`#define D(f) ; } int f(int i) { return` closes one function and starts the
next. Each of the 31 one-letter functions is `int f(int i)` with a single
expression. Loops are written as recursion: `Q()` and `t()` dig random
passages, `k()` places the items, `x()` and `w()` list items, `W()` lists the
exits, `J()` searches for an item, and `V()` is the main game loop
`c <= 15 && c && V(X(i))`. In `simple.c` these are `for` and `while` loops.

11. Control flow made of `&&`, `||`, `?:` and the comma operator

There is no `if` statement. `a && b` runs `b` only when `a` is true, `a || b`
runs `b` only when `a` is false, and commas run several things in one
expression. Functions such as `o()` return 0 so they can be put in the middle
of a condition. For example in `X()` the lamp check
`(!!d(0) || (a = !o(19)), b = 0)` prints "your lamp dies!" only if the cave
was lit, and is always false so the code after it still runs.

12. Global variables used for everything

`j` is used as a loop counter, an item number, a door number and the word
number for messages. `a` is "describe the cave again", but while the maze is
built `C()` also leaves the last door number in it, and `m[a ^ 2] = T(8 * 7)`
uses that to place the exit opposite the door into the last cave. `c` is the
cave number, but 0, 16 and 17 also mean "escaped", "quit" and "eaten". `s`
holds the typed letter during the game and the upper limit for random
passages while the maze is built.

13. Maze stored as door slot numbers

The maze is `m[64]`. A door is `cave * 4 + direction`, and `m[door]` is the
door at the other end. `C()` links two doors, and `H()` gets the new cave with
`i / 4 & 63` and the new facing with `i & 3 ^ 2`. Directions are relative to
the way you face, and `z()` turns them into real ones with `-~-~-~i + v & 3`.
Each `-~` adds 1, so this is `(i + 3 + v) & 3`.

14. Confusing operators and syntax

`j[m]` is the same as `m[j]`. `-~~- -!printf(...)` is always 0. `(putchar)(i)`
calls `putchar` with extra brackets. `signed(b)`, `int(s)` and `time_t(x)` put
brackets around the variable names. `03` is an octal number. `!-!N(...)` and
`!!b` turn values into 0 or 1. Smiley faces and pictures such as the fish
scales `Q)++j<` and shooting star `-~- ~ -~i +` are made of normal code.

15. Code that depends on the order the compiler works in

In `T()` the expression `(j = i*T(i-4)) + R(3) + 4` calls the random number
generator and a recursive call in one expression, and `m[a ^ 2] = T(8 * 7)`
reads `a` while `T()` changes it. C does not say which side goes first, so
another compiler can build a different maze or place the exit wrong.
`simple.c` writes out the order that gcc uses.

16. Hiding the end-of-input and time checks

`*fgets(L, 98, stdin)` reads the first character of the line in the same step
as reading it. At end of input `fgets` returns NULL and the program crashes,
which `simple.c` keeps. The lamp runs for `60 + time(0) + 60 * 4` (five
minutes), and the same number seeds `srand`. The cave is dark when
`!b + (*O - c) * *O` is not zero, which is a short way to write "the lamp is
dead, or it is neither carried (`*O == 0`) nor in this cave (`*O == c`)".
