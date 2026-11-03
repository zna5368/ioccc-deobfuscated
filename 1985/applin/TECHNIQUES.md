# 1985/applin

Difficulty: Medium

1. Recursion through `execlp` instead of a loop

The program never walks through the string in a loop. Each process prints only
the first character of its message and then calls
`execlp(*c,*c,c[!!c]+!!c,!c)`, which replaces itself with a fresh copy of the
same program (`argv[0]`), passing the message minus its first character
(`argv[1]+1`) as the only argument. The "loop" over the characters of
`"Hello, world!\n"` is really a chain of 14 separate program runs. The string
ends with an extra `\n` so that the chain stops when one character is left
(the loop checks that the second character is not the end of the string).

2. Swapped and misleading parameter names

`main(v,c)char**c;` uses an old-style declaration and names the argument count
`v` and the argument list `c`, the opposite of what most readers expect
(`argc`/`argv`). `v` is used as a countdown that decides when it is time to
re-execute, and `c` is moved back and forth with `c++` and `--c` to point at
either `argv[1]` (the message) or `argv[0]` (the program name).

3. Reversed array indexing

In C, `a[i]` means `*(a+i)`, so it is the same as `i[a]`. The code uses this
twice: `v[c++]` is really `argv[argc]` (writing the greeting into the slot just
past the last argument), and `(!!c)[*c]` is really `(*c)[1]`, the second
character of the current message.

4. `!!` and `!` used to make the constants 1 and 0

`!!x` turns any non-zero value into `1`, and `!x` turns a non-null pointer into
`0`. Since `c` is never null, `!!c` is just `1` and `!c` is just `0`. This hides
plain numbers: `write(!!*c,*c,!!**c)` is `write(1, *c, 1 or 0)` (standard
output, the message, one byte if the first character is not already erased),
`c[!!c]+!!c` is `c[1]+1`, `**c=!c` is `**c=0`, and the final `!c` passed to
`execlp` is the `0` that ends its argument list.

5. Control flow hidden in a `for` loop condition

All of the decisions are packed into short-circuit operators in the loop test:
`(!!c)[*c] && (v-- || --c && execlp(...))`. While the countdown `v` is
non-zero the `||` stops early and the body runs; when it reaches zero, `--c`
(always true) moves back to `argv[0]` and `execlp` runs. The loop's
"increment" part `**c=!c` erases the first character after it is printed, so
later passes write zero bytes.

6. Overwriting the argument list

`argv[argc]` is normally the null pointer that ends the argument list. The
program stores the `"Hello, world!\n\n"` copy there. On the first run there
are no arguments (`argc` is 1), so this lands in `argv[1]` and becomes the
message. On later runs `argc` is 2 and the greeting goes to the unused
`argv[2]`, while `argv[1]` holds the shortened message from the previous run.
