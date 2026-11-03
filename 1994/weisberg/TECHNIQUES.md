# 1994/weisberg

Difficulty: Medium

1. Program logic hidden in compiler command line macros

The source file only has a `while` loop that uses the names `c`, `N`, `o` and
`e`. Those are not defined anywhere in `weisberg.c`. They are `-D` options in
the Makefile: `c` is the "check the divisor" step, `p` is the "print the prime"
step and `N` is the "go to the next candidate" step. Reading the C file alone
tells you almost nothing, and `-D` options cannot be seen by a code beautifier.

2. Unbalanced parentheses split across macros

The macros `c`, `N` and `p` each have a different number of `(` and `)`. For
example `N` is `((b=t+1),x+=2,x>e?exit(0):o` with one parenthesis left open,
and `p` and `c` end with `N)` to close it. The expressions only become valid
after all the macros have been pasted together, which is why the judges say it
confuses some preprocessors and beautifiers.

3. Whole program in one expression with `?:` and the comma operator

There are no `if` statements or blocks. The loop body is one long expression
where `a ? b : c` is used in place of `if/else` and commas join statements
together. In simple.c this became a `for (;;)` loop with `if`, `else if`,
`continue` and normal statements.

4. Initialization inside the loop condition

`while(b?o:((*(t=b=(int*)malloc(o))=2),a=t+1,o))` does the setup only on the
first test, when `b` is still `0`: it allocates the table, stores `2` in it
and sets the end pointer. Every later test just returns `o`, which is never
zero, so this is really an endless loop with the setup moved into it. In
simple.c the setup is done before the loop.

5. Dummy values and casts to fill ternary branches

Every branch of `?:` must produce a value, so the code uses throwaway values:
`47` when the table is full, `o` after moving to the next candidate, and
`(int)b++` where the cast only makes the pointer increment look like an
integer. These values are never used.

6. Constants written as shifts of `sizeof(int)`

The limit `e` is `2<<6*sizeof(int)` (33554432) and the table size `o` is
`8<<5*sizeof(int)` (8388608 bytes). Writing them as shifts hides the actual
numbers. The same name `o` is also reused both as a byte count for `malloc`
and as a count of table entries, and as the "always true" loop condition.

7. Meaningless one letter names and a hidden square root test

The names `x`, `n`, `m`, `t`, `a` and `b` give no hint about what they hold.
`m` is the smallest prime whose square has not been reached yet. When the
divisor reaches `m` the code either finds the number is prime (`n*n>x`) or that
the number is exactly `m*m`, so it skips it and moves `m` to the next prime.
This replaces a normal "divide until the square root" loop and is easy to miss.
In simple.c the names are `candidate`, `divisor`, `next_square_root`,
`primes`, `end` and `next`.
