# 2006/hamre

Difficulty: hard

1. Source shaped as a picture

The whole program is laid out as a big triangle with the code split into short
pieces, sometimes in the middle of an expression or a string
(`"%d/%d"` and `"\n"` sit on separate lines and get joined by the compiler).
Macro definitions are broken across lines with a backslash (`r(v,a\` /
`)v<0&&(`). The layout hides where statements start and end.

2. One-letter macros for array slots

`#define q [v+a]` turns `2 q` into `2[v+a]`, which is the same as `v[a+2]`
because `x[y]` in C just means `*(x+y)`. `o` and `O` are shortcuts for `0 q`
and `1 q`, and `c` is `b[1]`, the expression string. So `o=o*s q` really means
`v[a] = v[a] * v[a+s]`. simple.c gives each slot a name: `frame[NUM]`,
`frame[DEN]`, `frame[LEVEL]`, and the next frame's slots via `operand[...]`.

3. One macro for every operator

`p(v,m,s,w)` builds the code for a whole operator: compare the current
character with the operator (`*c==*#v`, where `#v` turns the macro argument
into a string and `*` takes its first character), check the precedence `m`,
skip the character, run the operand, then compute the new numerator from `s`
and the new denominator from `w`. `p(+,3,4 q+O*3,4)` expands to
`o = o*4 q + O*3 q, O = O*4 q`, i.e. a/b + c/d = (ad + bc)/(bd). The
parentheses use the same macro: `p((),d,0+3,0+04)` matches the first character
of `"()"` and the trick `o*0+3 q` throws the old value away and copies the
result. The unary minus is hidden in the precedence argument
`(o?3:(O=1,6))`: if the value is still 0 it sets the denominator to 1 and uses
the higher level 6. simple.c writes each operator as its own `case`.

4. The whole parser is one expression

All the operators, the `)` check, the digit case and the final `printf` are
chained together with `?:` and the comma operator inside one `if (...) return 1;`.
The result of each operator is the new denominator, so a division by zero
gives 0 and quietly stops the program without printing. simple.c uses a
`switch` in `step()` and returns `frame[DEN] != 0`.

5. Recursive `main` as the parser

`main(a+3,b)` evaluates an operand in the next three slots of `v`, and
`return main(d,b)` at the end goes around again to read the next character.
The `a` argument (first `argc`) is the index of the current frame, which is why
extra command line arguments use up stack space. simple.c has an `evaluate()`
function with a `while` loop and an `operand_of()` helper for the recursion.

6. A global used as a scratch variable

`d` is set to 7 at the start of each call so `(` always passes its precedence
test, set to 1 just before `5 q=m` so that a `(` starts its operand at level 1,
and then reused to hold the frame index and, by multiplying with -1, the sign
while reducing the fraction (`r(o,d)`, `r(O,d)`, `r(d,o)`). simple.c uses
constants and a separate `negative` flag.

7. Euclid's algorithm spread over the array

To reduce the fraction, the larger and smaller value are copied after the
frame and `while(2 q=o%1 q)a++;` slides `a` forward, writing each remainder
into the next slot until it is 0. The last non-zero value is the gcd. This
reuses the stack area of finished operands, and those leftover values are what
an operand's denominator starts with, so simple.c keeps the same layout in
`reduce()`.

8. Octal and odd number spellings

`012` is octal for 10 (used to build decimal numbers digit by digit), `02` and
`04` are just 2 and 4, and `3-2` is 1. Any character that is not an operator is
treated as a digit, so a space becomes `' '-'0'` = -16, which is why
`12 / 15 / 15` gives a strange answer.
