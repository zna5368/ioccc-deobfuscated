# 1993/ejb

Difficulty: Hard

1. Disguised string literals

The two strings the program prints are stored as one `signed char a[]` array of
numbers such as `0x69`, `110`, `'o'/3`, `2*'2'`, `'%'+'%'+'%'` and
`1<<1<<1<<1<<1<<1`. Each of these works out to a character code, so the array
actually spells `"invalid"`, a `0` byte, and then
`"move ring %d from stack %d to stack %d\n"`. The pointer `b = a + (1<<1<<1<<1)`
skips 8 bytes to reach the second string, which is used as the `printf` format.

2. Obscured numeric constants

Small numbers are written in roundabout ways: `1-1` is 0, `1<<1` and `1- -1`
are 2, `-0200` is octal for -128, `0x7f` is 127, and `100-001` is 99. Even the
test `x - -x > 1-1` is just `2*x > 0`, that is, "is the level positive?".

3. Whole program as one expression

`main` contains a single `return` statement made of `,`, `||`, `&&` and `?:`
operators. The comma operator runs things in sequence, `a || b` means "only do
`b` if `a` was false", `a && b` means "only do `b` if `a` was true", and `?:`
replaces `if`/`else`. Together they replace every `if`, loop and separate
statement that a normal program would have.

4. Faking statements with `0 * puts(a) + 1`

Error handling is written as `r = ... ? 0 * puts(a) + 1 : 0`. The `puts` call
prints `"invalid"` as a side effect, multiplying its result by 0 throws the
result away, and `+ 1` makes the whole value 1, which becomes the exit status
and also stops the rest of the `||` chain.

5. Recursive `main` instead of helper functions

`main` is the only function, so it calls itself with `main(2, e)` to act as
both the Tower of Hanoi solver and the Patience solver. `main` tells the first
call apart from the recursive ones by looking at `argc` and `argv`: a real call
with exactly one argument has `argc == 2` and `argv[2] == NULL`.

6. Hiding data inside the `argv` NULL terminator

`q = (signed char *)(e+1+1)` points at the bytes of the NULL pointer stored in
`argv[2]`. The program uses those bytes as four small variables: `q[0]` the
ring number, `q[1]` and `q[2]` the source and destination pegs (or the target
`'0'`/`'1'` state), and `q[3]` a flag choosing Hanoi or Patience. Writing to
them makes `argv[2]` non-NULL, and that is exactly how each recursive call
knows it is not the first call. This replaces function parameters.

7. Saving and restoring shared state around recursion

Because `q[]` and `c` are shared by every level of recursion, each recursive
call is surrounded by code that changes them and puts them back, e.g.
`q[0]--, d = q[2], q[2] = c, main(2, e), c = q[2], q[2] = d, q[0]++`. The
unused parameter `d` is reused as a local temporary.

8. Negative indexes and bit tricks for the Patience puzzle

The ring number is stored as a negative value, so tests such as
`-1 - q[0] - 1 == 0` mean "ring 2" and `w[q[0] - x]` finds the character for
that ring. The target state is the character `'0'` or `'1'`, and `q[1] & 1`
reads its lowest bit to choose the order of moves, while `q[1] ^= 1` flips
`'0'` to `'1'` and back.

9. Unusual layout

The code is formatted into a decorative shape with large blank gaps in the
middle of an expression, making it hard to see where each part begins and
ends.
