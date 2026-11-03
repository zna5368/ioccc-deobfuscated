# 1992/lush

Difficulty: Medium

1. Message hidden in compiler warnings

The text "Hello World." is not stored anywhere in the program. Instead, the
source is full of lines like `#define f 100:` that redefine the macro `f` over
and over. Each redefinition makes the compiler print a warning such as
`lush.c:44: warning: "f" redefined`. `lush.sh` pipes the compiler output into
the program, so the program's input is the list of its own compile warnings.
The line numbers of those `#define f` lines are the real data.

2. Bits encoded in the parity of line numbers

Only the last digit of each warning's line number matters: odd means a 1 bit
and even means a 0 bit (`n=n*2+(1&*t)`). Four bits are collected (`__=(__+1)&3`
counts 0 to 3 and wraps around) and then used as an index into the `av` table
of letters. The author placed the `#define f` lines on carefully chosen lines,
sometimes padding with useless `s=t;` statements, to get the right digits.

3. Skipping repeated digits

A warning is only counted when its last digit is different from the last one
used (`v^*t`, where `^` is "not equal" for single characters). This lets the
author put several redefinitions on lines that end in the same digit without
adding extra bits, which makes the pattern of `#define` lines harder to read.

4. Decoy values in the macros

The values in the macros (`000:`, `666:`, `77::`, `puts`, `s=5;`, `@3:@`) look
like numbers or code, but they are never used because `f` is never expanded.
Only the fact that the macro is redefined matters. The values just change so
that every line really is a redefinition.

5. Letter table with lookup by single-character strings

`av` is an array of one-letter strings like `"W"`, `"H"`, `"o"` instead of a
plain string. `*av[n]` takes the first character of the n-th string. The table
is in a scrambled order (W, H, o, l, N, e, ...) and contains unused letters
(`N`, `m`, `s`, `g`, `!`, `*`) so the message cannot be read from it.

6. Code written as a narrow column

Every line of code is only a few characters wide, splitting keywords,
expressions and string literals over many lines (`"I" "BM" " S" "UX"` is joined
into one string by the compiler). This hides the structure and also controls
which line number each `#define f` lands on.

7. Dead code

`_[i--]=0` sets `i` from 0 to -1, so the following `if(!i)` that would print
`IBM SUX` can never run. It is only there to distract the reader.

8. Inverted digit test with nested negation

The loop that finds the line number is written as
`!(!(*t<'0'||*t>'9')&&!((t[1]^':')&&(t[1]^'.')))`. Removing the double
negations gives "stop when this is a digit followed by `:` or `.`", which finds
the last digit of the line number in the warning.

9. Logical operators used as if statements

`__||((n<15&&(*p=*av[n]),p++),n=0)` uses `||` and `&&` to run code only when a
condition holds, and the comma operator to run several things in a row. Written
normally it is: every fourth bit, store the letter if the index is below 15,
move to the next output position, and reset the bit collector.

10. Useless statements and filler

Lines like `s=t;` and runs of empty statements `;;;;` do nothing. They exist
to pad the source so that each `#define f` ends up on a line number with the
needed digit.

11. Variables with meaningless names

The input buffer is named `_`, the bit counter is `__`, and other variables
are single letters (`t`, `v`, `n`, `p`, `x`), so their purpose is not obvious.
