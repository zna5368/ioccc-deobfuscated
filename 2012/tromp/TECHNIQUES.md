# 2012/tromp

Difficulty: Insane

1. Source shaped like a picture

The whole file is laid out in the shape of a giant lambda. Line breaks fall
wherever the shape needs them, even in the middle of `b=b\n--?b:m|read(...)`,
so the layout says nothing about the structure of the code.

2. Single letter names that clash

Every variable and function is one letter, and upper and lower case pairs are
different things: `s` and `S`, `c` and `C`, `u` and `U`, `b` and `B`. Some
names are also reused. The global `m` is the mode, but inside `p(Int*m)` it
means the term being parsed. The global `u` is the current operand, but inside
`k(l,u)` it is the last index to copy.

3. Data hidden in an XORed string

`46^l++["-,&,,/.--/,:-,'/" ".-,-,,/.-,*,//..,"]` looks like a string, but it is
a table of small numbers. XOR with 46 turns each character back into a number,
for example `'-'` (45) becomes 3 and `':'` (58) becomes 20. The numbers are
lambda terms, and `k(l,u)` copies the ones from index `l` to index `u`. In
`simple.c` the table is split into `cons_header`, `boolean_header` and
`boot_program`.

4. Reading the end of a string on purpose

`k(10,33)` copies index 33, which is the hidden NUL at the end of the string.
That puts a useless 46 in `L[23]`, but it is replaced straight away by
`a[23]=p(U)`.

5. Backwards array indexing

In C `a[i]` means `*(a+i)`, so `i[a]` means the same thing. The code uses this
in `l++["..."]`, which is `"..."[l++]`, and in `++1[a=a[2]]`, which is
`++(a=a[2])[1]`.

6. All the logic inside a for statement

`main` has no `if` or `switch`. Its whole interpreter is one huge chain of `?:`
and comma operators in the third part of `for(...;;...)`. The real loop body is
only the fetch `e=L[C++],u=L[C];`. The test `e-2?e?e-3?` is a four way switch
that checks for 2, then 0, then 3, and treats the rest as 1. `simple.c` turns
it into a `switch` that calls `apply`, `lambda` and `variable`.

7. Comma operators inside an array index

`d(l[d(*l),*l=B,B=l,2])` has three statements hidden in the brackets. They run
first, and only the last value, 2, is used as the index. So this frees the next
closure, adds `l` to the free list, and then frees `l[2]`.

8. Function arguments used for side effects

`s(_)` ignores its parameter. The call `s(a=*a)` only uses the argument as a
place to do the assignment, and `s(D=a)` does the same to save `a`. The call
`p(U[1]++)` also passes a number where a pointer is expected, because only the
increment matters.

9. Recursion in place of loops

`s` walks a linked list and `x` repeats once per bit by calling themselves. For
example `u--&&s(a=*a)` is a counting loop written with `&&`, because the right
side only runs when the left side is not zero.

10. One function with two jobs

`p` starts with `!*U` to find out what it is doing. If the current entry is
still 0, it is starting a new term. If the entry is already 1, it is in the
middle of a variable and is counting the `1` bits, so the "state" of the parser
is stored in the output it is writing.

11. Overlapping lambda terms

The startup terms share entries. `L[9]` is 3, which is the size of a function
but also the opcode of a lambda when execution jumps to index 9. `L[13]` and
`L[20]` are the variable index 0 but also the opcode 0 of an I/O hook. Reading
the table from a different index gives a different program.

12. Term space that doubles as a closure

`a=L` makes the environment point at the program itself. `L[0]` to `L[3]` are
read as a closure, so `L[1]`, which is the size 20, is also a reference count.
This gives every environment an end that is never freed, so no NULL checks are
needed. The size only has to be big enough to never count down to 0.

13. Integers and pointers mixed together

The program stores pointers in integers and integers in pointers with no casts.
`B` is declared as `char*` but really holds a list of closures, `*D=A` makes a
pointer from a number, and `d(*l)` turns a stored integer back into a pointer.
The four fields of a closure are only reached by number, as in `D[2]` and
`D[3]`. `simple.c` uses `struct closure` to give them names.

14. Missing declarations and types

No function or parameter has a type unless it needs one, because old C assumes
`int`. `read`, `write`, `exit` and `calloc` are used with no header at all. The
type `Int` and the size `X` do not exist in the source, and only come from the
`-DInt=long -DX=8` options in the Makefile.

15. Initial value that is never used

`*D=A` looks important, but `D` is always assigned before it is read, so the
value has no effect. `simple.c` drops it.

16. Mode folded into arithmetic

`m=e-2&7` turns the argument count into 7 (bytes) or 0 (bits), and that same
number does several other jobs. It is ORed into `b` as the number of bits to
read next, and `a[4]-=m` patches a term so that it jumps to a different
printing routine. No `if` ever checks which mode is on.

17. Parity test in place of a comparison

`u+m&1` decides between collecting a bit and printing. `u` is the entry after
the I/O hook, which is odd only for the end of byte hook, and `m` is odd only
in byte mode. Adding them gives an odd number when exactly one is odd, so one
test covers every case. `simple.c` compares `byte_ended` to `mode & 1`.

18. Inverted input bits

`S` returns `~I>>b&1`, the opposite of the bit that was read. This is exactly
what the callers want, since a 0 bit starts a lambda and becomes the variable
index 1 for "true". Nothing in the code says that the value is flipped.

19. Peeking at the input with a counter

`S(),x(0<b++?...)` reads a bit only to find out if EOF was reached, and then
`b++` puts the bit back. The test and the undo are the same expression. After
EOF `b` is -1, so `0<b++` is false and `b` becomes 0, which makes `x` write the
empty list.

20. Values used twice in one expression

`a[++D[1]]++` sets the reference count of the new closure to 1, and then uses
that same 1 as the index of the reference count of `a`. `x(... U[-5]=96 ...)`
patches a term and also passes the 96 as "true" to `x`. In `write(m||(O=C+28),
&O,1)`, the result of the `||` is always 1, which is the file number of
standard output, and the right side sets the character to print.

21. Return values used as jump targets

`C=...write(...)+1` uses the fact that `write` returns 1 to jump to term 2. The
number looks like error handling, but it is really the address of the code that
prints the next list item.

22. Exit status read from the program

`exit(L[C+1])` looks like an error exit. It is the normal way to stop, because
the last term to run is the empty list `\a \b. b`, and the entry after its
second lambda is the variable index 0. That is why a finished program exits
with status 0.
