# 2013/endoh1

Difficulty: Catastrophic

1. Program written in a different language

The bulk of `endoh1.c` is not C but SKI combinator calculus: long chains of the
letters `S`, `K` and `I` such as `S(S(K S)K)`. There are no numbers, strings or
loops in that language, so every value and every step has to be built out of
those three functions. A reader cannot tell what `K(I(I(I(S I I(S(K(S(S(K S)...`
does without working it out by hand. `simple.c` breaks the text into named
pieces such as `SUCCESSOR`, `CONS` and `PRINT_LIST`.

2. Macros that make another language compile as C

`#define S (Y(s+6))`, `#define K (Y(s+4))` and `#define I (Y(s+2))` turn each
letter into a C expression. Writing two things side by side normally does not
compile, but `S (K I)` becomes `(Y(s+6)) ((Y(s+4)) (Y(s+2)))`, which is a
function call. The C compiler therefore reads the SKI text as ordinary code, and
running that code builds the expression. `simple.c` keeps the text in a string
and reads it with a small parser, `parse_terms`.

3. Polyglot include guards

The file is split up by `#ifndef SKI`, `#ifdef SKI` and `__INCLUDE_LEVEL__` so
that it can be compiled alone or be included twice around another file. The body
of `main` begins in the first `#ifndef SKI` block and its closing
`(Y(s)))))(0));return 0;}` is in the last block, so whatever sits between them
becomes the middle of a C expression. `#` starts a comment in Lazy K, which is
why `hello.lazy` can have `#include "endoh1.c"` lines and still be valid in both
languages.

4. Token pasting macros

`A`, `B`, `C` and `D` each call the next macro six times, adding one digit to a
name with `x##0`, `x##1` and so on. `A(x)` therefore expands to 6^4 = 1296 copies
of `Z`, named `x0000` up to `x5555`. The macro `Z` is defined in the middle of the
ASCII art, then `#undef Z` and redefined as `&x##z,`, so the same `A(x)` first
writes 1296 functions and later writes the table `h[]` that lists them.

5. Closures faked with pre-made functions

C functions cannot carry data with them, but each letter must act as a function
that remembers a value. Each generated function `x0000z` is paired with its own
global variable `x0000`. Calling it with `0` returns the variable, calling it the
first time with a value stores the value, and calling it after that builds an
application. `Y` hands out the next unused function from `h[]`, and the program
prints `out of closure` if there are none left. This limit disappears in
`simple.c`, which has plain `apply` and no closures.

6. Deeply nested function pointer type

C cannot declare a function type that returns itself, which is what
`s(x)(x)(x)...` needs. The macro `p(x)` wraps a type in six layers of "pointer to
function returning", and `typedef v(p(p(p(...f...))))` repeats it until `f` is a
type that can be called 66 times in a row. Longer chains would not compile,
so the source splits them with `I(...)`: `I(a b c) d e` means the same as
`a b c d e` because `I` returns its argument. That is the reason for the
`K(I(I(I(` at the start of the program and the stray `)` in the letters at the end.

7. Overlapping data table

`v s[]={0,0,s+6,s+2,s+4,s,s+3,s+5,s+1};` is a single array holding seven
different three-field nodes at once. A node is three pointers in a row, and `s+1`,
`s+2` and so on each start one element later, so they share their fields with
their neighbours. The array was chosen with an SMT solver so that each node has
the right type in its third field. It even gives `s+3` a type of 0 with the
fields `K` and `I`, which makes it the expression `K I`, and gives `s+5` a first
field of `s`, which makes it the number zero. `simple.c` replaces this with the
`NodeType` enum and a `Node` struct.

8. Pointers used as numbers and types

`n=(w)d[2]-s` works out a node's type by subtracting the address of the array
from a pointer stored in the node. Numbers are stored the same way: `s+65` means
65, so `z(*a)+1` adds one by moving a pointer, and `z(*a)-s` turns it back into
the character to print. The pointers are never read through. `simple.c` uses the
`type` and `count` fields.

9. Generic pointer used for every type

`typedef void *v` and `typedef v *w` make nearly everything a `void *`. A node is
three `void *` values reached by index, so `*e` is the right child, `e[1]` is the
left child and `e[2]` is the type. Nothing in the source says which is which, and
`*(w*)e[1]` means "the right child of the left child". Closures are also cast back
and forth with `(g)` and `(f)`.

10. Whole evaluator in a for statement

`z` is a single `for` whose condition holds all of the reduction rules in one
nested `?:` expression with comma operators, and whose body is only
`c=b,b=a,a=d;`. Picking the argument to test is written
`*(n>1?n<4?b:a:c)`. The `d=e` at the end of the condition jumps back to the start
of the expression, which a reader would expect to be a separate loop. `simple.c`
uses a `while` to walk the expression and a `switch` with one case per rule.

11. Reused global variable

`n` has three unrelated jobs. While parsing it is the index of the next free
closure in `Y`. During evaluation it holds the type of the current node. It then
holds the character from `getchar()` and the countdown in `u` that builds the
number. Each use relies on the last one being finished, and `simple.c` gives them
separate local variables.

12. Uninitialized and leftover variables

`a`, `b` and `c` in `z` are never given starting values and keep old contents when
evaluation restarts. This only works because the sentinel `e=m(e,0)` has an empty
first field, and the program only tests the variables that are known to be set.
`simple.c` clears them on each pass.

13. Unusual array subscript

`2[*a=z(*a)+1,a]=s+5` looks like a strange array declaration. `x[y]` and `y[x]`
mean the same in C, so this is `a[2]=s+5` after running the assignment to `*a` with
a comma operator inside the brackets. `2[...]` hides that two things are happening.

14. Numbers hidden as Church numerals

A number `n` is written as a function that applies another function `n` times.
`S(S(K S)K)` adds one, and `S I I(x)` means `x(x)`, which is a power of `x`, so
`S I I(S I I(S(S(K S)K)I))` is 256 without showing any digits. The program needs
10, 32, 64 and 256 and builds them from 2 with short arithmetic. `simple.c` names
these `TEN`, `THIRTY_TWO`, `SIXTY_FOUR` and `TWO_FIFTY_SIX`.

15. Banner stored as combinators in reverse

The text to print is the run of letters at the end of the program. `K` stands for
`@`, `I` for a space and `S` for a newline. The letters are functions, so the
decoder tells them apart by applying each one to prepared arguments and seeing
what comes back. Each result goes on the front of a list, which reverses the order
and is why the data is stored backwards. `simple.c` holds the readable `banner`
string and converts it to these letters at startup.

16. Recursion without recursion

SKI has no way for a function to refer to itself. `S S(j=(g)S(S(K S)K))(K(S I I))`
in `main` looks like more arithmetic but is really a Y combinator, which feeds a
function to itself, and it is what makes the output loop repeat. `S I I` does the
same job in the banner decoder. It also shows an assignment to `j` hidden in the
middle of the expression, which saves `S(S(K S)K)` for later use by `u`.

17. Code shaped into a picture with split tokens

The code is laid out to spell the rewrite rules in large letters. To fit the
shape, tokens are split over lines with a backslash, such as `ma\` and `lloc`,
`typed\` and `ef`, and strings are split as `"out of c" "" "losure"`. Comments like
`/*ab c*/` and `/*de fghi jk*/` exist only to fill space in the picture.

18. Misleading unused parts

`./endoh1 hello.lazy` looks like an interpreter reading a file, but `main` takes
no arguments, so the file name is ignored and the banner always prints. The
leading `K` in the program also discards the input list, so the `getchar` code is
never run by the standalone program.
