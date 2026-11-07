# 2025/ncw3

Difficulty: Hard

1. Invisible Unicode characters used as macro names

C compilers allow many Unicode characters in identifiers, including ones
that print as nothing.  The program defines macros named with zero width
characters: U+200B (zero width space) becomes `(void) {` and opens a
function body, U+200C (zero width non-joiner) becomes `();` and calls the
word in front of it, and U+200D (zero width joiner) becomes `;`.  Other
invisible characters push values: U+2060 becomes `*--_=` (push a
variable), U+2061 becomes `(*--_).i=` (push a number), U+2062 becomes
`(*--_).s = 1+` (push a string) and U+FEFF becomes `(T.i);` so that
`cos ﻿` reads like a call on the top of the stack.  In an editor the
source looks like plain FORTH with spaces, but the compiler sees normal C.
simple.c uses ordinary C syntax with no hidden characters.

2. Fullwidth look-alike characters

The FORTH words `:`, `;`, `+`, `-`, `*`, `/`, `<`, `=`, `!`, `@`, `.`,
`>R`, `R>` and `2DUP` are written with Unicode fullwidth forms such as
`：` (U+FF1A), `；` (U+FF1B), `＋` (U+FF0B) and `＠` (U+FF20).  These look
like the real punctuation but are actually letters as far as C is
concerned, so `： ＋ ​ O(+) ；` is the definition of a function whose
name is `＋`.  `：` is a macro for `void` and `；` a macro for `}`.
simple.c replaces every one of these with plain arithmetic.

3. A FORTH interpreter built from C functions

Every FORTH word is a tiny `void` function that works on a stack array.
`_` is the data stack pointer and `__` the return stack pointer, both
inside the global array `M`, and the macros `P` (pop) and `T` (top of
stack) hide the pointer work.  `O(o)` builds the binary operators,
`DUP`, `DROP`, `OVER`, `SWAP`, `２DUP`, `＞R` and `R＞` shuffle values,
and `．`, `EMIT` and `TYPE` print.  The real program (the Mandelbrot code,
the colour palette and the frame loop) is written as long chains of these
calls in reverse Polish order, for example `⁠ u ‍ ＠ ‌ DUP ‌ ＊ ‌` for
`u*u`.  To read it you have to track the stack by hand.  simple.c turns
each chain back into normal expressions with local variables, so the
stack and all the word functions are gone.

4. Macros that look like FORTH control words

`DO`, `LOOP`, `LEAVE`, `IF`, `THEN` and `VARIABLES` are C macros.  `DO`
pops a start and a limit and opens a `for` loop with an index named `I`
(the inner loop hides the outer `I`), `LOOP` closes two braces, `LEAVE`
is `break`, `IF` pops a value and opens an `if`, and `THEN` is `}`.
simple.c uses `for`, `if` and `break` directly.

5. Variables as pointers into a hidden memory array

`VARIABLES` expands to the union type `E`, and the invisible separator
U+2063 after each name becomes `={.e=&M[__COUNTER__]},`.  `__COUNTER__`
is a preprocessor counter that goes up by one each time it is used, so
`u`, `v`, `j`, `U`, `V`, `X`, `Y`, `p`, `q`, `s` and `Q` each become a
pointer to the next slot of `M`.  Reading and writing them needs `＠`
(fetch through the pointer) and `！` (store through the pointer).  The
last name `K` is declared but never used.  simple.c uses normal `double`
variables with real names like `zx`, `step_x` and `color_phase`.

6. A union so one stack holds numbers, strings and addresses

`union E { double i; char *s; union E *e; }` lets the same stack slot be a
number, a string or a variable address depending on which member is
read.  Even the loop index and characters for `EMIT` are doubles, and
comparisons such as `400 < I` produce `0.0` or `1.0` that are then
multiplied into the zoom factor.  simple.c gives each value its proper
type.

7. String literal offset trick

The string pushing macro adds `1+` before the literal, so
`⁢" \033[2J\033[H"` skips the leading space that was only there to make
the source look like the FORTH `" text"` syntax.  simple.c passes the
strings directly to `fputs` and `printf`.

8. Meaningless one letter names

The words are named `S` (print a `;`), `L` (print one coloured cell),
`C` (one colour channel), `c` (the palette), `m` (Mandelbrot iteration
count), `d` (step one pixel down) and `l` (draw a frame).  simple.c
calls them `print_cell`, `channel`, `palette`, `mandelbrot` and
`draw_frame`, and gives the magic numbers (800 frames, 120 columns,
40 rows, 100 iterations, 16000 microsecond delay) named constants.

9. Escape sequences and a half block character for graphics

The output is built from ANSI escape codes: `\033[38;2;R;G;Bm` and
`\033[48;2;R;G;Bm` set 24-bit foreground and background colours, and the
`▀` upper half block is drawn in the foreground colour over the
background colour so each character shows two pixels.  In the original
the three colour numbers are printed one at a time with `．` and `S`,
and come off the stack in reverse order.  simple.c
prints each colour with one `printf` and comments what each escape code
does.
