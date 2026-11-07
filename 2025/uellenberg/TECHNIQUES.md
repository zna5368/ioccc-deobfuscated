# 2025/uellenberg

Difficulty: Hard

1. Self-reproducing program (quine)

The array `j[]` contains the program's own source text. `main()` prints every
entry of `j[]` and, at the empty entry `""` (index 17), prints the array itself
again as a C initialiser with commas, quotes and backslash escapes (function
`m()`). The output is therefore a new copy of the program. Because the code you
are reading and the text it prints are the same thing, it is hard to tell
which part is "data" and which part is "logic".

2. Program state stored in the printed source

The game state is kept in the initialisers `x a= 32 ; x b= 6 ; ...`. The odd
entries `j[1]`, `j[3]`, ... `j[15]` hold those numbers as strings, and
`n(index, value)` replaces them with `asprintf(j+o,"%i",p)`. When the output is
compiled and run, it starts with the new values. So the program "remembers" the
game between runs only through the source it prints, not through a file.

3. Output formatted as a picture

While printing the array (the `for(o=0;o<15;o++)` / `for(x u=0;u<70;u++)`
loops), a space is printed instead of the next character whenever the position
is inside the ball or a paddle. The text of the program is drawn around these
holes, so the source of the next frame looks like the playing field. The holes
are harmless because spaces and newlines in the strings are skipped when they
are printed (`w o!=32&&o!=10)`), which also means the strings in `prog.c` are
broken up at random places such as `o!=1   0`.

4. Substitute characters inside the data

Inside the strings, `$` stands for a space and `'` stands for a newline (the
`w*t== 36 )` and `w*t== 39 )` tests). Real spaces are thrown away, so this is
the only way to keep spaces such as `#define$z$else`. The numbers 36, 39 and
29 are kept in their own array entries (`"36"`, `"39"`, `"29"`) so that the
characters `$` and `'` never show up in the data where they would be replaced.

5. Single-letter macros for keywords

`#define z else`, `y return`, `x int`, `w if(`, `v putchar(`, `B v 10);` and
`A v 92);` hide the keywords and make lines like `w o==92){A y A}` hard to read.
`w` even contains the opening parenthesis of the `if`, so the code looks like it
has unbalanced brackets.

6. Single-letter names and variable reuse

Every name is one letter: `a`/`b` ball position, `c`/`d` ball speed, `e`/`f`
left paddle, `g`/`h` right paddle, `k`/`l` print cursor. Variables are also
reused for unrelated jobs: `o` is `argc` and later the row counter, `s` is first
a collision flag, then the paddle distance, then the loop counter, and `r` is a
collision flag and later "this entry is empty".

7. One variable used as a game mode

`c` is the ball's horizontal speed (-1 or 1) in pong, but any value from 2 to 6
means "jump game, stage c". The test `w c<2)` therefore decides which game is
running, and `i[c-2]` / `i[c-1]` pick obstacle heights from the table `i[6]`.

8. Scaled values

The left paddle's height `f` is stored doubled (`f/2` everywhere) so it can move
at half speed by adding or subtracting 1 each frame. This makes expressions like
`f=h*2` and `f=28` look unrelated to screen rows.

9. Magic numbers

ASCII codes are used instead of characters (`119` is `'w'`, `101` is `'e'`, `34`
is `"`, `92` is a backslash, `44` is a comma, `123`/`125`/`59` are `{`, `}`, `;`)
and game constants such as `67` (70 - ball width), `13` (15 - ball height),
`70`, `62`, and `15` appear without names.

10. Recursion instead of a loop

`m()` calls itself (`y m();`) to skip whitespace characters in the data until it
finds one it can print. Its return value is also a trick: `y!v 44)` returns 0,
while `y v 34)` returns non-zero to tell the caller two characters were printed
so the column counter `u` must skip one place.

11. Compiler generated code

The README explains the program was produced by a compiler from `prog.int`.
Function inlining, constant folding and variable merging remove the structure
the original had, leaving one long `main()` with all the game logic packed
together.
