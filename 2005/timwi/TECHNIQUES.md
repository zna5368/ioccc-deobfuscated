# 2005/timwi

Difficulty: Terrifying

1. Underscore and `l` macro names

Every token is hidden behind a `#define` whose name uses only `_`, `l`, `1`
and `I`, such as `_`, `__`, `___l`, `____1` and `______1`. These look almost
the same, so the reader has to expand each one by hand. `_` is `main(`, `__l`
is `int`, `____1` is `)?(` and `___1` is `_1&(__`. The variables use the same
style: `__I` holds the input and the program, `_I` is the BF tape, `l__` is the
data pointer, `l_` is the next input number, `_1` is the mode and `__1` is the
current program position.

2. Macros that split tokens and brackets

Several macros leave a bracket open or close one they did not open. `____1` is
`)?(` and `_____1` is `*__1%(__`. One macro on its own is not valid C, and only
the whole line put together makes sense. This hides where each expression
starts and ends.

3. Numbers built from `1+1+...`

The only digit used is `1` (`____`). `__` is `1+1`, `_____` is `1+1+1` and `___`
is `__+_____`, which expands to `1+1+1+1+1`. The macros have no brackets, so
they melt into the sums around them: `_____1+___ ____1` becomes
`*__1%(1+1+1+1+1+1+1)?(`, which is `c%7`. The array size `_l` repeats
`___)*(` to multiply five copies of `(1+1+1+1+1)` and get 3125. Even after all
the sums are worked out, the numbers (2, 3, 4, 5, 7, 8) have nothing to do with
the ASCII codes of the BF instructions.

4. Recursive `main()` with one `return`

The whole program is a single `return` statement with nested `?:` operators.
Every loop is a recursive call to `main()`. Reading the input numbers calls
`main(3, ...)` again for each number, and every BF instruction calls `main()`
for the next one and adds 1 to its result.

5. `argc` used as a set of mode bits

The first parameter of `main()` (`_1`) is not an argument count after the first
call. Bit 4 means "interpret the program", bit 8 means "execute" (it is clear
while a loop body is skipped), bit 2 means "read the next instruction with
`getchar()`" and bit 1 means "go back to reading when this loop ends". Before
interpreting, the low bits choose between the start (`1`) and reading numbers
(`3`). Expressions such as `_1&13|(_1&2?1:0)` change the bits between calls.

6. `argv` used as an `int` pointer

The second parameter `char **a` is copied into `int *__1`. The first call
ignores it and calls `main(3, l_)`, and from then on the second parameter
points into the `__I` array: first where the next input number goes, then
where the next program character goes. The parameter that should hold the
command-line arguments holds the program position instead.

7. Instructions found by remainders, not character codes

The program never compares a character with `'+'` or `'['`. It uses
remainders: a character is `[` when `c%7==0`, `]` when `c%5==3` and `c%3==0`,
`-` when `c%5==0` and odd, `<` when `c%5==0` and `c%4==0`, and so on. That is
why many other characters work as BF instructions, as the README shows, and
why space, tab, newline and `:` fall through to "do nothing".

8. Multiplying by zero to throw away a value

The instruction is run inside `( ... ) * (1-1)` or `( ... ) * (0)`. Its side
effect (such as `(*l__)++` or `printf`) still happens, but the product is
always 0, so the test that follows only sees whether the character was `]`.

9. Program stored only as it is read

Each instruction is read with `getchar()` into the next `int` of `__I`, right
after the input numbers, at the moment it is first needed. When a loop runs
again, bit 2 is cleared and the stored copy is used instead. Bit 1 remembers
that reading must start again after the loop's `]`, so no global variable keeps
track of how far the input has been read.

10. Loop ends found from return values

`main()` returns how far it went: the distance to the `]` plus 2. A `[` that is
skipped runs the body with the execute bit cleared and uses the result `_l_` to
jump past the matching `]` with `__1+_l_`. Nested brackets add up the same
way, so the matching bracket is found without a stack or a bracket counter.

11. Input numbers and program in the same array

The `,` instruction reads from `l_`, which walks through `__I`. If a program
reads more numbers than were given, it gets the empty slot left by the failed
`scanf()` and then the character codes of the program itself.
