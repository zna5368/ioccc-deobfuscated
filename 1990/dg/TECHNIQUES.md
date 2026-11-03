# 1990/dg

Difficulty: Medium

1. Token-by-token macro wrapping

The whole program body is written as four lines of deeply nested macro calls,
such as `bma(acp(bop(app(bcp(ai(...))))))`. Each of the 60 macros adds exactly
one token to its argument: macros starting with `b` put the token *before* the
argument (`#define bma(x) main x`) and macros starting with `a` put it *after*
(`#define acp(x) x)`). The preprocessor expands from the innermost call
outward, so the real program is assembled backwards, one token at a time,
starting from the middle. To read it you have to run the preprocessor (or
unwrap every call by hand).

2. Cryptic macro names

The macro names are short abbreviations of the token they add: `cp` is a close
parenthesis, `ob` an open bracket, `se` a semicolon, `qm` a question mark,
`pl` a plus, `wh` the `while` keyword, `ix` `index`, and so on. Numbers are
named after themselves (`b12`, `a64`). This hides keywords like `for`,
`while` and `main` so nothing in the body looks like C.

3. Abbreviated `#define` directive

In `dg.orig.c` the line `#define d define` is followed by directives written
as `#d b12(x) 12 x`. This looks like `#d` is being expanded to `#define`, but
the preprocessor never expands macros inside a directive name, so this does
not actually work on standard compilers; the distributed `dg.c` spells out
`#define`. The judges also had to replace the `aqu(bqu(A))` trick, which
glued two unmatched single quotes around `A` to form the character `'A'`.

4. Lookup table built with chained assignments

`l[i]=13+(l[i+16]=19+(l[i+32]=13+(l[i+48]=(i>12?12:i)+'A')))` fills four
16-character blocks in a single statement by nesting assignments and adding
offsets between them. The magic numbers 13 and 19 are the distances from `A`
to `N`, from `N` to `a` and from `a` to `n`, so the letters never appear
in the source. The table ends up as
`nopqrstuvwxyzzzzabcdefghijklmmmmNOPQRSTUVWXYZZZZABCDEFGHIJKLMMMM`.

5. XOR to find the rot13 partner

Each letter's rot13 partner is placed exactly 16 slots away in the table, so
the code uses `16 ^ offset` instead of adding or subtracting 13. Flipping
bit 4 of the offset jumps between the paired blocks. The extra repeated
letters (`zzzz`, `mmmm`) pad each block to 16 so the XOR lines up, and since
`index()` returns the first match they are never found themselves.

6. Pointer arithmetic with `index()`

`index()` (the old BSD name for `strchr()`) is used to test whether a
character is a letter: it returns a pointer into the table or `NULL`.
Subtracting the table address (`j-l`) turns that pointer into an offset.
The `?:` operator then picks the translated letter or the original
character inside a single `fputc()` call.
