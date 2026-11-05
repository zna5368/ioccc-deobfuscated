# 2013/cable1

Difficulty: Easy

1. Reversed array subscript

`1[acters]` looks wrong, but `a[b]` in C just means `*(a + b)`, and addition
works in either order. So `1[acters]` is the same as `acters[1]`, which is the
first command line argument. The outer `1[acters-~...]` uses the same trick.

2. Bitwise negation in place of addition

`acters-~x` hides a plain addition. `~x` is the same as `-x - 1`, so subtracting
it gives `acters + x + 1`. Together with the reversed subscript from above,
`1[acters-~x]` means `acters[x + 2]`.

3. Logical not used as a number

`!(...)` turns the result of the hash into 1 when it is zero and 0 otherwise.
That number is then added to the array index, so `x` above picks between
`acters[2]` and `acters[3]`. This replaces an `if` statement, and the two
possible outputs are never visible as two separate choices in the code.

4. Reading a string as a number

`*(int*)1[acters]` casts the `char *` name to an `int *` and reads it. Instead
of one letter, this loads the first four bytes of the name as one big number.
Nothing in the code compares text, yet the name still decides the result. The
number depends on the byte order of the CPU, which is why the entry needs a
little endian machine. Names under three letters even read into the next
argument.

5. Magic numbers in place of a lookup table

`%4796%275` replaces the list of presidents and the table of parties. Chaining
remainders squeezes the big number from the name down to a small one, and these
two constants happen to send every president to the right party. All the
knowledge sits in two numbers, so there is no data in the source to find. Any
other name, such as `Gates`, just falls wherever the arithmetic puts it.

6. The argument count used as a constant

`%riguing` is the last step of the hash, and `riguing` is `argc`. The program is
meant to run with three arguments, which makes `argc` equal to 4, so this is a
hidden `% 4`. The value comes from how the program is run instead of from the
source, and passing a different number of arguments changes the answers.

7. Misleading parameter names

`int riguing` and `char**acters` read as the words "intriguing" and "characters"
once the type is joined to the name. The names are a joke that hides the fact
that these are the usual `argc` and `argv`.

8. Implicit declarations and a single line

`main` has no return type and `puts` is called without including `stdio.h`,
which old C allows by assuming `int` for both. Dropping these, along with all
whitespace, is what lets the whole program fit on one short line.
