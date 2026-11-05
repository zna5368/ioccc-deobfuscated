# 2015/muth

Difficulty: Terrifying

1. All of the logic lives in the preprocessor

`prog.c` has only two lines of real C: `int puts (char*);` and a `main` that
calls `puts` once. The argument is `R ( A, tape)`, a macro that expands into one
long string while the file is compiling. Reading the C code tells you almost
nothing, and a debugger has nothing to step through, because all of the work is
finished before the program exists. `simple.c` has to stay a set of macros too,
but they are named after what they do and have comments explaining them.

2. Delayed macro calls to get around the ban on recursion

A macro is not allowed to expand itself, so `K` cannot call `K`. In `QR`, the text
`I sym_(,)(O, (C, C))` works around this. `sym_(,)` expands to nothing, but it is
sitting between `I` and its parentheses when the preprocessor looks at `I`, so
`I` is left alone. The next time the text is scanned, `I` is directly followed by
its parentheses and expands to `K`, which then takes the following parentheses as
its arguments. By then the first `K` has finished, so it is allowed to run again.
`simple.c` uses the empty macro `NOTHING` to make the gap easy to spot.

3. Rescanning used as a loop

There is no loop statement in the preprocessor. `E0(X)` looks like it does
nothing because it returns its argument unchanged, but the preprocessor scans an
argument again each time it is passed along, and each scan runs one delayed call.
`E1` nests `E0` eight deep, `E2` nests `E1` eight deep, and so on up to `E9`, so
the number of scans grows by about eight times for each level. `simple.c` calls
them `EVAL_0` to `EVAL_9`.

4. Token pasting used as if statements

The preprocessor rules out `#if` inside a macro, so choices are made by gluing
names together with `##` and calling whatever macro that names. `J(Q, ...)` builds
`QL`, `QR` or `QH` from the move of a transition. `J(M, P)` builds `LP`, `RP` or
`HP` in the same way, `J(F, V)` picks a verbosity level and `J(E, X)` picks a
number of scans. `G(sym_, R)(L, (,))` builds `sym_1`, which returns its first
argument, or `sym_` for an empty cell, which returns its second. That is how the
code tests for the end of the tape.

5. Nested parentheses used as data structures

The preprocessor has no arrays, so the tape is written as pairs inside pairs, like
`(1, (1, (,)))`, which works like a linked list. Macros such as `A`, `B` and `C`
pull out a field by being placed right in front of a list, as in `B R`, so the
list itself becomes the macro's arguments. `U`, `D`, `H` and `T` take apart one
pair. This is hard to spot because `B R` looks like two unrelated names rather
than a function call. `simple.c` uses names like `TAPE_CURRENT tape`.

6. A macro used only for a hidden extra scan

`K` contains `sym_ (  K, M  (  L, B R))`. `sym_` returns its second argument, so
the `K` is thrown away and the call seems pointless. It is needed because `M`
expands to `B` followed by another macro, which is not a call to `B` until the
text is scanned again, and passing it through `sym_` causes that scan. `simple.c`
does this with a macro named `SCAN_AGAIN`.

7. A comment that swallows a macro definition

`F2` ends in `/*N,*\R)~`, which looks like a complete comment, but `*\` does not
close a comment. It carries on through the next line and only stops at the `*/`
in `F3(L,*/R`. So `F3` is never defined, the visible line `# define F3` is not a
directive at all, and the text after that `*/` is really the rest of `F2`.

8. Misleading comments

Parameter lists are full of comments like `/*  L,*/` and `/*N,*/`, as in
`QR(L, /*N,*/ R)`. They look like arguments that have been disabled and make
macros seem to take three values when they take two. Many lines also end in a
pointless `//` so that they line up.

9. A keyword redefined as a macro

`# define break  H ,` turns the C keyword `break` into a move called `H` and a
comma. Writing `(1,break)` in a machine header looks like a tuple with a C
statement in it, but it becomes the three fields `(1, H , )`. The name was picked
because it reads as "stop here". Nothing in the program uses `break` as a keyword.

10. One letter names, reused for different things

Nearly every macro is a single letter, and the same letters are used again as
parameters, as in `# define R( L, R)`, `# define A(A,  B  ,  C ) A` and
`# define Y( Y) Y`. `L` and `R` mean state and tape in most macros, but left and
right in others, and they are the move names in the machine headers as well.
`A`, `B` and `C` are also state names in those headers. `sym__(_  , __ )` and
`sym___(  ___, __)` use only underscores. `simple.c` keeps the single letters only
where they can be seen in the output, which the comment at the top of it lists.

11. Backslashes outside of strings

`F2` contains `\t` and `\n` outside of any string. They are not escape sequences
there, just a stray backslash followed by a letter. When `O` turns the text into a
string with `#W`, the backslash is copied in unchanged, and the compiler then reads
it as a real tab or newline inside the finished string.

12. Decoration that spells out the contest name

The arguments in `I (O, (C, C))` are thrown away and are only there to spell
IOCCC, which makes them look important. The parameters of `E1` to `E9` are
`_I_`, `_O_`, `_C_`, `_C_`, `_C_`, `_2_`, `_0_`, `_1_` and `_5_`.

13. Duplicate macros

Several macros have different names but do the same job, so there is more to read
than there seems to be. `Q`, `QL` and `QR` all end up in `QR`, `F`, `F0` and `FV`
all print nothing, `E` and `EX` are both `E3`, and `P` and `HP` are identical.
Some of these are needed to pick a default for `X` and `V`, but they are spread
out so that the connection is hard to see.

14. Scattered layout

The macros are arranged in columns and blocks to form a pattern, with extra spaces
inside them and big gaps between them, and they are not in any logical order. Many
are used long before they are defined. Even `main` is split up: a `# define O(W)#W//`
line sits in the middle of the `puts` call.
