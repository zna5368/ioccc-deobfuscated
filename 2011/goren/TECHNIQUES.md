# 2011/goren

Difficulty: Hard

1. Branch-free selection with bit masks

`Q(a){return(a|-a)>>31;}` turns any value into a mask: `0` when `a` is zero
and `-1` (all bits set) otherwise, because `a|-a` has its sign bit set for
every nonzero `a` and an arithmetic right shift spreads that bit. `S(c,a)`
then computes `a&~c|Ar&c`, which picks `a` when the mask is `0` and the
global `Ar` when the mask is all ones. Comparisons are done with XOR, e.g.
`Q(*r^c)` is `0` only when `*r == c`. Together these replace every `if`,
`?:`, `&&` and `||` in the program.

2. Function pointers and recursion instead of loops

The program is a small state machine. `Ar` holds the "current state"
function, `R(h,c)` sets `Ar` to `h` only when the condition mask `c` is
zero, and then calls `main()`, which calls `Ar` (or `a` on the very first
call, when `Ar` is still null). `e(c,h)` means "if `*r == c` move on to
state `h`, otherwise run the current state again". So a function like
`f(){P(O(r));e('f',g);}` is really the loop "print characters until `f` is
found, then go to `g`", written as recursion. Every step goes deeper in the
call stack and nothing returns until the last state `z` prints the final
newline.

3. Run-length encoded picture

The ASCII art lives in `l[]="|\\/=_ \n](.\0(),*(.(=(}*.)[[*."`. The first
seven bytes are a palette of characters. Each following byte is a code: its
low three bits (`*r%8`) pick a palette character and its high bits minus 4
give how many times to print it. Function `w` subtracts 8 from the code
after each print and steps to the next code once only the base value 32-39
remains. For example `}` is 125, which is palette entry 5 (space) printed
11 times. `I()` picks the start position: `l+7` for the small `|` drawing
when the first input character is `|`, or `l+11` for the larger pipe
drawing otherwise.

4. Self-modifying data used as a hidden flag

The decoder changes the `l[]` array as it runs (`Z(r,8)` subtracts from the
code bytes). Afterwards `c()` looks at `r[-2]`, two bytes before the end of
the drawing it just printed. It checks whether that byte equals `'"'`,
which only happens for the larger drawing, so it acts as a hidden "not a
pipe" flag without storing one.

5. Reused misleading string

`T()` points `r` at `"This is not a function\n"`. When the flag says a real
pipe was seen, states `d` and `p` print `This is `, skip the `not ` part,
then state `f` prints `a ` up to the `f` of `function`. Otherwise `f`
prints `This is not a ` directly. The word "function" is never printed.

6. Encoded word hidden in an out-of-bounds pointer

`ist="Rene Magritte"-(1898-1967)` uses Magritte's birth and death years to
move the pointer 69 bytes past the start of the string, and `M()` sets
`r=ist-68`, so `r` ends up at `"ene Magritte"`. `h()` reads that text three
letters at a time and mixes them with subtraction and XORs
(`(r[1]-r[2])^r[0]`, then `^30`, then `V()`) to produce one letter per group:
`ene`, ` Ma`, `gri`, `tte` become `p`, `i`, `p`, `e`.

7. Misleading and unused arguments

Functions declared with no parameters are called with arguments, such as
`P(O(r))`, `T(Ar=d)` and `P(P(*r++))`. The arguments are only there so
their side effects (printing, assigning, incrementing) happen first.
`e(w(O(...)),c)` passes the return value of `w`, which is always `0`, as the
comparison character. `w(U)` stores the garbage return value of `Z` into
its unused parameter `U`.

8. Implicit int and one-letter names

Every function relies on the old C rule that a missing type means `int`, and
all names are one or two letters (`t` for `unsigned char`, `H` for a
function pointer type, `F` for the `"%c"` format), which hides what each
piece does.
