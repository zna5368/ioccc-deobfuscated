# 1987/wall

Difficulty: Hard

1. Control flow through signal handlers

No function except `ii` is ever called directly. `m(x)` is
`signal(X/'I', vi[x])`, and `X/'I'` is `1000/73`, which is `13`, the number of
`SIGPIPE`. So `m(x)` only says "the next time SIGPIPE arrives, run `vi[x]`".
`ii` creates a pipe with `pipe(VI)` and then closes its read end with
`close(*VI)`. `main` then loops forever writing to the write end
`VI[1]`. Writing to a pipe that nobody can read from makes the operating system
send `SIGPIPE`, so every write runs whatever handler was chosen last. Each
function does a little work, calls `m()` to pick the next function, and
returns, so the program is really a state machine that moves one step per
failed write. The `for(;v)write(VI[I],V,M));` loop in `main` looks like it
writes `V`, but the data never goes anywhere.

2. A table of function pointers chosen by arithmetic

`vi[]` lists 21 functions, some of them repeated (`xx` four times, `MV`
twice) and some never used (`l` and `d` at indexes 5 to 8). The index passed
to `m()` is computed, not written out: `m(cm + !isatty(i = I))` picks index 2
or 3 depending on whether output goes to a terminal, `m(4 - !(i & I))` picks
`exit` or the same function again, and `m(c/M/M + M)` uses the hundreds digit
of a number (plus 10) to pick which Roman numeral pattern to print. The
repeated entries are what make digits 5, 6, 7 and 8 all go to the same
function.

3. The signal number used as an argument

When the system calls a signal handler it passes the signal number. One of
the entries in `vi[]` is `exit`, so when it is reached through `SIGPIPE` the
program ends with `exit(13)`. The README's hint about `signal` being used
"with and without an arg" is about this: elsewhere `(vi[iv])()` calls `exit`
directly with no argument at all, so the exit status is whatever happens to be
in the argument register.

4. Misleading names made only of Roman numeral letters

Every identifier (`i`, `ii`, `iv`, `v`, `vi`, `VI`, `XI`, `xi`, `xv`, `LV`,
`LXX`, `MM`, `MC`, `ml`, `md`, `mi`, `X`, `M`, `I`, `D`, `c`, `cc`, `cm`, `l`,
`lx`, `xl`, `xx`, `ix`) is spelled with Roman numeral letters, and the
upper and lower case versions are different things (`vi` is the function
table, `VI` is the pipe, `xi` is a lookup table). The names hint at numbers
that have nothing to do with what the variables hold.

5. Macros that hide syntax

`#define v ;(void` means `v)write(...)` is really `;(void)write(...)`, which
hides the statement separator inside a letter and leaves a parenthesis that
looks unbalanced. `XI(xi)` declares an `int` array, and `L(c,l,i)` builds a
whole function `c(){d(l);m(i);}` that prints something and then chooses the
next state, so many functions are defined without a visible body.

6. Numbers hidden in character constants

`'\d'` is not a real escape and old compilers read it as `'d'`, which is 100,
so `X='\n'*'\d'` is `10*100 = 1000`. `M='\n'` is 10, `ix='\t'` is 9,
`iv*'V'` is `4*86 = 344` (the array size), `D['x']` is a 120 byte buffer,
`gets` reads at most `'x'` characters, `'d'` (100) is added in `MV`, and
`'\f'` (12) is a state number. `I` starts at 0 and `ii` does `++I` so it is
used as the number 1 from then on, and `cm` becomes 2.

7. A lookup table built by an encoded string

The Roman digit values are never written down. `ii` walks the string
`"jWYmDEnX"` two characters at a time, storing 1, 5, 10, 50, 100, 500, 1000,
5000 into `xi[]` at both the upper and lower case of each letter (`*V ^ ' '`
flips the case because upper and lower case ASCII letters differ only by the
value 32, which is `' '`). Each letter is one after the real Roman digit
(`j` for `i`, `W` for `V`, `Y` for `X`, `m` for `l`, ...), and the table is
then read through `cc = xi + 1`, which shifts everything back by one so that
`cc['i']` is 1 and `cc['v']` is 5. The same table also gives special values
to the newline (`cc[M] -= M`, giving -10) and to `EOF` (`cc[-I] -= ix`,
giving -9). Those negative numbers end the reading loop and, after negation,
are printed as the line ending: `-(-10)` is a newline and `-(-9)` is a tab.

8. Pointer arithmetic on strings

`V="ivxlcdm"+iv` points at `"cdm"`, the one, five and ten letters for the
hundreds. `MM` does `V-=cm` to step back two letters to `"xlcdm"` and then
`"ivxlcdm"`, so `V[0]`, `V[1]` (`V+I`) and `V[2]` (`V+cm`) are always the
letters for the current decimal place.

9. Doing one step at a time instead of loops

The Roman numeral for each digit is produced by bouncing between states
rather than with a loop. For the digit 3, `ml` prints a letter and goes to
`MC`, which prints a letter and goes to `md`, which prints a letter and goes
to `MM`. For 4 and 9, `MV` prints the one letter and adds 100 to the number
so that it becomes a 5 or a 10, which then prints the five or ten letter.
For 5 to 8, `xx` prints the five letter and subtracts 500 to leave 0 to 3.
`MM` then multiplies by 10 modulo 1000 to move the next digit into the
hundreds place.

10. Conditional expressions instead of if statements

`LV` packs its whole decision into one statement:
`i?(c?(c<i&&l(-c-c,"%d"),l(i,"+%d")):l(i,"(%d")):(c&&l(M,")"),l(*D,"%c"))`.
It uses `?:`, `&&` and the comma operator in place of `if` and `else`. The
`-c-c` prints minus twice the previous digit, so `iv` turns into
`(1-2+5)`: the 1 is first added, then subtracted twice. Similarly
`gets(D)||(vi[iv])()` means "if reading fails then exit".

11. Arguments that do double duty

`d(x)` is `write(i,x,i)`: the global `i` is set to 1 by `li`, so it is both
the file descriptor (standard output) and the number of bytes to write. `l`
takes its arguments in an unusual order, value first and format second, and
is sometimes given a value that the format ignores, such as `l(M,")")` or
`l(X,")")`. `c-=c` is a roundabout way of writing `c=0`.

12. Replacing a library function with a macro

`#define gets(D) fgets((D),'x',stdin)` keeps the call looking like the unsafe
`gets` while actually reading at most 119 characters into the 120 byte
buffer `D`.
