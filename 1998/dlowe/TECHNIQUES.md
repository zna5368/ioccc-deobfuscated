# 1998/dlowe

Difficulty: Easy

1. ASCII art layout

The code is laid out so that its shape spells "POOT" in big letters, with
the last lines squashed together. Tokens are split across lines and padded
with spaces (`( o == 60 ) && ( ++p )`) so they fill the letter shapes. The
compiler ignores the whitespace, so the shape does not change what the
program does, but it makes the statements very hard to read in order.

2. Magic numbers instead of character literals

Characters are written as their ASCII codes: `64` and `91` around the
upper case letters (`'A'` is 65, `'Z'` is 90), `96` and `123` around the
lower case letters, `32` as the distance between upper and lower case,
`60` for `'<'` and `62` for `'>'`. Reading `o == 60` gives no hint that
the program is looking for the start of an HTML tag.

3. Short-circuit operators used as `if` statements

`&&` and `||` only evaluate their right side when needed, so they can stand
in for `if`. For example `(o == 60) && (++p);` means "if the character is
`<` then add one to `p`", and `(t = p = 0) || (*O = '\0');` always runs the
right side because the assignment gives 0. The line
`(t != 8) && (O[t] = o) && (O[++t] = '\0')` appends a letter to the word
buffer only when the buffer is not full; it relies on the letter being
non-zero so that the chain continues to the final store.

4. Macro hiding the case conversion

The `PO(o,t)` macro is a deeply nested `?:` expression that returns the
letter `t` in upper case when `o` is upper case and in lower case otherwise.
Using it with the array expressions `*(O+T)` and `*(Poo+T)` makes the
`printf` calls expand to very long, hard to follow expressions. In
`simple.c` this became the small `match_case()` function.

5. Confusing, look-alike names

All the variables are made from the letters of "poot": `o`, `t`, `T`, `p`,
`O`, `poo` and `Poo`. `t` and `T`, `o` and `O`, `poo` and `Poo` differ only
by case, so it is easy to mix up the word length, the loop counter, the
input character and the word buffer.

6. Pointer arithmetic instead of indexing

`*(O+T)` and `*(poo+T)` are used instead of the usual `O[T]` and `poo[T]`.
They mean the same thing but are harder to read at a glance.

7. Hidden state machine and hidden behaviour

The variable `p` counts how deep the program is inside `<...>` HTML tags
so that markup is not changed, which is not obvious from the code. Other
details are also easy to miss: words longer than eight letters are cut
short because the buffer stops growing at `t == 8`, and a word still in the
buffer when the input ends is never printed.
