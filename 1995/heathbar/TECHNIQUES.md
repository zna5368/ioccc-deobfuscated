# 1995/heathbar

Difficulty: Easy

1. Lookalike macro names

Every token in the program is hidden behind a `#define` whose name is a
variation of `main`, such as `MAin`, `mAIN`, `Ma1N`, `mA1N` and `MAln`. The
names differ only in letter case, `1` versus `l`, and `i` versus `I`, so they
are very hard to tell apart. For example `Ma1N` is `(`, `ma1N` is `)`, `Main`
is `;` and `MAIN` is `int`. Running the preprocessor (or replacing each macro
by hand) turns the code back into normal C.

2. Lookalike variable names

The undefined names left over after macro expansion are the real variables,
and they are also spelled like `main`: `maIn` and `ma1n` hold the two numbers,
`maiN` holds the result, `Ma1n`/`mAin` are `argc`/`argv`, and `mAIn` is the
adder function. Renaming them to `first`, `second`, `sum`, `argc`, `argv` and
`full_adder` makes the code readable.

3. Reusing the name `main` for a local variable

Inside the adder function, `static int main = -1;` declares a local variable
called `main` that hides the real `main` function. It counts which bit is being
added (0 to 15) and is increased each time the function runs. In simple.c this
is the loop counter `bit`.

4. ASCII art layout

The macro names are arranged on screen to spell the word "main" in large
letters. The line breaks and spacing are only there to draw the picture and
have no meaning to the compiler.

5. Addition built from logic operators

Instead of using `+`, the program adds the numbers one bit at a time like a
hardware full adder. Exclusive-or is written the long way as
`(x || y) && !(x && y)`, and `!!(n & 1 << i)` pulls out bit `i` of `n` as 0 or
1. The sum bit is `a XOR b XOR carry` and the carry out is
`(b && carry) || (a && (b XOR carry))`. In simple.c these are written with the
`^` operator in a small `full_adder` function.

6. Nested calls instead of a loop

The 16 adder stages are run as 16 nested calls,
`mAIn(mAIn(mAIn(...(Ma1n)...)))`, where each call passes its carry out to the
next call as the carry in. Only bits 0 to 15 are added and the last carry is
thrown away, so the answer wraps around at 65536. simple.c uses a `for` loop
that runs 16 times.

7. Comma operator chains and hidden zero

The adder's return statement `return 0, sum |= ..., carry;` uses the comma
operator to do several things in one statement: the `0` does nothing, the
middle part stores the sum bit, and only the last part (the carry) is
returned. Similarly `Ma1n--` is mixed into the `atoi` calls so that, after both
numbers are read, `argc` has been counted down from 3 to exactly 0, which is
then used as the starting carry.

8. Tricky starting values

`maIn` starts at `-1` and is incremented to `0` before being overwritten,
and the bit counter `main` starts at `-1` so the first `++` makes it `0`. These
add noise without changing the result.
