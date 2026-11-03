# 1995/schnitzi

Difficulty: Medium

1. Confusing variable names

The names `l`, `i`, `I` and `Ia` look almost the same in many fonts, so it is
hard to tell which one is used where. `l` is the divisor being tested, `i` is
the argument list `argv`, `I` is the number from the command line and `Ia` is
an unused third parameter (the environment pointer) that only exists so that
`I` can be declared with a pointless starting value `int I=Ia;`. In simple.c
they are called `divisor`, `argv` and `number`, and `Ia` is gone.

2. Reusing `argc` as a loop counter

`main` is called by the C runtime with `argc == 2` (the program name plus one
number). That happens to be the first divisor worth trying, so the program
reuses `argc` as the divisor and then calls `main` again with `l+1` to try the
next one. The recursion takes the place of a `for` loop from 2 up to the
number.

3. Pointer bumping on the argument list

`atoi(*++i)` moves the `argv` pointer forward by one to read `argv[1]`, and
then `main(l+1,i-1)` moves it back by one so the next call sees the original
`argv` again. This is the same as just writing `atoi(argv[1])` and passing
`argv` unchanged, which is what simple.c does.

4. Control flow hidden in `&&` and `||`

The whole program is one expression:
`l>I || fork()&&main(l+1,i-1) || I%l`. Because `||` and `&&` stop as soon as
the answer is known, this works like an `if`/`else if`/`else`: if the divisor
is too big, stop; otherwise `fork()`, and the parent (which gets a non-zero
process id) recurses into `main` for the next divisor, while the child (which
gets 0) checks `I%l` to see whether the divisor leaves a remainder. simple.c
writes this out as an `if` chain.

5. One process per divisor with `fork()`

Instead of a loop, every step calls `fork()` to split into two processes. Each
child tests one divisor and the parent goes on to make the next child. This is
why the author warns that a large number acts like a fork bomb, and why the
output order changes from run to run.

6. Crashing on purpose with division by zero

`l /= !(...)` divides the divisor by 1 when the test passed (keeping it) or by
0 when it failed. Dividing an integer by zero crashes the process with a
floating point exception, so non-divisors never reach `printf` ("Sometimes a
core dump is a feature, not a bug!"). Because dividing by zero is undefined
behaviour in C, an optimizing compiler is allowed to assume it never happens
and remove the crash, so with `-O3` every process survives and prints its
whole chain of numbers instead.

7. Using `printf`'s return value

`return printf("%d\n",l);` prints the number and returns the count of
characters written, which is always non-zero. When a recursive `main` call
returns this value to its caller, the `||` in the caller sees "true", so the
caller divides by zero too. The return value is doing double duty as both the
exit status and a hidden boolean.

8. Implicit `int` and no headers

There are no `#include` lines, `main` has no return type and the parameter
`l` has no type, so the compiler assumes `int` for all of them and for the
undeclared functions `atoi`, `fork` and `printf`. The K&R style parameter list
`char**i,**Ia;` after the parentheses gives the types of the remaining
parameters.
