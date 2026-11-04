# 2012/grothe

Difficulty: Medium

1. Meaningless short names

Every global is one letter, often with upper and lower case pairs that mean
unrelated things: `o` (input denominators) and `O` (output points), `w` (the
multiplication table) and `W` (the inverse table), `z` (output count) and `Z`
(current input bytes). The functions are `f`, `t`, `y`, `a` and `X`, which say
nothing about printing errors, multiplying, or interpolating.

2. Shadowed names

The same letters are used again as locals and parameters, which hide the
globals. `main(int t, char *T[])` reuses `t`, which is also the name of a
function. `t(int i,int j)` declares its own `k`, `l`, `n`, `m`, and the main
loop opens a block `{int n; n=getc(...)}` so `n` there is not the global `n`.
In `X(int v,int u)` the parameter `u` hides the input counter `u`, and `char*y`
hides the function `y`. A reader has to check the nearest braces to know which
variable a letter means.

3. Reversed array indexing

In C `a[i]` means `*(a+i)`, which is the same as `*(i+a)`, so `i[a]` also
works. The code writes `i[I]`, `i[O]` and `u[K]` instead of `I[i]`, `O[i]` and
`K[u]`, which makes the arrays look like indexes.

4. Encoded error messages

The error strings are stored shifted, for example `"EvqmjdbufJoqvuQpjou"`.
`f(y,v)` prints every character minus `v`, where `v` is the same number as the
`case` label that chose the string, so each string uses a different shift
(`E`-1 = `D`, so this one is "Duplicate Input Point"). Before each capital
letter it also prints a space, which is why the strings need no spaces. Some
strings are split into two literals (`"HckngfVqQrgpKprwv" "Hkng"`), which C
joins back together.

5. Hidden macro and shuffled switch

The Makefile passes `-DBC='break;case'`, so `BC(4):` in the source is really
`break;case(4):`. The `case` labels are written out of order and in a mix of
decimal, hex and octal (`case(0x2)`, `BC 0x1`, `BC(07)`), so the list of
messages is hard to match with the error numbers. `X` also subtracts one from
its argument first, so `X(3,0)` selects `case 2`.

6. Error function used as a normal exit

`X(v,u)` with a nonzero `u` just calls `exit(0)`. The program uses
`X(42,1)` to finish when an input file ends, and `X(11,1)` after a
`while(!(0))` loop that never ends, so a normal exit looks like an error call.

7. Field multiplication without `*`

The shares are computed in a 256 element field (GF(256)) where adding is XOR.
The multiply is the nim-multiplication: `y(a,b)` XORs together the products of
every pair of set bits, and `t(i,j)` multiplies two powers of two by splitting
them into "Fermat powers" `2^1`, `2^2`, `2^4`. Equal Fermat powers multiply to
`F ^ F/2` (`1<<n|1<<(n-1)`), and different ones are just shifted (`1<<l`).
None of this uses the `*` operator, so the math is very hard to recognize.

8. Memo table filled by a side effect

`y` caches every product in `w[a][b]` and `w[b][a]`. The first loop in `main`
looks like it only builds the inverse table `W`, but calling `y(i,j)` for every
pair also fills the whole `w` table. The rest of the program then reads `w`
directly as the multiplication table, which only works because of that loop.

9. Lagrange interpolation in one expression

`a()` computes each output byte with
`n^=w[Z[j]][w[I[i]][W[w[o[j]][O[i]^q[j]]]]]`. That is
`y_j * numerator / (denominator_j * (x - x_j))` with every multiply written as
a table lookup in `w` and the divide written as a lookup of the inverse in `W`.
The numerator `I` and denominators `o` are worked out ahead of time in `main`.

10. Oversized arrays and mixed number bases

All the arrays only need 256 entries, but they are declared as `q[0400]`,
`O[0x101]`, `o[0401]`, `I[257]`, `w[258][0403]`, `Z[0x103]`, `J[0416]`,
`K[280]` and so on. Mixing octal, hex and decimal sizes hides that they are
almost all the same.

11. Inconsistent and odd code style

The same thing is written in different ways: `i++`, `++i`, `j+=1`, `e+=1`;
`if(!(u!=0))` for `if(u==0)`; `while(!(0))` for a forever loop; `040` for a
space; `toupper('0')` and `tolower('9')` for the plain characters `'0'` and
`'9'`; and casts like `(void)putc(...)` that change nothing.
