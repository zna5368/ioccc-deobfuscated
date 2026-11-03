# 2000/tomx

Difficulty: Easy

1. Polyglot source file (C, shell script and Makefile in one file)

The same file `tomx.c` is compiled as C, run as a shell script
(`./tomx.c`) and read as a Makefile (`make -f tomx.c`). Each language skips
the parts meant for the others: `#` starts a comment in both the shell and
make, while C hides everything between `true /*` and `*/` inside one comment.
Every line has to be valid in all three languages at the same time, which is
why the code looks so odd.

2. Code hidden inside a C comment

After `true /*` on line 4, the whole Makefile and shell script is one C
comment that ends at the `*/` in `.PHONY: /* true clean */`. A beginner
looking at the C sees only `#include <stdio.h>` and `main()`, because the
many lines in the middle are, for C, just a comment.

3. A macro that expands to nothing

`#define true` makes `true` an empty macro, so in C the line `true /*:all`
turns into the start of a comment and nothing else. To the shell, `true` is a
command that always succeeds and ignores its arguments. To make, `true` is
the first target in the file, so it becomes the default goal, and `:all`
makes it depend on the `all` rule that builds the program.

4. One line meaning different things in each language

`false :` is a command that quietly fails in the shell, but in make it is a
rule for a target named `false` that is never built. The tab-indented lines
under it are its recipe in make, but the shell runs them directly:
`make -f $0 $1` runs make on the script itself (`$0` is the script's name and
`$1` is its first argument, such as `clean`), and `exit 0` stops the shell
before it reads the make-only lines below. In make, `$0` and `$1` are empty
variables, so the recipe is harmless.

5. C code disguised as a make recipe

The line holding `main()` starts with a tab, so make treats it as the recipe
of the `.PHONY` rule, which is never run. The words `/*` and `*/` on the
`.PHONY` line are wildcard file names to make, but in C they close the big
comment so `main()` can follow.

6. Using printf's return value as the exit status

`return!printf("Hello, world\n");` prints the message and returns the logical
NOT of the number of characters printed. Since printf prints 13 characters,
`!13` is `0`, so the program exits with success. It is a short way to write a
print followed by `return 0`.
