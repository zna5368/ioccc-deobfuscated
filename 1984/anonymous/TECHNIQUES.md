# 1984/anonymous

Difficulty: Easy

1. Redefining a standard library function name

The program defines its own function called `read`, which replaces the real
`read()` system call. Anyone reading `read(...)` in `main` expects the program to
read input, but this `read` actually calls `write()` to print a character. The
judges hint at this with "What reads like a read may be written like a write!"

2. Reversed array indexing

`i["]<i;++i){--i;}"]` looks strange, but in C `a[b]` is the same as `*(a + b)`,
which is the same as `b[a]`. So `i[string]` simply means `string[i]`, the i-th
character of the string.

3. Decoy string that looks like code

The string `"]<i;++i){--i;}"` looks like C code but it is just text inside
quotes. It is only used to stop the loop: it has exactly 14 characters, the same
length as `"hello, world!\n"`, so the loop runs once per character of the message
and ends when it reaches the string's terminating zero.

4. Character arithmetic to hide constants

`'-'-'-'` is a character minus itself, which is `0`. `'/'/'/'` is a character
divided by itself, which is `1`. Inside the fake `read`, `j/p+p` becomes
`0/1+1`, which is `1` (standard output), and `i/i` is also `1` (write one
character). These expressions hide the plain numbers 0 and 1.

5. Confusing increment and decrement operators

`i+++"hell..."` is parsed as `i++ + "hell..."`, giving a pointer to the current
character and moving `i` forward. In the fake `read`, `i---j` is parsed as
`i-- - j`; since `j` is 0 this is just the pointer `i`, and the decrement of the
local parameter has no lasting effect. Also note that the global `i` (a counter)
and the parameter `i` (a pointer) share the same name.

6. Undoing work and empty loop body

The loop's real work (printing and incrementing `i`) is hidden inside the
`for` loop's condition-and-step area, and the loop body is just `;`. The step
`i++` is buried in the argument list of the fake `read` call.

7. Splitting a string across lines

The string `"hell\` + newline + `o, world!\n"` uses a backslash at the end of
the line to continue the string on the next line, so the word `hello` is split
as `hell o`, making the message harder to spot.

8. Old-style (K&R) function definitions and implicit int

`main()` and `read(j,i,p)` have no return types and their parameters have no
declared types, so the compiler assumes `int` for everything. This lets a
pointer be passed through an `int` parameter, which is unusual and confusing.
