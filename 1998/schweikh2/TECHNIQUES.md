# 1998/schweikh2

Difficulty: Hard

1. Hiding the string table in `__FILE__` with `#line`

`#line 10 ONE(O(1,1,2,6,0,6))` resets both the line number and the
"file name" the compiler thinks it is reading. After macro expansion the
file name becomes `"\060\061\012"`, which is the octal spelling of `"01\n"`.
From then on every `__FILE__` in the code means the string `"01\n"`, so
`__FILE__+(one&1)` points to either `'0'` or `'1'` and `1+__FILE__+1` points
to the newline. `sizeof __FILE__` is 4 (three characters plus the ending
`'\0'`).

2. Token pasting with shuffled macro arguments

The macro `O(OOO,OO0,O00,O0O,OO,O0)` glues pieces together with `##` (written
with the digraph `%:%:`) in an order different from the parameter list, so
`O(1,1,2,6,0,6)` produces `\060 \061 \012`. The parameter names use only the
letter `O` and the digit `0`, which look almost identical. `ONE(x)` then calls
`Zero(x)`, which uses `#` to turn the result into a string literal. The two
levels are needed so that `O(...)` is expanded before it is stringified.

3. `__LINE__` used as a signal number

Because `#line 10` sets the line counter, the `signal(__LINE__, ...)` call
ends up on line 14, which is `SIGALRM` on POSIX systems. Adding or removing a
line would change the signal number and break the program. In simple.c this
is simply `SIGALRM`.

4. `main` used as the signal handler

`main` is cast to `void(*)(register)` and installed as the alarm handler.
When the alarm fires, `main` is called again with the signal number (14) as
`argc`. Since 14 is bigger than `sizeof __FILE__` (4), the `switch` takes the
`case 1:` branch and runs `return One /= zero;`, which divides the flag `One`
(value 1) by 14, setting it to 0 and ending the busy loop. The same check
also makes the program exit immediately when started with four or more
arguments. simple.c uses a separate `on_alarm` function that sets the flag
to 0.

5. `switch` with a lone `case` inside a block

`switch (cond) case 1: return ...; { ... }` has no braces around the switch
body, so the body is only the single `case 1:` statement. When the condition
is false the switch does nothing and execution falls into the following block,
making it an unusual way to write an `if`.

6. Trigraphs and digraphs

`??=`, `??/`, `%:`, `<%`, `%>`, `<:` and `:>` are alternate spellings of `#`,
`\`, `#`, `{`, `}`, `[` and `]`. The braces are even mixed: the function body
opens with `<%` and the inner block with `{`, but they are closed in the
"wrong" order `%>}`. Since they mean the same thing, it still matches up.

7. Splitting `int` across lines

`in??/` followed by `t` on the next line is `in\` plus a newline, which the
preprocessor joins into `int`. Code formatters tend to indent the second line
and break the keyword apart.

8. Avoiding the word `int`

Variables are declared using only `register` or `auto` (for example
`auto one = ...`) and the cast `(void(*)(register))`, relying on old C's
"implicit int" rule where a missing type means `int`.

9. Null preprocessor directive

The `%:` alone on a line is an empty `#` directive, which does nothing. It
sits between the `main` header and its body only to confuse the reader and to
keep the line count (and therefore `__LINE__`) correct.

10. Confusing identifier names

`zero`, `Zero`, `one`, `One` and `ONE` are all different things: `zero` is
`argc` and later the bit count, `Zero` and `ONE` are macros, `One` is the
"still waiting" flag, `one` is the counter, and `ONE` is also the name of
`argv`. Whether `ONE` is the macro or `argv` depends on whether a `(` follows
it.

11. `do for ... while` loop

`do for (...; One; ++one); while (...)` is a `do`/`while` loop whose body is a
single empty `for` loop. The `for` initializer installs the handler, starts a
one second `alarm`, and sets the `One` flag all in one expression; the `for`
then spins incrementing `one` until the handler clears `One`.

12. Arithmetic with return values

`one = zero = atoi(ONE<:zero-1:>)` reads the last argument (or `argv[0]` when
there is none, giving 0). The loop condition `zero -= write(...)` subtracts
the number of bytes written (1) from the count, so it stops at zero; a count
of 0 wraps around and runs almost forever. The final
`return write(...) != 1;` prints the newline and turns a failed write into
exit status 1.
