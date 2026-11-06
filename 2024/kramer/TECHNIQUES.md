# 2024/kramer

Difficulty: Medium

1. Hashed words instead of strings

The program never has the words "one", "plus", "thousand" and so on in its
source. The function `l()` turns each word into a number from 0 to 255, and
the program only compares those numbers, for example `C(201,1)` means "if the
word hashes to 201 (which is "one") then add 1". A reader has to run the hash
on every English word to find out which number stands for which word.

2. Macros that hide code

`C(a,b)` expands to `if(l(*v)==a){n+=b;}`, `D` is built on top of `C`, and `E`
and `F` hold whole blocks of the shunting yard algorithm. Arguments are abused
so that `b` is not just a number: in `C(16,0;E(F(5)))` the text `0;E(F(5))`
is pasted in, so it becomes `n+=0;` followed by more statements. The real code
only appears after the preprocessor has run.

3. Comments used as decoration and to break up tokens

The source is filled with comments like `/*  7 */` and `/*----*/` that draw a
calculator keypad and the text "nine thousand and one". They are placed in the
middle of expressions such as `C(/*  7 */223/*  8 */,18/*  9 */)` so the
numbers are hard to pick out.

4. Do-nothing arithmetic

Some numbers are written as `1*121`, `+40` or `1*1*1`. Multiplying by one or a
leading plus sign does not change the value, but it makes each line look
different and helps fill the keypad picture.

5. Short meaningless names

Every variable is one letter: `m` is the number being built, `n` is the
current group under one thousand, `o` is the sign, `i` is the end of the output
queue, `g` is the top of the operator stack and `h` is the read position while
evaluating. The struct `e` with fields `b` and `d` is a token holding an
"is a number" flag and a value.

6. Two data structures in one array

The single array `f[1000]` is used as both the output queue (filled from index
0 upward with `f[i++]`) and the operator stack (filled from index 1000
downward with `f[--g]`). Because they share memory it is not obvious there are
two different structures.

7. XOR used as "not equal"

Loops like `for(;g^A;...)` and `for(;h^i;)` use `^` (exclusive or), which is
zero only when both sides are the same. So `g^A` means `g != 1000`.

8. Precedence packed into the operator code

The operators are stored as the numbers 1, 2, 5, 6 and 7. Shifting right by two
(`a>>2`) gives 0 for plus and minus and 1 for times, divide and mod, so the
precedence comparison `a>>2<=f[g].d>>2` needs no table.

9. Nested conditional operator

The evaluation step is one long chain
`m^1?m^2?m^5?m^6?j(o%n,1):j(o/n,1):j(n*o,1):j(o-n,1):j(n+o,1)` that picks the
math to do based on the operator code. Reading it requires matching each `?`
with its `:` from the inside out.

10. Reusing variables

After the words are read, the same variables `n`, `o` and `m` are reused while
evaluating to hold the right operand, left operand and operator code. The
hundred multiplier is written `n+=n*99` instead of `n*=100`, so it fits the
same `C()` macro as every other word.
