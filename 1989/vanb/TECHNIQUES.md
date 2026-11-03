# 1989/vanb

Difficulty: Easy

1. Recursive main

`main(Q,O)` calls itself twice: `main(Q,O);` before flipping a flag and
`main(Q,O);` again after printing. Because `Q` is decremented with `--Q`
first, each call works on one fewer word. The program uses this to walk
every on/off combination of the words, so each subset is printed in Gray code
order (only one word changes between two printed subsets). `simple.c` moves
the recursion into `enumerate_subsets()` and calls it once from `main`.

2. Misleading parameter names

`argc` and `argv` are renamed to `Q` and `O`, which look like the digit `0`
and make expressions like `O[O[0][0]][0]` hard to read. `simple.c` uses
`count` and `words`.

3. No variables, the arguments are the storage

There are no local or global variables. The loop counter of the print loop is
stored in the first character of the program name, `O[0][0]`, which is never
printed, so `for(O[0][0]=0;O[++O[0][0]]!=0;)` counts through the words using
`argv[0][0]` as `i`. `simple.c` uses a normal `int i`.

4. High bit of a character used as a flag

`O[Q][0]^=0X80` flips the top bit of the first letter of word `Q`. That bit
marks whether the word is left out of the current subset, so no separate
array of flags is needed. The words are changed in place but can still be
shown because the bit is only on while the word is hidden.

5. Signed char test for the flag

`if(O[O[0][0]][0]>0)` checks the flag. A `char` is signed on most systems, so
when the top bit is set the value is negative and the test fails. The same
test also skips empty words whose first byte is `0`. `simple.c` writes this
as `is_in_subset()`, which checks for a non-zero character and tests the bit
with `LEFT_OUT_BIT` directly.

6. Old style C

There are no `#include` lines, `main` has no return type, `Q` is an implicit
`int` and `O` is declared in a K&R parameter list. `puts` is used without a
declaration. `simple.c` keeps no headers but uses modern function
definitions.

7. Everything on one line

The whole program is a single line, with an `if` holding a `for` holding
another `if`, so the structure is hard to see. `simple.c` splits it into small
functions for toggling, checking, printing and recursing.
