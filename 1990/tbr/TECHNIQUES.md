# 1990/tbr

Difficulty: Hard

1. Shaped layout with a decorative left column

The first half of `tbr.c` is drawn as large block shapes of code, and the
second half puts a column of single characters (`3`, `*`, `1`, `4`, ...) at
the start of each line. Those characters are not decoration: they are real
tokens of the program (`:3)`, `*--u`, `f[1]:`, ...), split away from the
rest of their expression so that the code is hard to follow line by line.

2. Macro that hides `close()` calls

`#define D ,close(` makes `r(o,0)D o)D*f)` expand to
`r(o,0),close(o),close(*f)`. The macro carries a comma and an unbalanced
open parenthesis, so the matching `)` appears later in the source and the
calls do not look like function calls at all.

3. Bizarre table initialisation

`m[m[60]=m[62]=32]=m[*m=124[m]=9]=6` fills the character class table in one
chained assignment. `m[60]` (`'<'`) and `m[62]` (`'>'`) get 32 and that
32 (`' '`) is used as an index, `*m` (`m[0]`) and `124[m]` (`m['|']`, written
backwards since `a[b]` is the same as `b[a]`) get 9 and that 9 (`'\t'`) is
used as an index. The result marks blanks with 6, separators with 9 and
redirections with 32, but the characters are only given as decimal numbers.

4. Parsing the line backwards

The main loop moves `c` to the end of the line and `r()` walks to the left
with `*--c`. Each word is terminated with `\0`, its start is found by
scanning further left, and it is pushed onto `v[]` from the top
(`*--u=++c`). Because the stack grows downward, the words come out in the
right order for `execvp()`. The scan stops at a `'|'` or at a zero byte,
and the zero before the buffer is actually the last byte of the table
`m[]`, which the compiler happens to place right before `q[]`.

5. Bit mask to tell `<` from `>`

`i[*c&2]=*u` uses the fact that `'<'&2` is 0 and `'>'&2` is 2, so the same
statement stores the input file in `i[0]` and the output file in `i[2]`
(`i[1]` is never used). The file name is the word on top of the stack,
which is the word that was to the right of the symbol.

6. Whole shell in one expression

All of the work (the `cd` builtin, `pipe()`, `fork()`, `dup2()`, `open()`,
`creat()`, `execvp()` and `wait()`) is a single statement of nested `?:`
and comma operators. Numbers such as `1`, `4`, `5`, `9`, `6`, `2` and `3`
are placed in branches that do nothing, just to fill the empty side of a
`?:`. Writing it as `if`/`else` blocks shows the parent and child paths.

7. Recursion instead of a second fork

When the command is to the right of a `|`, the parent creates a pipe, forks
the right command and then calls `r()` again with the write end of the pipe
as `t`. The recursive call keeps parsing to the left from the same `c`, so
every command in a pipeline is started by the same loop without a separate
loop for pipelines.

8. One `write()` for the prompt and errors

`e(x)` prints `"?\n$ "-x/4`, which is the string plus `-x/4` characters.
Calling `e(-8)` skips two characters and prints `"$ "`, the prompt. Any
other negative value from a failed system call prints `"?\n"`. `e()` also
exits when the value is exactly `-1`, so `chdir()`'s result is doubled to
`-2` to print the error without leaving the shell.

9. Magic numbers

`438` is the octal permission `0666` for `creat()`, `512` is the line size
and `98` is the top of the argument stack (`v[98]` stays `NULL` to end the
argument list). `u-v^98` uses XOR as a "not equal" test to check whether
the stack is empty.
