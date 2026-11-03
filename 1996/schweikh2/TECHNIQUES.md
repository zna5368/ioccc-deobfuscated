# 1996/schweikh2

Difficulty: Medium

1. Token pasting with reversed arguments

The macro `#define e(p,o)o##p` glues two tokens together but puts the second
one first. So `e ( f , i )` becomes `if`, `e ( n , i )` becomes `in`, and
`e ( = , * )` becomes the operator `*=`. All keywords and multi-character
operators (`if`, `else`, `for`, `int`, `||`, `&&`, `<<`, `/=`) are written
this way, so none of them appear in the source as readable words.

2. Nested pasting to build long keywords

One paste only joins two tokens, so longer words are built in layers. `R(n,d)`
and `H(O,r)` both just call `e`, adding extra levels of macro expansion so the
already pasted pieces get pasted again. For example
`H ( R ( e ( d , e ) , e ( n , g ) ) , R ( e ( i , s ) , e ( n , u ) ) )`
expands to `ed`+`gn` = `gned`, `si`+`un` = `unsi`, and finally `unsigned`.
This is how `typedef`, `struct`, `static`, `void`, `return`, `main`,
`sscanf`, `while` and `unsigned` are produced.

3. Hidden library calls and types in macros

`P` expands to `printf (`, `M` to `static`, and `U` to `long`. Because the
opening parenthesis of `printf` is inside `P`, the calls `J = P D , f . n ) ;`
look like they have unbalanced parentheses.

4. String literals built by stringizing

`#define O( _ ) # _` turns any single token into a string, so `O ( % )` is
`"%"`. Adjacent strings are joined by the compiler, so `D` (`O(%) O(l) O(d)`)
is the format `"%ld"`, and `C O ( % ) O ( c ) C` is `" %c "`. The operator
table `"*/+-"` is written as `O ( * ) O ( / ) O ( + ) O ( - )`.

5. Single-letter names

Everything uses one or two letter names: `F` is the fraction type with fields
`n` (numerator) and `t` (denominator), `B` is the unsigned bit mask type, `I`
holds the operands, `w` the working terms, `p` the four parameters, `o`
prints a fraction, `n` reduces a fraction, and `J` stores the last `printf`
result which also becomes the exit status.

6. Whitespace art

A space is placed after every token and the code is broken into tab-aligned
columns so `grep -v '#'` shows a diamond pattern. The line breaks fall in the
middle of expressions, which hides the structure of the program.

7. Disguised numbers and comparisons

The default goal 42 is written `6 * 7`, the array size 20 as `4 * 5`, and the
newline is printed as the number `10` with `%c`. Comparisons are hidden too:
`f . t - 1` means `f.t != 1`, `! ( i > n )` means `i <= n`, and the loop
test `~ ( B ) 0 - W` is non-zero until `W` wraps around to all one bits.

8. Packed operator choices in a bit mask

Every combination of operators is one number `W`. Each operator uses two bits
(`0`=`*`, `1`=`/`, `2`=`+`, `3`=`-`), read with `m & 3` and moved to the next
one with `m / 4`. The test `(m & 3) < 2` means "multiply or divide" and
`m & 1` then picks `/` over `*`, or `-` over `+`. Counting `W` down from
`~(~0 << n*2)` (that is `4^n - 1`) to zero visits every combination.
