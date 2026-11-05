# 2014/morgan

Difficulty: Hard

1. Meaningless identifiers

Every function, variable and struct field is a single letter or `_`. `J` sets a
variable, `M` parses text, `N` expands variables and `Q` builds a target, but
nothing in those names says so. The same letter is also used for unrelated
things: `O` is both a function and a `goto` label inside `M`, and `o` is both a
local variable and another label. This is legal because labels have their own
separate set of names in C.

2. Keywords and operators hidden behind macros

`#define U for(`, `#define V if(`, `#define R &&`, `#define S ++` and
`#define L ->` turn normal C into a code of capital letters. `V j-i)B(` means
`if(j-i)B(`, so the opening bracket lives inside the macro and the line looks
unbalanced. `W`, `X`, `Y` and `Z` go further and stand for a field access, so
`s W` means `s->i`. `A` hides a whole `calloc` call and `D` hides `m.`.

3. Source code shaped into block letters

The middle of the file is laid out as large letters made of code. Statements
are cut wherever the shape needs a gap, so one expression may be spread over
several lines with big holes in it, and the line breaks tell you nothing about
the structure of the program.

4. One struct used for every kind of data

`struct T` is the only data type. Text, lists, variables, targets, rules and the
parser's stack of open brackets are all `struct T`, and each one uses a different
set of fields. `i` can be a string length, a list size, a variable priority or a
counter of how many times a target was visited. The type tells you nothing, so
you must work out what each value is from how it is used.

5. Encoded string literal

`char p[]="NBLF\1Nblfgjmf\1"` looks like noise. The loop
`U; (*q++)--; );` subtracts one from every byte, which gives `MAKE`, a zero
byte, then `Makefile` and another zero byte. The one array now holds two strings:
`p` is `MAKE` and `p+5` is `Makefile`. The loop stops when it reaches the real
zero byte at the end of the array.

6. Character codes written as numbers

Characters are written as numbers, so you need an ASCII table to read the parser:
`92` is a backslash, `35` is `#`, `36` is `$`, `10` is a newline, `9` is a tab,
`40` is `(`, `123` is `{`, `58` is `:` and `61` is `=`. The `0` in `open(p+5,0)`
is also a number standing for the name `O_RDONLY`.

7. XOR used in place of not equal

`c^10` is non-zero only when `c` is not 10, so it does the same job as `c!=10`.
The same trick appears in `*q^61`, `b X[u]^9`, `d^40` and `g^2`. Most people read
`^` as bit twiddling, so this hides that it is only a comparison.

8. Jumps between branches of a switch

In `M`, the `default` case of the inner `switch` does `goto o`, and the label `o`
is in the middle of an `if` in the `default` case of the outer `switch`. Two
branches that look separate share their last lines. `case 35:j S; case 0:break;`
also falls from one case into the next with no warning. `goto O` jumps out of the
nested `switch` and the loop from two different places.

9. Results passed through global variables

`M` does not return what it parsed. It writes the list and the stop character to
the global `x` and returns only the position. Any later call to `M`, including
one made inside `N`, overwrites `x`, which is why `main` has to save a copy with
`y = x`. Reading a call to `M` on its own does not show what it produces.

10. Variables reused for different jobs

The global `u` is the read position in the Makefile while parsing, then the
status of the last command while building, and `main` returns it. In `J` the
length parameter `l` becomes a loop counter, in `Q` the parameter `s` is replaced
by the command text, and in `main` the argument count `i` becomes a loop counter
and then the file size. The meaning of a name changes halfway through a function.

11. One function doing three different jobs

`M` parses Makefile lines, recipe lines and variable values. The number `g` picks
which one and the string `t` lists the characters to stop at, so a single loop has
to mix all of the rules for comments, newlines and backslashes together. `i` and
`g` are changed in the loop as well, so the rules shift while it runs.

12. Logic done with arithmetic

`d` holds the result of a true or false test and is then used as a number:
`p-k-d*l/2-1` removes half of the backslashes only when `d` is 1. `!l%2` looks
like it tests for an even number, but `!` is done first, so it only tests for
zero. You have to know operator precedence to see what is really being worked out.

13. A two pass loop that hides its data

`U i=0; i<3; i+=2)` runs twice, with `i` as 0 and then 2. The first pass reads
the command line and the second reads the environment, because `a=e` swaps the
array at the end of the loop body. `i+1` then gives 1 or 3, which is the priority
of the variable, and `!i` means that only the command line can add goals. Nothing
in the loop says that it has two separate meanings.

14. A hidden NULL

`void * H` is a global that is never assigned, so C starts it at zero. It is used
in places like `C(H,0)` and `_ r=H` as a stand-in for `NULL`, and it looks like it
should hold something important.

15. Work packed into arguments and conditions

Assignments and calls are nested inside other calls, so one line does several
steps. `b W=read(open(p+5,0),b X=P i),i)` allocates a buffer, stores it, opens a
file, reads it and stores the length. `stat(memcpy(calloc(1,d W+1),d X,d W),&z)`
makes a text copy with a zero byte on the end and passes it to `stat` in the
condition of an `if`.

16. Lists consumed by moving their pointers

`U; d W--; )B(O(*d Y S) L d,c);` walks a list by adding one to the list's own
array pointer `b` and subtracting one from its count until nothing is left. The
list is destroyed as it is read, and there is no index variable to show that this
is a normal loop over the items.
