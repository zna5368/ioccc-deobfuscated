# 1996/schweikh3

Difficulty: Easy

1. Misleading identifier names

Every variable and function is named after a programming language, so the names
say nothing about what the value means. `far` is the allocation size, `Lisp` is
the bit being tested (and later the write offset), `Basic` is the allocated
memory, `COBOL` is a start time, `PL1` is the child's exit status, `Asm` holds
the letter `m` or `c`, and `LOGO`/`BCPL` are just `unsigned long`/`size_t`. The
function `new` prints a size and the function `inline` waits for the child.
`new` is a C++ keyword and `inline` is a C99 keyword, so the program only
compiles as old ANSI C (`-ansi`).

2. Leading tokens and line splitting

Most lines start with a single token such as `;`, `,` or `:` that really
belongs to the end of the line before it. `#include` lines are split with a
backslash so the header name sits on the next line, and the `return` at the end
is split in the middle of the word as `re\` and `turn`. The backslash joins the
lines before the compiler reads them, so all of this is just hard to read.

3. Mixed up declarations, includes and macros

`#include`, `#define`, `typedef` and declarations are interleaved in a random
order. `<iso646.h>` is even included in the middle of the `for` statement, right
before the first `or_eq` that needs it. A `typedef` appears right after the
closing brace of a function.

4. Switch statements instead of if

There are no `if` statements. `switch (cond) case 1: stmt;` runs `stmt` only
when `cond` is true, and `switch (x) { ... default: ... }` is used for the
else part. The `switch` around `fork()` also declares the variable `Basic`
right after its opening brace, where no `case` label can reach it.

5. Comma operator in conditions

`switch (setbuf(stdout, 0), fork())` first turns off output buffering and then
switches on the result of `fork()`. The loop condition `far or_eq Lisp, Lisp
not_eq 0` sets a bit in the size and then checks whether the loop should keep
going. `switch ((void) wait (&PL1), WIFSIGNALED (PL1))` waits for the child
and then tests how it ended.

6. Alternative operator spellings

`<iso646.h>` lets the code write `or_eq` for `|=`, `and_eq` for `&=`, `compl`
for `~` and `not_eq` for `!=`. They look like words instead of operators, so
`far and_eq compl Lisp` really means `far &= ~Lisp` (clear one bit).

7. Constants computed from the preprocessor

The shift amounts are hidden behind predefined macros. `awk >> (__LINE__ - 1)`
depends on the line number of that code, which is 21, so it means `awk >> 20`
(megabytes). `sizeof __TIME__` is the size of the string `"hh:mm:ss"` plus its
ending zero, which is 9, so `>> (1+sizeof __TIME__)` means `>> 10` (kilobytes).
Moving the code to another line would change the result.

8. Bit tricks for the size search

`~(BCPL)0^(~(BCPL)0>>1)` is all ones XOR all ones shifted right by one, which
leaves only the highest bit of `size_t`. The loop then sets each bit from the
top down, keeps it if the allocation works and clears it if it fails, building
the largest size that can be allocated.

9. Macros that hide code

`Pascal` is `(void)printf`. `Alloc` is a whole `switch` that calls either
`calloc` or `malloc`, and is used twice. `B(C)` uses token pasting
(`SIG##C`) to make the label `case SIGKILL:` and stringizing (`#C`) to print
the name `"KILL"`, so `B(KILL) B(SEGV)` builds two whole `case` blocks.

10. Hidden letters inside strings

`Asm` is printed with `"%calloc"`, so the single letter `m` or `c` turns the
text into `malloc` or `calloc`. `Asm += 'c' - Asm` is a long way to write
`Asm = 'c'`. The message `"Thr\aashing"` hides a bell character `\a` inside the
word, and every touched byte is set to `'\a'` too.

11. Program name used as an argument

`F90[--F77][0] == 'c'` looks at the first letter of the last argument. With no
arguments that is the program name itself, so a program renamed to start with
`c` would use `calloc` as well.
