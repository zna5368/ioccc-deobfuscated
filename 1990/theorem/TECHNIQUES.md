# 1990/theorem

Difficulty: Hard

1. Palindromic source (reversible file)

The file is built so that it still compiles when its lines are put in reverse
order. The top of the file opens with `#include` lines and `#define`s, and the
bottom of the file ends with the same `#include` lines and an opposite set of
`#define`s. When the program is run with `-r`, it prints its own source with the
lines reversed, which becomes `sorter.c`. Every line was written so that it does
something useful when read top to bottom, and something different when read
bottom to top.

2. Swapped brace macros

`#define o {` and `#define C }` make `o` and `C` stand for open and close braces
in the forward file. The bottom of the file has `#define o }` and `#define C {`,
which are the macros the reversed file sees first. So the same letter is an
opening brace in one program and a closing brace in the other, which lets one
set of lines form valid blocks in both directions.

3. Macros that swap function bodies

`X` and `Y` are defined as nothing or as an empty body `{}`. `X` sits on the
line before `K(...)`, and `Y` sits after the second `main(A,a)` header. In the
forward file `Y` turns that second `main` into an empty function, and in the
reversed file `X` does the same to a different function. This decides which
code is a real function and which is a dummy in each version.

4. Renaming main with the preprocessor

`#undef main` near the top and `#define main L` in the middle of the file mean
that only the first `main` keeps its name. The second `main(A,a)` header lower
down is quietly renamed to `L`, an empty function that is never called. In the
reversed file the order of these lines flips, so a different `main` is the real
one.

5. Dead code hidden after loops and returns

Most of the lines in `main` and `e()` never run in the theorem program. For
example `while(T=0);` never loops because the assignment gives 0, `while(W!=1)`
never runs because `W` was just set to 1, and everything after `return O;` in
`e()` is unreachable. These lines are the shell sort that becomes live when the
file is reversed into `sorter.c`. They are written backwards (the loop body
comes before the loop header) so they read correctly once reversed.

6. Comment tricks

`/*/` is both the start and the end of a comment depending on which side you
read from. In the forward file the line `/*/` opens a comment and the `*/` at
the end of the next line `z;/*/` closes it, so `z;` is hidden. In the reversed
file `z;` comes first and is real code (it declares the variable `z`), and the
comment then runs from one `/*/` to the other. `/**/` at the start of a
line is an empty comment that just pads the line so it lines up the same way in
both files.

7. Unused functions for a third and fourth program

The function `w()` prints a Fibonacci sequence but is never called in theorem.
It only becomes `main` after the file is run through the sorter, which sorts the
source lines alphabetically and so moves the lines into a new order. The
judges' Fibonacci program comes from sorting the same file.

8. Single letter names and duplicate globals

Variables such as `D`, `T`, `B`, `M`, `G`, `U`, `V`, `N` hold the current x,
the end of x, the step, the current y, half the step and the Runge-Kutta slopes.
The globals are declared twice, once at the top and once at the bottom, in a
different order, so that each direction of the file has its own declarations.
Local parameters `A` and `a` in `main` hide the global `A` and `a` with the same
names.

9. Old style (K&R) function definitions

Functions are written as `K(g,R,u)float*g,u;char R;` with the parameter types
listed after the parameter names and no return type. This is valid old C but
hard to read, and the `R` parameter hides the global `R`.

10. Side effects inside expressions

`U=e((G=B/2,*a),D,M,a);` uses the comma operator to set `G` (the half step)
while passing the expression string. `e(*a,D+=B,M+G*N,a)` moves x forward by a
full step in the middle of the y update. `M=atof(*(4+(a-=3)));` moves the
argument pointer back by three while reading the fifth argument, so that `*a`
points to the expression for the rest of the program.

11. Hidden Runge-Kutta formula

The fourth order Runge-Kutta step is spread over four lines and rearranged.
The third slope is stored already doubled (`N=2*e(...)`), so `M+G*N` is really
`y + step*k3`, and the update `B*V/3+B*N/6+B*k4/6+G*U/3` is the usual
`step*(k1 + 2*k2 + 2*k3 + k4)/6` with the terms reordered and the halves and
doubles folded into different variables.

12. Array index reversal

Expressions like `j[m]`, `i[m]` and `b++[m]` use the fact that `a[b]` and
`b[a]` mean the same thing in C, so `j[m]` is just `m[j]`, the j-th line.

13. Left to right expression evaluator

`e()` reads the expression one character at a time with no precedence rules.
Each `x`, `y` or digit is combined into a running total with the last operator
seen, using `K()`, and `^` calls `P()` which multiplies in a loop to raise to
an integer power.
