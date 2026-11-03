# 1988/applin

Difficulty: Hard

1. Preprocessor directives hidden behind single letters

`applin.c` has no `#` characters in it at all. Every line is a single capital
letter such as `I a`, `U a`, `L`, `D m` or `E`. The Makefile runs `cpp` once
with options like `'-DI=#ifdef'`, `'-DU=#undef'`, `'-DL=#else'`,
`'-DE=#endif'`, `'-DD=#define'`, `'-DF=#if'`, `'-DN=#ifndef'` and
`'-DR=#include'`, which turns each letter into the directive it stands for and
writes the result to `large.c`. Only `large.c` is a real program, and it is
compiled with a second, ordinary run of the preprocessor and compiler. So the
source has to be preprocessed twice before it means anything.

2. A file that includes itself

`M` becomes `#include "large.c"`, so `large.c` includes itself fifteen times
(the fifteen `M` lines). Each copy does a little work and then includes itself
fifteen more times. The program never loops; the only repetition comes from
the preprocessor reading the same file over and over, 1 + 15 + 225 + 3375 =
3616 times in total.

3. Stopping the recursion with a counter made of macros

Something has to stop the self inclusion, and there are no variables in the
preprocessor. The macros `w`, `x`, `y` and `z` remember how deep the current
copy is: the first copy defines `w`, a copy inside it defines `x`, then `y`,
then `z`. `N z ... M ... E` only includes the file again when `z` is not
defined, so the fourth level stops. At the end of each copy the chain
`I z U z L I y U y L ...` undefines the deepest of these macros so the parent
copy sees its own depth again.

4. Numbers stored as "is this macro defined"

The thirteen macros `a` to `m` form a binary number in which a defined macro
is a 1 bit and an undefined one is a 0 bit. The long nest at the top of the
file is "add one": if `a` is defined, undefine it and carry into `b`, and so
on; the first undefined macro is defined and the carry stops. Every copy of
the file runs this once, so the counter goes up by one per inclusion.

5. Turning the bits back into numbers

`I m D n (1<<9) L D n 0 E` and the lines after it give each bit of `e`..`m` a
value (`n` to `v`), and `vv` adds them up with an extra `+1`. Because the
lowest bit is worth 2, `vv` is always odd: it is the number being tested.
`ff` does the same with the low four bits using `defined(d)*16+...+1`, giving
the odd trial divisors 1, 3, 5, ..., 31. So for each number `vv` the next
sixteen copies of the file try sixteen divisors. `F vv==1 U vv D vv 2 E`
replaces 1, which is not prime, with 2, the one even prime.

6. Trial division done with `#if`

`F ff<vv`, `F ff!=1` and `F (vv/ff)*ff==vv` are three nested `#if`s that check
whether `ff` divides `vv` using integer division in the preprocessor's own
arithmetic. If it does, the macro `dd` is defined to remember that a divisor
was found. When `ff==31`, the last divisor for this number, `dd` decides the
outcome: if it is defined it is cleared for the next number, otherwise the line
`printf("%d\n", vv);` is left in the output with `vv` replaced by its value.

7. Code that is only emitted once

`R <stdio.h>` and `main(){` sit in the `#else` branch taken only when `w` is
not yet defined, so they appear once at the very start. The closing `}` sits
in the branch that undefines `w`, so it appears once at the very end. Between
them the compiler sees nothing but a list of `printf` calls with constant
numbers, one per prime. All of the actual computing has already been done by
the preprocessor.

8. Meaningless one and two letter names

Every macro is a single letter or a doubled letter (`vv`, `ff`, `dd`). The
names give no hint that `a`..`m` are counter bits, `w`..`z` are depth flags,
`n`..`v` are bit values, `vv` is the candidate and `ff` is the divisor.

9. Limits hidden in the structure

How many primes are printed is not written anywhere. It falls out of the
fifteen `M` lines and the four depth macros: 3616 inclusions divided by sixteen
divisors per number gives 226 odd numbers, 1 to 451, so the last prime printed
is 449. Testing only divisors up to 31 is enough because every odd composite
number up to 451 has a factor no bigger than 21.
