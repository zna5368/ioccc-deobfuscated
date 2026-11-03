# 1986/pawka

Difficulty: Easy

1. Macros that rename keywords

The first lines use `#define` to swap C keywords and functions for names
like `xyxx` (`char`), `xyyyxx` (`while`), `xyxyxy` (`if`), `xxxyyx` (`main`)
and `xyyxx` (`putchar`). The preprocessor puts the real words back before the
compiler sees the code, but a person reading it has to look each one up.

2. Look-alike identifiers

Every variable is built from only the letters `x` and `y` (`xyx`, `xyyx`,
`xyyyx`, `xyyyyx`, `xyyyyyx`, ...). The names differ only by how many `y`s they
contain, so they are very easy to mix up. In `simple.c` they became `banner`,
`pos`, `count` and `row`.

3. Decoy comments

Comments such as `/*xyyxyxyxxxyxxxyy*/` are scattered at the ends of lines.
They do nothing; they are filler made of the same `x`/`y` letters so they blend
in with the real code and make it harder to tell code from comment.

4. Run-length encoding of the picture

The banner is not stored as text. Each line is a list of pairs: "print this
character N times". The first character of a pair is the repeat count and the
second is the character, so the big block letters take far less space and
can't be read directly from the source.

5. Offset (shifted) character encoding

Both halves of every pair are shifted so they look like noise. The count is
stored with `0x59` (`'Y'`) added, so `]` means 4 and `^` means 5. The printed
character is stored with `0x29` added, so `I` becomes a space, `^x` becomes
five `O`s and `h` becomes `?`. The program subtracts the offsets at run time
(`*xyyx++-xyyyyyx` and `*xyyx-xyyyyyyx`). The offsets are written in hex
(`0x59`, `0x29`, `0x68`) to hide that they are just characters.

6. Hidden end-of-line marker

There is no `\n` in the data. Instead the program checks whether the encoded
character is `h` (`0x68`, stored in `xxyx`) and, if so, prints a newline. `h`
decodes to `?`, which only appears at the end of each row, so the question
mark doubles as the line break signal.

7. Character used as a null pointer

The string array ends with `'\0'` instead of `NULL`. A character constant with
the value zero is also a valid null pointer constant, so the loop
`while(xyx[xyyyyx])` stops there, but it looks like it belongs to a string.

8. Useless dereference

`*xyyx++;` reads the character and throws the value away; only the `++`
matters. The extra `*` is there to confuse the reader. It is written as plain
`pos++` in `simple.c`.

9. Packed formatting

The whole program is squeezed into a few long lines with no indentation, and
nested `while` loops share lines with declarations, so the loop structure is
hard to see.
