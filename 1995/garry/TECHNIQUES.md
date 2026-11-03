# 1995/garry

Difficulty: Easy

1. Code shaped as ASCII art

The source is drawn as a picture (a rat "dropping core" according to the
judges). Statements are cut across lines in odd places, for example `if(` at
the end of one line and `*v!=92)` at the start of the next, so it is hard to
see where an `if` or `else` begins and ends. The compiler ignores all of the
layout, so reformatting the code shows the real structure.

2. Filler comments and empty statements

Many `/*  */` comments and runs of semicolons such as `;;;;` are sprinkled
through the code. Empty comments only fill out the picture, and an extra `;`
is an empty statement that does nothing. They make the code look busier than
it is. simple.c removes them.

3. Characters written as plain numbers

Characters are written as their ASCII codes instead of character literals:
`D=36` is `'$'`, `32` is a space, `92` is a backslash, and `48` and `57` are
`'0'` and `'9'`. The `$` is also hidden behind a one-letter global `D`, so
`strchr(y,D)` does not look like a search for a dollar sign. simple.c uses
`DOLLAR`, `SPACE`, `BACKSLASH` and an `IS_DIGIT` macro.

4. Short and meaningless names

Global buffers and functions have names like `y` (the input line), `z` (the
expanded output), `l` (the output length), `cb()` (expand one line), `f` (the
decoded byte) and `a`, `b`, `c` for the read, write and closing `$`
pointers. The local `c` in `cb()` also hides a global `c` used by `main()`.
simple.c gives them descriptive names.

5. Unneeded global variables

`i` and `o` are global copies of `stdin` and `stdout`, and `x` is a global
that only holds the return value of `cb()`. Using globals for these hides
where data flows. simple.c uses `stdin`, `stdout` and a local variable.

6. Deeply nested if/else instead of early exits

Decoding an escape like `\101` is written as a chain of nested `else`
blocks, with the last digit squeezed into a `?:` expression:
`*b++=((*v<48||*v>57)?f:((f<<3)+(*v++)-48))`. Each digit shifts the value
left by 3 bits, which is the same as multiplying by 8 (reading octal). simple.c
moves this into its own function `copy_value()` and uses `continue` so each
case is handled at one level of nesting.

7. Dead decoy code

The function `tng()` at the end is never called. It contains nonsense such
as `if(x==c||c!=x)`, which is always true, and `strcpy(&D,"(C)1995")`,
which would copy a string into a single `char` and overwrite memory if it
ever ran. It only exists to confuse the reader and carry the author's name
in a comment. simple.c leaves it out.
