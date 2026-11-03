# 1987/korn

Difficulty: Easy

1. A predefined macro that looks like a variable

`unix` is never declared in the program. Old Unix C compilers defined it as a
macro with the value `1` (the Makefile fakes this with `-Dunix=1`). So every
`unix` in the code is really just the number `1`, and `(unix)["have"]` is
`(1)["have"]`.

2. Swapped array subscripts

In C, `a[b]` is defined as `*(a + b)`, and since addition does not care about
order this is the same as `b[a]`. The program uses this backwards form:
`unix["\021%six\012\0"]` is `"\021%six\012\0"[1]` and `(unix)["have"]` is
`"have"[1]`, which is the character `'a'`. Putting the number first makes it
look like `unix` is an array.

3. Pointer arithmetic on string literals

A string literal such as `"fun"` is really the address of its first
character, so adding a number to it skips characters. `'a'` is `0x61`, so
`(unix)["have"]+"fun"-0x60` is `"fun" + 0x61 - 0x60`, which is `"fun" + 1`,
the string `"un"`. Likewise `&unix["\021%six\012\0"]` takes the address of
the second character, skipping the first byte.

4. Octal escapes and a decoy byte in the format string

`"\021%six\012\0"` hides the real format string. `\021` is a junk control
character that is skipped by the `&1[...]` trick above, `\012` is octal for
the newline `\n`, and the trailing `\0` is an extra string terminator that
does nothing. What is left is `"%six\n"`, so `printf` prints `"un"` followed
by `"ix"` and a newline: `unix`.

5. Hex constant and implicit declarations

The `0x60` is written in hex so that it is not obvious it is one less than the
ASCII code of `'a'`. `main()` has no return type and `printf` is used without
including `<stdio.h>`, relying on old C's implicit `int` rules.
