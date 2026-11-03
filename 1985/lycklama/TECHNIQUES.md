# 1985/lycklama

Difficulty: Easy

1. Keywords hidden behind macros

`#define o define` turns `#o` into `#define`. That is how the original
`lycklama.orig.c` writes its macros. All the other macros then rename keywords
and functions to strings made only of `o` and `_`: `_o_` is `char`, `_oo` is
`goto`, `o_o` is `for`, `o__` is `if`, `o_` is `main`, `___o` is `write` and
`_oo_` is `read`. The preprocessor puts the real words back before the compiler
sees the code. Without them the program looks like random "tty noise".

2. Identifiers made of underscores

The variables are named `_`, `__`, `___` and `____`, and the labels are named
`_____`, `______` and `_______`. The only way to tell them apart is to count the
underscores, which is hard to do by eye. In simple.c they are `bytes_read`,
`visible`, `steps_left` and `chunk`.

3. Numbers built from bit shifts

The buffer size `__o` is written as
`(o_o_<<((o_o_<<(o_o_<<o_o_))+(o_o_<<o_o_)))+(o_o_<<(o_o_<<(o_o_<<o_o_)))`,
where `o_o_` is `1`. Working from the inside out: `1<<1` is 2, `1<<2` is 4, and
`1<<4` is 16, so the whole thing is `(1<<(4+2)) + 16 = 64 + 16 = 80`, the width
of an old terminal. The other constants `oo_` (0) and `o_o_` (1) also stand for
the file descriptors of standard input and standard output.

4. Loops written with goto

There are no `while` or `for` loops for the main logic. The code first jumps
(`goto ______`) to the `read` at the bottom. If something was read it jumps back
up to `_____` to set the step counter, and `if(--___) goto _______;` repeats the
sliding step. simple.c uses a `while` loop around the reads and a `for` loop for
the steps.

5. Work hidden inside the loop header and the ternary

`o_o(;__;_o(o_o_,"\b",o_o_),__--);` is a `for` loop with an empty body. All the
work (writing a backspace and counting down) is in the increment part of the
loop header. The number of characters to print is assigned in the middle of a
`write` call: `__=(_-o_o_<___?_-o_o_:___)` picks the smaller of "bytes read
minus one" and "steps left" and stores it in `__` at the same time.

6. Extra hidden function argument

The macro `_o(_,__,___)` expands to `write(_,__,(unsigned)(___),Z&&usleep(Z))`.
`write` only takes three arguments, but since it is not declared the compiler
accepts a fourth. Evaluating that fourth argument calls `usleep(Z)`, so every
write is slowed down. The sleep is buried in an argument that `write` ignores.

7. Missing headers and implicit int

There are no `#include` lines and `main` has no return type. Old C assumes
undeclared functions such as `read`, `write` and `usleep` return `int`, and that
a function with no type also returns `int`. The program depends on these rules
and on the build flags that turn off the warnings about them.

8. Terminal tricks

The animation is made only with the backspace character `\b` and a space. Each
step prints the text, moves the cursor back to the start with backspaces, then
prints a space. The space erases the first column and leaves the cursor one
column to the right, so the next step prints the text one column further over
and the text seems to slide off the screen.
