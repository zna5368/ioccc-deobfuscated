# 1986/bright

Difficulty: Easy

1. Look-alike identifiers

Almost every name is built only from the characters `O`, `0`, `l`, `1`, `I`,
`L` and `Q`, e.g. `O1O`, `OlO`, `O10`, `Ol0`, `lO`, `llO`, `lll`, `Il`. These
characters look nearly the same in many fonts, so `O10` (exit) and `Ol0`
(strlen) are easy to confuse. Even the number literal `1O` inside a comment
looks like `10`. Renaming them to `file`, `offset`, `line`, `i` makes the code
readable.

2. Preprocessor renaming of library functions and keywords

`#define O1O printf`, `#define OlO putchar`, `#define QLQ fopen`,
`#define OlQ fgetc`, `#define QO0 for` and `typedef char lOL` hide well known
names behind look-alike macros. The preprocessor swaps them back before
compiling, so `QO0(...)` is really a `for` loop and `lOL*` is `char *`.

3. Constants hidden in variables and arithmetic

Simple numbers are computed instead of written. `O=1` and `l=0` are used as the
constants one and zero, `lll=O+O+O+l` is 3, `OQ=056` is the octal code for
`'.'`, `llO = EOF`, and `OL[' '^'0']` declares a 16 element array because
`0x20 ^ 0x30` is `0x10`. `1011-1010` is 1, `01^10^9` is 2, and
`I-(O<<l<<O)` is `argc - 2`, which is always 0 at that point.

4. Excessive shifting

Numbers are written as chains of shifts: `1<<1` is 2, `1<<1<<1<<1<<1` is 16 and
`1<<(1<<1<<1)<<1` is 32 (the space character). `O<<O<<1` is 4. Each shift
doubles the value, so counting the shifts gives the number.

5. Strings selected from a table and lengths used as numbers

The messages live in the array `QI`, and the code picks them with computed
indexes such as `QI[lll]` (three spaces) and `QI[01^10^9]` (newline). The hex
loop runs from `strlen(QI[4])` (the empty string, 0) to `strlen(QI[0])` (the
usage message, which happens to be 16 characters long), so the line width of 16
is hidden inside the length of a message. The strings also use octal and hex
escapes (`\012` newline, `\011` tab, `\x25` `%`, `\045` `%`) instead of plain
characters.

6. Expressions used as statements

The argument and file checks are written as `a && (b, c)` joined by commas
instead of `if` statements: the right side of `&&` only runs when the left side
is true. The printing loop uses `cond ? (...) : (...)` to choose between
replacing an unprintable byte with `'.'` and printing a space for bytes past
the end of the file.

7. Disguised loop conditions and increments

`(Q & ~(0x10-O)) == l` means "Q has no bits above the low four", i.e.
`Q < 16`. `Q += Q<0100` adds the result of a comparison (always 1 here) instead
of writing `Q++`. `while (L-l,1)` uses the comma operator so the condition is
just `1`, an endless loop that is left with `break` at end of file.
`*(OL+O1Q(Q-l))` is `OL[abs(Q-0)]`, which is simply `OL[Q]`.

8. Misleading comments and dead code

`/*"  O10(QI[1O])*/` looks like a string or a call to exit but is a comment,
and `/* 0010 10lOQ 000LQL */` is noise. The `if (I == (1<<1))` test is always
true because the program already exited when `argc` was not 2, and the empty
block `{}` does nothing.

9. Old style function definitions

`main(I,Il) lOL*Il[];` declares the parameter types after the parameter list,
and `D(l)` has an implicit `int` return type and `int` parameter. `D` is used
before it is defined, relying on an implicit declaration. The escape `'\~'` is
not a real escape sequence and is just `'~'`.
