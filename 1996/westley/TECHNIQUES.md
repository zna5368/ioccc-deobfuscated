# 1996/westley

Difficulty: Easy

1. Single-letter names that hide their meaning

`main(h,m)` calls the argument count `h` and the argument list `m` instead of
`argc` and `argv`. `h` is used both as the argument count and as the loop
counter, and `m` is used both for the program name `m[0]` and for the clock
picture `m[4]`. The names give no hint about what each value is for.

2. Old-style (K&R) function definition with implicit `int`

`main(h,m)char**m;` leaves out the return type and the type of `h`. Old C
assumes `int` for anything without a type, so `h` is an `int`. The type of
`m` is given on its own after the parameter list (`char**m;`) instead of
inside the parentheses. This looks unusual to readers used to
`int main(int argc, char **argv)`.

3. No header files

The program calls `time()` and `puts()` without including `<time.h>` or
`<stdio.h>`. Old C lets you call a function that was never declared, so the
code is shorter but the reader has to know which library functions are being
used.

4. Writing the time into the program name

`time(*m)` passes `m[0]`, the string holding the program name, as the place to
store the current time. `time()` expects a pointer to a `time_t`, but here it
overwrites the first bytes of the program name with the time value. This saves
declaring a variable, but it reuses memory that was meant for something else.

5. Loop counter that shrinks by halving

`for(time(*m);h/=2;);` uses the argument count as a counter. With 5
arguments, `h/=2` gives 2, then 1, then 0, so the loop runs twice (2 is the
hour slot and 1 is the minute slot of the time structure). The division is
also the loop test, so the counter changes and is checked in one step. The
setup part of the `for` calls `time()`, which has nothing to do with the loop
counter.

6. Empty loop body

The `for` loop ends with `;` right after the closing parenthesis, so it has no
body and does nothing useful. In the original entry (`westley.orig.c`) the
body was `m[4][m[h][h[(int*)localtime(*m)]]]=m[3][h]`, which draws the clock
hands. Without it the program only stores the time and prints the clock
picture unchanged, which is easy to miss because the loop still looks like it
does work.

7. Reversed array indexing (in the original entry)

The original entry writes `h[(int*)localtime(*m)]` instead of
`((int*)localtime(*m))[h]`. In C, `a[b]` means `*(a+b)`, and since addition
works in either order, `a[b]` is the same as `b[a]`. Putting the number
first is shorter but confusing to read. The result of `localtime()` is also
cast to `int *` so the structure fields (seconds, minutes, hours) can be
picked by number.
