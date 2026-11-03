# 1986/applin

Difficulty: Easy

1. Polyglot source file (C, shell and Fortran in one file)

The same file is a valid program in three languages. When run by `/bin/sh`,
the first line `cat =13 /*/ >/dev/null 2>&1; echo "Hello, world!"; exit` runs
`cat` with its output thrown away, then prints the greeting and exits before
the shell ever sees the C code. In Fortran 77, any line starting with `*` or
`c` in the first column is a comment, so the C parts are skipped and Fortran
only sees `write(...,'(...)') "Hello, world!"` and `end`. In C, everything
that belongs to the other languages is hidden inside `/* ... */` comments.

2. Comments that look like code

The first line contains `/*/`. To C this is the start of a comment (the
trailing `/` does not close it), so the shell commands after it are ignored
until the `*/` at the start of `*/; main() {`. Inside `write(`, the text
`/*,'(` and the following `*/` hide the Fortran format string, and the final
`/* ,)') end */` hides the rest of the Fortran statement and its `end`.

3. Implicit int global with a misleading name

`cat =13` uses the old C rule that a declaration without a type is an `int`.
The name `cat` is chosen because it is also a shell command on the same line,
but in C it is just an integer holding 13, the length of `"Hello, world!"`.

4. Bitwise arithmetic tricks to hide constants

`~-x` is the same as `x - 1` (negate, then flip all bits). So `cat-~-cat` is
`13 - 12`, which is `1`, the file descriptor for standard output. Applying the
trick three times, `~-~-~-cat` is `13 - 3`, which is `10`, the ASCII code for
the newline character `'\n'`.

5. Low-level output instead of printf

`write(1, "Hello, world!", 13)` sends the bytes directly to standard output
without any header or prototype, relying on implicit function declarations.
The newline is printed separately with `putchar`, so neither call shows a
complete, recognizable `"Hello, world!\n"`.
