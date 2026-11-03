# 1990/stig

Difficulty: Easy

1. Program hidden in the build command

The file `stig.c` contains only the single character `c`. The real program
lives in the shell alias in `stig.sh`, which passes it to the compiler with
`-Dc='int main(){printf(...);}'`. When the preprocessor sees the lone `c` in
`stig.c` it replaces it with the whole `main` function. Reading the C file
alone tells you nothing; you have to read the build script to find the code.

2. Alias with the same name as the source

The alias in `stig.sh` is also called `c`. The Makefile runs
`source ./stig.sh ; source ./stig.c`, so the shell reads `stig.c` as a shell
script. Its only line, `c`, runs the alias, which compiles `stig.c`. The same
one-letter file is both a shell command and a C program.

3. Two-stage build through a generated file

The alias runs `cc -E` to only preprocess `stig.c`, strips the `#` line markers
with `grep -v '^#'`, saves the result as `o.c`, and then compiles `o.c` a
second time into the program `o` and runs it. The missing `#include <stdio.h>`
is supplied on the command line with `-include stdio.h`, so the header never
appears in any source file.

4. Creative shell quoting

To get double quotes, `\n` and `%s` into a C string inside a `-D` option inside
an alias inside a double-quoted string, the author switches between `"..."`,
`'...'` and `\"` quoting several times in one line, e.g. `\"N%sested comments
allowed.\\n"'",...'`. Each layer of the shell removes one level of quoting,
so the text only becomes valid C after all the layers are peeled off.

5. Comment that reads differently on different compilers

The expression `/*/*/0*/**/1` gives `0` on a normal compiler: the comment
`/*/*/` ends at the first `*/`, leaving `0*`, then the empty comment `/**/`,
then `1`, so it is `0*1`. A compiler that allows nested comments sees the two
`/*` as two levels that are closed by the two later `*/`, leaving only `1`.
The result picks `""` or `"o n"` with `?:`.

6. Spliced format string

Instead of two full messages, the output is built from one format string
`"N%sested comments allowed.\n"`. Inserting `""` gives "Nested comments
allowed." while inserting `"o n"` gives "No nested comments allowed.", which
hides both messages from a quick search of the source.
