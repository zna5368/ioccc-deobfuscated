# 2013/robison

Difficulty: Insane

1. Arithmetic done without arithmetic operators

The source has no arithmetic operators and no array indexing. Every calculation
goes through a library call instead. `U()` adds one to a pointer by copying the
one-character string `c` over it, asking `strchr` where its end is, and putting
the old bytes back. `o()` subtracts one from each character by filling a
buffer with that many bytes, removing one and taking `strlen`. The `strlen` of a
buffer is really the counter in almost every part of the program.

2. Numbers stored as a pile of characters

A number is not stored as digits or as a binary value. It is a string with one
character for each unit of each digit, and the value of the character names the
place of that unit. Character 1 is a one, character 2 is a ten, character 3 is a
hundred, so 203 becomes `"\1\1\1\3\3"`. The order does not matter, so you cannot
tell what a number is by looking at the string.

3. String functions used as math

Each math step looks like text handling. `strcat` is addition, because joining
two piles of units adds them. `strcmp` on two strings sorted high to low tells
which number is larger. In `C()`, the `memset`, `strchr`, `strrchr` and `strlen`
chain lays two runs of bytes end to end and measures them, which adds the two
places together and so multiplies two powers of the base.

4. Carrying done with search and replace

`D()` looks for the base's number of equal characters with `strstr`, overwrites
them with `memset` and leaves one character for the next place. The search
string is built on the fly in `E`, so nothing in the code looks like a carry.

5. qsort used as a way to go backward

String functions only move forward through memory. `qsort` is used to bring
things to the front instead. `t()` sorts with ascending order, which pushes the
marker character 127 to the end, so one cut deletes every marked unit at once.
`m()` sorts in descending order, which brings the sign to the front where it can
be seen and skipped.

6. One character with three jobs

The string `c`, which is just character 127, is a minus sign when it is inside a
number, a "delete me" mark while subtracting or carrying, and a tally mark in
`R`, where `strlen(R)` counts how many times the divisor was scaled up.

7. Subtraction that works from the left

`n()` removes the highest units first, unlike school subtraction. When a unit is
missing it does not borrow from the next place. It breaks up the highest unit in
the number into `l - 1` units of every lower place, using the fact that 1000 is
999 plus 1. This makes the loop hard to recognize as subtraction.

8. Short macros for local variables and loops

`#define r char S; T R; I N, G, s` hides the local variables of nearly every
function behind one letter. Because it ends with `s`, writing `r=Q(),P,O;` gives
`s` a starting value and also declares two more pointers, so the line looks like
an assignment to a variable called `r`, which does not exist. In the same way,
`#define A(P,O) for(P=O; *P; P=U(P))` hides a loop over the characters of a
string, so `A(P,P)A(s,O)` is two nested loops.

9. Macro that changes a library function

`#define memset (I)\` followed by `memset` on the next line adds a cast to every
call, so the result can be used right away as a `char *`, as in
`*memset(N,*c,l)=S`. Without the macro, that line would not compile, because
`memset` returns a `void *`. Nothing at the call shows that the cast is there.

10. Layout that hides the program structure

Every library name is lined up in one column, and the code around it is pushed
to the left. A single nested call is spread over many lines, so `* strrchr (`
`memset (` `strchr (` is one statement that reads like a table. The line breaks
have nothing to do with where statements begin and end.

11. Single letter names reused for everything

Functions, globals and locals all have names of one letter, and the same letter
often has several meanings. `P` is a global function pointer, a parameter in
most functions and a local variable in others. `I`, `T` and `p` are types that
look like variables, and `f` is a type for `const void *`.

12. Dead code as a distraction

`void*P= strcpy;` declares a global that is never used. All it does is look
important, and every later `P` is a different variable that hides it.

13. Encoded operators and strings

The characters in the parser are not the real ones. `d()` subtracts one from each
punctuation character it reads, so `+` is seen as `*`, `-` as `,`, `*` as `)`,
`/` as `.`, `%` as `$`, `(` as `'` and `)` as `(`. A check like `S==')'` is really
looking for multiplication. In the same way, `"tzouby!fssps"` is "syntax error"
with one added to every letter, and `o()` decodes it just before printing.

14. Digit table computed at startup

The digits `0123456789abcdefghij` do not appear in the source. `H` is `"(*&,."`,
`i` is `"\13<\f="`, and `main` multiplies them together using the program's own
number format. Adding each pair of characters and removing one gives exactly the
twenty digit characters, which are then copied back into `i`.

15. Argument found by sorting

`main` sorts `argv` by first letter and reads the base from `*O`, the first
element, rather than `argv[1]`. This only finds the base if it sorts before the
program name. `./robison 16` reads the name and gets base 0, while
`./robison +16` works. The third argument of `strtol` is `l`, which looks like the
base but is still zero at that point, so `0x` and `0` prefixes are accepted too.

16. Recursive parser hidden by short names

`h()`, `M()` and `Q()` are a normal expression parser for sums, products and
single values, calling each other for parentheses. Their names and the encoded
operators in `d()` hide this. The sign logic is also packed into comparisons of
pointers, as in `P==s==(O==N)`, where `P==s` means that no sign was skipped.
