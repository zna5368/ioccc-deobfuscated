# 1989/jar.1

Difficulty: Easy

1. Output from the compiler instead of from the program

The whole entry is one line, `char*_="Hello world.\n";`. There is no `main`
function, so it can never be linked into a program and never runs. It doesn't
have to. When the compiler turns a C file into an object file (`cc -c`), it
copies the bytes of every string literal into the object file exactly as
written. The author's command `cc -c -o /dev/tty jar.1.c` tells the compiler to
write that object file straight to the terminal instead of to a file. Most of
the object file is unprintable binary that the terminal ignores or shows as
junk, but the bytes `Hello world.` followed by a newline appear on the screen.
So the "Hello world." output is a side effect of compiling, not something the
program prints. The Makefile and `jar.1.sh` do the same thing safely by
sending the object file through `strings`, which shows only the readable text
in a binary file.

2. A variable named `_`

An underscore on its own is a legal C name, but it says nothing about what the
variable holds. In `simple.c` it is renamed to `msg`. The new name is kept to
three letters on purpose: names of global variables are stored in the object
file too, and `strings` only shows runs of four or more readable characters,
so a three letter name stays hidden just like `_` did. A longer name such as
`message` would show up as an extra line of output.

3. Missing whitespace

`char*_="Hello world.\n";` has no spaces around `*` or `=`. C doesn't need
them, but writing `char *msg = "Hello world.\n";` makes it easier to see that
this is a pointer variable set to point at a string.
