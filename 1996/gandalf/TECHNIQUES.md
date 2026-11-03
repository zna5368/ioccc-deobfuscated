# 1996/gandalf

Difficulty: Easy

1. Decoy preprocessor macros

The file starts with `#define fine #define de ...`, `#define de #define` and
`#define do de`. They look like they turn `do` into a `#define` directive, but
a macro can never create a new preprocessor directive. Its expansion is plain
text in the code. None of the three macros is used anywhere in the program, so
they only exist to confuse the reader. The judges point at this with
`cc -E gandalf.c`, which shows the macros just disappear.

2. Code hidden in comments, and comments hidden in code

The "green eggs and ham" lines that use `do`, `fine` and `begin`/`end` are all
inside a `/* ... */` comment, so the compiler never sees them. The ASCII art
picture of Gandalf is another big comment, and it ends in the middle of a line
just before `char groo(p,q)`. Small comments like `/**/` and `/*_____===~*/`
are put inside the code as part of the picture, where they act like spaces.

3. Code shaped into a picture

The real program is spread over the outline of Gandalf's robe. Statements are
cut at strange places, so a `;`, a `(` or a `,` often sits alone at the start
of a line. This makes it hard to see where one statement ends and the next one
begins. Running a code formatter shows the program is only about 25 lines.

4. Meaningless names

The functions are called `groo`, `grem` and `cred`, and every variable is
`a`, `b`, `p`, `q` or `x`. In `simple.c` they are `hex_digit`,
`print_header`, `print_row`, `row`, `column`, `argc` and `argv`.

5. Hex digits from a formula

`groo(p,8)` returns `--q*q + (p/10)*q + --p`. With `q` = 8 this is
`7*7 + 7*(p/10) + (p-1)`, which is `48 + p` for `p` from 0 to 9 and
`55 + p` for `p` from 10 to 15. In ASCII, 48 is `'0'` and 55 + 10 is `'A'`.
So the formula returns the hexadecimal digit for `p`. The `7*(p/10)` part
skips over the seven characters `:;<=>?@` that sit between `'9'` and `'A'`.
This only works on an ASCII system, as the author says. `simple.c` writes it as
`'0' + value` or `'A' + (value - 10)`.

6. Undefined order of evaluation

`--q*q` changes `q` and reads it again in the same expression. C does not say
whether the second `q` is read before or after the decrement, so a compiler may
give 8*7 or 7*7. The program only prints the right digits when the compiler
picks 7*7. `simple.c` does not depend on this.

7. Program name decides the output

In `main`, the loop runs while `x < 1<<3+(int)b[0][0]%2`. `b` is really `argv`,
so `b[0][0]` is the first character of the name used to start the program.
`+` is done before `<<`, so this is `1 << (3 + c % 2)`. If the character code
is even (like `h` or `.`) the limit is 8 and only the ASCII rows 2 to 7 are
printed. If it is odd (like `c` or `/`) the limit is 16 and rows 8 to 15 with
bytes 128 to 255 are printed too. That is why `hatcat`, `cathat`, `./cathat`
and `$(pwd)/cathat` give different output even though they are copies of the
same program.

8. Misleading cast and loop increment

In `cred`, `(char)a*16+x` looks like it casts the whole sum to `char`, but a
cast only applies to the value right after it, so only `a` is cast. The loop in
`main` has an empty increment part and calls `cred(++x-1)` instead, which
passes the old value of `x` and then moves `x` to the next row. `simple.c` uses
a normal `for (row = 2; row < end_row; row++)` loop.

9. Old style function definitions and implicit declarations

The functions use the old K&R style where parameter types are listed after the
parentheses, like `char groo(p,q)int p;int q;{...}`. `main` has no return type
and no `return`, and `printf` is used without including `<stdio.h>`. Older C
compilers allow all of this, but it makes the code harder to read.
