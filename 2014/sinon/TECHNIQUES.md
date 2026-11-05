# 2014/sinon

Difficulty: Hard

1. Meaningless identifiers

Nearly every name is one letter, and many differ only by case: `o`, `O`, `p`,
`P`, `m`, `M`, `n`, `N`, `z`, `Z`. Nothing says that `l` is the elapsed time,
`M` is the damage dealt or `z` is the function that appends text. The array
`o[]` is five unrelated values packed together: the start time, the last firing
time of each weapon and the shot count of each weapon. The reader has to work
out from the `o[O+1]` and `o[O+3]` indexing which is which.

2. Source code drawn as a picture

The file is laid out as a portrait of the character Sinon. Lines are cut
wherever the outline needs them, so strings, keywords and expressions are
broken at random spots. The line breaks and indentation say nothing about the
structure of the program.

3. Hiding the include directive

Line 1 ends with `#/*` and line 3 starts with `*/include<stdio.h>`. A comment
counts as a single space, so the compiler sees `# include<stdio.h>` even though
the words `#` and `include` are two lines apart. A search for `#include` in the
file finds only the copy inside a string.

4. One file that is both C and Perl

`perl prog.c` runs the same file as a Perl script. Perl treats line 1 as a
comment and `q{` on line 2 as the start of a quoted string that runs to the
`}}` on line 17. In C, that closing part is inside a `/*...*/` comment that
holds the Perl code. The Perl prints a shell script that plays the game by
itself, and `__DATA__` tells Perl to ignore everything after it, which is the
rest of the C code. The reader must track two languages at once to see which
text belongs to which.

5. Program stored as a string of itself

`q(*s=__TIME__;extern void w()...)` looks like a function call, but `q` is a
macro whose `#b` turns its whole argument into a string. `b` is also pasted back
as normal code. So the text between the brackets is compiled and stored as
the string `p`, without being written twice. This lets the program print its own
source code, and it also means most of the program is the argument of a macro.

6. Game state stored in the source code

The program keeps no save file. It prints a new C program in which the numbers
in `int o[]={0,0,0,0,0}` are replaced, and that output is compiled to take the
next turn. Reading one run of the code is not enough, because the meaning of
`o` depends on the runs before it. The first run also differs from later ones
because `o` starts at all zeros.

7. Compiler optimization used as program input

`(&x-&w)-(&y-&x)?1:0` picks the weapon. `w`, `x` and `y` are never called, and
only their addresses are used. Subtracting two function addresses gives the
distance between them in memory, which here is the size of the code in between.
`w` holds an empty loop that does nothing visible. An optimizer removes that
loop, so the functions come out the same size, while an unoptimized build keeps
it and `w` is larger. The result comes from how the compiler lays out the code,
so reading the source alone cannot tell you the answer, and the C standard does
not define this subtraction at all.

8. Compile time used as program input

`__TIME__` is a text constant the compiler fills in with the time of the build,
such as `"11:22:08"`. The program reads the time the code was compiled, not the
time it runs, so the build is what acts as the clock. The characters are used
directly instead of being converted one digit at a time. Each digit carries an
extra 48, the character code of `0`, and all of those extras are removed at once
by subtracting the unexplained number `1933008`.

9. Character substitution in strings

`z` copies a string while subtracting `061` from any `;` or `Q`. That turns `;`
into a newline and `Q` into a space, so `";Time:Q"` really means `"\nTime: "`.
The strings therefore contain no real spaces or `\n` escapes, which makes the
display text much harder to read.

10. Compressed win picture

The string `N` holds the victory screen packed with run-length encoding. A
character above 92 means "repeat the next character", so `dQ` is ten spaces. Any
other character gives the length of a run of plain text, so `=/*` is the two
characters `/*`. The decoder works inside the string itself: it slides each
piece left to make room for a terminating zero, and it uses the first two bytes
of `N` as scratch space.

11. Disguised literals

Numbers and characters are written in unusual forms to hide their value. There
is octal such as `061`, `040`, `%03` and `o[01]`, and escapes such as `\40`,
`\x20`, `\12` and `\161`, which is the `q` of `#define q`. Strings are chopped
into pieces like `"Q(j" "a" "m" "me" "d" ")"`, which C joins back together. The
statement `return+0` is simply `return 0`, and `59==*d` is `*d==';'`.

12. Variables reused for unrelated jobs

`Z` starts as the time of day and is then overwritten with the number of the
jammed weapon, so `Z?"":"Q(jammed)"` tests a value that is usually a clock
reading. `O` is the weapon, and later a padding counter or a run length. `s` is
the compile time, and later a pointer into the output or into `N`. `p` is the
program text until it is reused to walk through `N`, and `z` has a parameter
also called `p` that hides the global.

13. Macro that hides declarations

`G(Z,60*(...))` looks like a function call, but `G` expands to `int Z=...;`,
with the semicolon included. All eight local variables of `main` are declared
this way and run together without any visible `int` or `;`. The second macro
parameter is named `o`, the same as the global array, which adds to the
confusion.

14. Work done inside loop headers

Most loops have an empty body. In `for(z(";Time:Q");(l++<60);z("#"));` the label
is printed by the setup part and each `#` by the update part, and the test
changes `l` as a side effect. The bars also count `o[3]` and `o[4]` upward past
25, which destroys the values, so the code only works because they are not
needed afterwards. A similar loop in the grid erases a face by writing
spaces over it in the output.

15. Number sequences that replace stored data

The enemy grid is not stored anywhere. `m=(97+(67*m))%198` and
`n=(n*193+287)%384` produce a fixed sequence of scrambled numbers from the start
time. `m` decides when each enemy dies, and `n` picks the spacing and the face.
The formulas give no hint that `198` is the 18 by 11 grid size, or that `m`
reaches each enemy number exactly once.

16. Display hidden inside a comment

Everything the player sees is wrapped between `/*` and `*/`, so the printed
screen is also a valid source file. The closing `*/` is pushed out to the far
right by 77 spaces so that it stays out of the picture. Without noticing this,
the output looks like it could not possibly compile.
