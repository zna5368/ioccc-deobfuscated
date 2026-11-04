# 2011/konno

Difficulty: Medium

1. Recursive `main` instead of a loop

`main(2+_,l)` calls `main` again to print the next character, so the
program never uses `for` or `while`. The first parameter `_` (normally
`argc`) acts as the loop counter: `--_` subtracts one and `2+_` adds two,
so each call moves forward by one character position. Because counting
starts from `argc`, the counter's first value depends on how many
arguments are given.

2. Hidden stop condition

The recursion stops in `6*putchar(...)^_&&main(...)`. `putchar` returns
the character it printed, and `^` (XOR) is zero only when both sides are
equal. The only time `6 * character` equals the position is the newline
(10) at position 60, so `6*10 == 60` ends the recursion after the last
line. The `&&` skips the call to `main` once the left side is zero.

3. Keyboard layout computed with arithmetic

`--_%20` puts a newline every 20 positions. `_+_/21&56>_` decides whether
a position is a key or a space: `_/21` shifts the second and third rows
by one or two places, the odd/even test (`&` with 1) puts a key on every
other column, and `56>_` cuts the last row short. The simple version
replaces all of this with a picture of the keyboard as a string.

4. Encrypted letter table

The string ``"pt`u}rxf~c{wk~zyHHOJ]QULGQ[Z"`` does not contain the
keyboard letters directly. Each letter is found by XOR-ing the table
entry `[_/2]` with the position `_`, for example position 1 gives
`1 ^ 'p'` which is `'q'`. This is the "encrypted" part mentioned by the
author.

5. Character constants written as numbers

`111`, `46`, `32` and `10` are the ASCII codes of `'o'`, `'.'`, space and
newline. Writing them as numbers hides what is being printed.

6. Unusual array indexing and names

`1[l]` means the same as `l[1]` (that is, `argv[1]`), because `a[b]` is
just `*(a+b)` in C and addition works in either order. The parameters
are named `_` and `l`, which look like punctuation and the digit one.

7. Old-style declarations and missing headers

`main(_,l)char**l;` uses the pre-ANSI way of declaring parameter types
and relies on `main` defaulting to `int`. `putchar` and `strchr` are
used without including any header, so the compiler assumes their types.

8. Nested conditional operators

The whole output character is chosen with three nested `?:` operators
inside the `putchar` call, so the decision between newline, space, `.`
and `o` is packed into one expression.
