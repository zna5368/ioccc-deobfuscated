# 1988/spinellis

Difficulty: Easy

1. Including a device file as source code

The whole file is the single line `#include "/dev/tty"`. Normally `#include`
pastes the contents of a header file into the program before it is compiled.
Here the "header" is `/dev/tty`, a special file that stands for the terminal
you are typing on. When the preprocessor opens it, it waits for keyboard input
and pastes whatever you type, up to EOF (usually control-D), into the source.
The program you get is therefore the code you typed during compilation, not
anything written in `spinellis.c`.

2. Program text that does not exist in the source file

Because the real code is supplied while compiling, reading `spinellis.c`
tells you nothing about what the built program will do. Two people compiling
the same file can end up with completely different programs, and the code
that ran is not saved anywhere unless you kept a copy of what you typed.

3. Using a feature for something it was not meant for

`#include` is meant for header files on disk, and the preprocessor does not
check what kind of file it is reading. Pointing it at a device instead of a
file turns the compiler into an interactive editor. This also makes the entry
depend on the compiler being able to read from a terminal, which is why some
compilers (and builds run without a terminal, such as from a `Makefile`)
cannot compile it at all.
