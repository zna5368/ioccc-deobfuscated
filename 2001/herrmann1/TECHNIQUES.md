# 2001/herrmann1

Difficulty: Catastrophic

1. A program that runs in the preprocessor, many times over

`herrmann1.sh` runs `cc -E` on the file again and again until the output stops
changing, and only then compiles it. So the real program is made of `#define`,
`#undef`, `#if` and `#include` lines, not C statements. One pass cannot remember
anything, so each pass writes its results as `#define` lines at the top of the
next file. The `int main() ... printf(...)` that you can see in the source is
only the very last thing it writes.

2. The source is compressed and unpacks itself

The first lines make `n` mean `#`, `D` mean `define` and `U` mean `undef`, so
`n D f __FILE__` prints the text `# define f __FILE__` for the next pass. The
next pass gets more shorthands: `d` is `o D`, `o` is `E #F`, `E` is `Q(` and
`F` is `,)`, so `d c 0` turns into `Q( #,) define c 0`. `u`, `g`, `w` and `q`
work the same way. It takes three passes until the file is in its working form.
To see what the program really does you have to preprocess it three times and
read the result.

3. A file that prints itself (quine) by including itself

Every pass has to print the whole program again for the next pass. The file
holds two copies of its body. The first copy has real `#` directives and does
the work. The second copy is data in which every directive is written as
`Q( #,) define ...`. Near the end of the first copy it does `#define P P` and
`#include __FILE__`. The included copy sees `P` is defined, skips the first
copy with `#ifndef P`, and prints the second copy with `Q(y,z)` set to `y ## z`.
That turns `Q( #,)` into `#`, which rebuilds the first copy. Back in the outer
file, `Q(y,z)` is changed to `Q(y,z)` (it now prints itself), so the second copy
is printed unchanged. `#undef __FILE__` stops the line `include __FILE__` from
being changed into a file name when it is printed.

4. `##` with an empty argument to stop macro expansion

The arguments of a macro are normally expanded before they are put in. An
argument next to `##` is not. `Q( Q( #,),)` with `Q(y,z) y ## z` therefore gives
`Q( #,)`. This removes exactly one level of quoting, which is how there can be
lines that are quoted twice (directives that a later pass must print). The
same trick is used to pop the tape: with `X(x,y,z) z ## y`, the call
`X(I, X(O, V ,),)` gives back `X(O, V ,)` unexpanded, because `z` is always
empty. That is why every tape cell has a strange empty third argument.

5. Names that swap every pass (double buffering)

A pass cannot print `#define R <something using R>`, because the name `R` would
be replaced as well while it is printed. So there are two sets of names:
`L M R S` and `l m r s`. A pass reads one set and writes the other. Short
aliases hide which set is used: `J A B C` read and `a b c` write, and
`#ifdef s` decides which set they point to. `B` and `c` mean different things
in different parts of the file (state name, middle cell, status).

6. The tape as nested macro calls

The cells left of the head are stored as `X(I,X(O,V ,) ,)` and the ones to the
right as `Z(O,Z(I,W ,) ,)`. `V` and `W` are the two ends, and `Y(O)` is the
cell under the head. Nothing in that text means anything by itself. The program
gives `X`, `Y`, `Z`, `V` and `W` a new meaning right before each use: draw a row
of the picture (`X(x,y,z)` = `y x`), get the top cell (`x`), pop (`z ## y`), or
look up the next move (`J(_ ## x)`). Then it `#undef`s them again.

7. Building the tape from `tape="I I I"`

The tape is turned into nested calls by expanding `tape` twice. The first time
`O` and `I` are `Z(O,` and `Z(I,`, which gives the opening halves (macro `i`).
The second time both are `,)`, which gives the closing halves (macro `v`). Then
`#define R i W v` joins them. If no tape is given, it first has to
`#include` the Turing program to read its own `#define tape`, which is one of
the reasons for the extra start-up passes.

8. Printing `##` for the next pass

The line `w j k(P ## x)` prints something like `# define S(x) T(I, 0, st2 ## x)`.
In a normal text line `##` does nothing, so it is copied to the output. In the
next pass it is inside a macro definition and pastes the state name to the cell,
so `S(_O)` becomes `st2_O`, which is the user's `#define st2_O ...`. `left` and
`right` are defined as `0` and `1` just before this line, so the direction is
stored as a number.

9. Finding an undefined state from a list of parameter names

`define a(P) O&I&x` prints the looked-up move as the parameter list of a macro,
for example `# define a(I, left, st2) O&I&x`. `define c a(P)` then prints the
call `a(0, left, st2)`. If the move exists, one of `O` or `I` is a parameter
and becomes `0`. If the state is `stop`, the move is just `x` (from
`#define stop_O x`), so `x` becomes `0`. If the state is not defined, the
parameter is one unknown name and nothing becomes `0`.

10. Operator precedence in `#if`

With `O`, `I` and `x` all `3`, the test `#if c+2` reads as `0&3&3+2` = `0`
(running), `3&3&0+2` = `2` (stopped) or `3&3&3+2` = `1` (undefined state),
because `+` is done before `&`. The first value, `#define c 0 & 0`, looks like
zero, but `#if !c` is `!0 & 0`, which is also `0`. This is the README's
`#define a 0&0` / `#if !a` joke. Without a program `c` is a real `0`, which
picks the usage message. The final `return 2+c|!printf(...)` uses the same
values to return 2 (usage), 1 (undefined state) or 0.

11. Pasting tokens that are not tokens

The picture is printed inside a comment so that the next pass throws it away.
The comment markers are made with `Q( /,*)` and `Q( * ,/)`, and the state name
gets its period from `start ##x` with `x` set to `.`. Pasting `/` and `*` does
not give one valid token. Old gcc allowed it and simply printed `/*`, but
modern gcc and clang stop with an error, so the entry no longer runs. The same
period also ends the sentence "Undefined state start.".

12. Single letter names used many times

Almost every macro is one letter, and the README says there are only 53 of
them. `x`, `y` and `z` are macro parameters, then `"\n"`, `".."` and `*` while
the picture is drawn, then `3` or `0` during the status test. `O` and `I` are
tape symbols, `3`, `0`, `"O"`, `Z(O,` and `,)` at different times. `simple.c`
uses names like `LEFT_CELL`, `TRANSITION_A`, `STATE_LABEL` and `QUOTED`.

13. Hidden strings and comment art

`"Un" "define" "d state "` hides the word "Undefined" by splitting it into
pieces that C joins back together. The layout is the "modern art" from the
README: little triangles, the `StarBox`, "rain" made of `/*/*/*/*/` comments,
and fake comments such as `/* the Turing */ /* state to */` that are spread
over several macro definitions.
