# 1998/schnitzi

Difficulty: hard

1. Function-pointer state machine instead of control flow

`main` contains only `x: (O)(); goto x;`, which calls whatever function `O`
points to, forever. Each of the roughly 45 tiny functions does one small step
and then sets `O` to the next step, like `void _s(){ O=g; _=x[3]; j=exit; }`.
The order the program runs in is hidden in these assignments rather than
written out as loops and function calls. To read the program you have to
follow the chain of `O=` assignments by hand. simple.c replaces them with
normal functions, loops and recursion.

2. Branching without `if` using arithmetic on function pointers

The code has no `if`, `?:` or loops, yet it still makes decisions. A function
stores the condition in `m`, the "true" target in `w` and the "false" target in
`l`, then jumps to `p`. `p`, `n` and `_j` then build the target address in `h`
by computing `h = 0 + (!m)*l + (!!m)*w`. When `m` is zero only `l` survives the
multiplication, and otherwise only `w` survives. `_c` then sets `O=h`, so the
next step is the chosen function. This is an `if/else` written as
multiplication on pointers cast to `long`.

3. Continuation variable for a shared "print" step

All output goes through the single function `g`, which runs `printf(_)` and
then jumps to `j`. A step that wants to print something sets `_` to the text and
`j` to the step that comes next (for example `_=x[7]; j=_h;`). This way one
`printf` serves the whole program, but it also means each piece of output is
split across two functions.

4. Loops and recursion built from globals

`for`/`while` loops are made by pointing the "true" branch back at the same
function (`w=e_` inside `e_`). Because the conditions are checked after the
body, these loops always run at least once. That is why `./schnitzi 1` prints
`int a,b;`, which is a known bug. The recursion that tries every insertion
position uses the global counter `z` as its depth, `f[z]` as the loop counter
for each level and `_p[z]` as a saved copy of the ordering. So a recursive
function is simulated with global arrays, and the call stack never goes deeper
than one call.

5. Meaningless and misleading identifiers

The names are single letters with underscores added (`_o`, `o_`, `_q`, `_p`,
`_d`, `_`), so they look alike and are easy to mix up. The `main` parameters are
named `v` and `V`, and `v` is also the name of a global function. The variable
that holds `argv` is declared as a `char *` (`_d`) and cast back to `char **`
when it is used.

6. String table and indirect literals

Every piece of generated C text, such as `"if ("`, `"else "` and `"printf(\""`,
sits in the array `x[]` and is printed by index (`_=x[12]`), so the reader
cannot see which text goes where. The letter `'a'` never appears. It is taken
from the second character of `"main()..."` as `x[17][1]`, and every variable
name is computed as `x[17][1]+k`. Doubled `%%` in the strings are needed because
they are passed to `printf` as the format.

7. Single-character strings for printing characters

To print one character, the code puts it in `r[0]` (with `r[1]=0` set earlier)
and calls `printf(r)` instead of using `%c` or `putchar`. This hides that a
single variable name is being printed.

8. Hand-written number parsing and a hidden exit

The command-line number is checked with `atoi`, but its value is parsed again by
`_b`, one digit per step (`q=*i+q*10-'0'`). The program ends by setting `j=exit`.
The main loop then calls `exit()` with no argument, so the exit status is
whatever value is left in a register.
