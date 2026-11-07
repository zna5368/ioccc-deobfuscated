# 2025/dogon

Difficulty: insane

1. Only one variable

The whole program uses a single pointer `int*e`. Every "variable" is really a
slot in the array it points to: `e[0]` is the constant 4, `e[1]` the divisor,
`e[-1]` the limb index, `e[2]` the count of finished limbs, `e[-2]` the number
of limbs (8192), `e[3]` the base 100000, and `e+4` and `e+65536` are the two big
numbers. In `simple.c` each slot got its own named variable or array.

2. Numbers built from comparisons of the pointer with itself

There is not one digit in the source. `e==e`, `!(e>e)` and `!(e<e)` are always 1,
and once `e` points at memory, `!e` is 0 and `!!e` is 1. These are then used as
the building blocks for every other number.

3. Increment and decrement written with `-~` and `~-`

`~x` is `-x-1`, so `-~x` is `x+1` and `~-x` is `x-1`. Chains like `-~-~-~!!e`
mean 1+1+1+1 = 4. A `-` placed right after another operator becomes a sign, so
`calloc(...) -~-~-~-~(e==e)` actually adds 5 to the pointer.

4. A stored value used as a constant

After `e[!e]=-~-~-~!!e` sets `e[0]` to 4, the code writes `e[!e]` everywhere
it needs a 4 and builds bigger numbers from it, for example
`e[!e]<<e[!e]+e[!e]` is `4<<8` = 1024 and `!!e<<e[!e]*e[!e]` is `1<<16` = 65536.

5. Strings hidden inside integers

There are no string literals. Each format string is computed as an `int` whose
bytes spell the text, then the address of that `int` is cast to `char *` and
given to `printf`. For example `e[-4]` becomes 0x64353025, which in memory is the
bytes `%05d`, and `e[1]` = 11826 is the bytes `2.`. The zero bytes that end each
string come from the next array slot, which `calloc` cleared. Because the byte
order matters, a test on `*((char*)(e+2))` checks if the machine is little or
big endian and picks a different set of numbers.

6. Macros that hide names

`#define E printf` and `#define F char` mean the source never spells out
`printf` or `char`, so `E((F*)(e+!!e))` is really `printf((char*)(e+1))`.

7. All the work done in the `for` header

The setup, the end test, the final printing, and the loop step are all in the
three parts of the `for`. The end test `e[-~!!e]<e[~-~!e]||(...,0)` runs the
final "IOCCC 2026" printing when the loop is over and then returns 0 to stop.

8. Comma operators and short circuit operators instead of statements

Long lists of assignments are joined with `,` so they count as one expression.
`&&` and `||` take the place of `if`: `!A[k]&&(k<2||!printf(...)||(k%16&&...)||printf("\n"))`
means "if this limb is done and it is not one of the first two, print it, and
print a newline after every 16 limbs". `~~!printf(...)` is just a way to turn
the return value into 0 so the next test always runs.

9. Reversed and nested subscripts

In C `a[b]` is the same as `b[a]`, so `e[-~!!e][(e+(!!e<<e[!e]*e[!e]))]` is
really `(e+65536)[e[2]]`. Assignments are also put inside subscripts, as in
`(e+e[!e])[*(e+65536)=e[!!e]=!!e]=!!e`, which sets three slots to 1 at once.

10. Unusual layout

The code is spread out with huge gaps of tabs and spaces so it forms a picture
when viewed with 8 column tabs, and tokens are split across lines, which hides
how the expressions are grouped.
