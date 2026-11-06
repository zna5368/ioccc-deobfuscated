# 2018/vokes

Difficulty: Terrifying

1. One function pointer that keeps changing

`static void (*_)();` is a single function pointer that the program points at
a different function almost every time before calling it. `_` is set to
`spectre`, `toad`, `newt`, `O`, `potion`, `witch`, `toil`, `nasal_demons`,
`hex`, `boil`, `bubble` and `cast` at different points. So a line like `_ (Q);`
can run any of them, and you only know which one by tracing what the last
assignment to `_` was. Many functions end by setting `_` to the "next step",
so they form a hidden chain.

2. Empty parameter lists that accept any arguments

Because `_` and `B` are declared with `()` and not `(void)`, the compiler
doesn't check how many arguments they get. `spell` calls `_ ()`, `_ (H)`,
`_ (H, G)`, `_ (H)` and `_ (G)`. Each call goes to a different function
(`toil`, `nasal_demons`, `hex`, `boil`, `bubble`), and the last one changes
`_` for the next call. Together these five calls are a counting sort. In
`familiar`, `_ (Y[i], c-1, &i[1])` passes three arguments to `spectre` when a
line had no numbers. `spectre` takes none and returns without doing anything.

3. Same name used for different things

`_` is a `goto` label in `main`, the name of the `enum`, and the function
pointer. `"\x5f"` is also just the string `"_"`. Labels, enum tags and
ordinary identifiers live in separate namespaces, so C accepts all of them.

4. Enum constants used as array indexes

`enum _ {F=6, I=7, N=5, D=3, M=14, Y=0, C=8, L=1, U=9, E=4}` gives
one-letter names to small numbers. They act as field offsets inside each node
array: `p[N]` is the visit index, `p[I]` the lowlink, `p[D]` the on-stack flag,
`p[F]` the edge count and `p[M+j]` the j-th edge. Sums like `p[F+L]` (= 7),
`b[M-N]` (= 9) and `b[N+D]` (= 8) hide which field is used.

5. Swapped array subscripts

In C, `a[b]` is the same as `b[a]`. The program writes `I[p]`, `N[J]`,
`E[b]`, `U[b]`, `C[b]`, `F[p]`, `Y[n]` and `y[w]`. These look like the
constant is the array, but they really index into `p`, `J`, `b` and so on.

6. Program state hidden inside a node

`b = a[t]` is the node array of the largest node id. Its unused slots hold the
algorithm's global state: `b[1]` is the component counter, `b[4]` is the
stack top, `b[8]` is the size of the current component and `b[9]` is the
capacity of the component buffers. That node is still a normal graph node in
its other slots.

7. Global variables reused for unrelated jobs

Almost every variable is a one-letter global with several jobs. `i` first
holds the numbers on the current line and later the counting-sort buckets.
`t` is the largest node id and later a sort loop counter. `h` is the capacity
of `i` and later the Tarjan index counter. `r` is the node table size and
later the stack capacity. `y` is a loop counter in several places and also the
largest id in a component. `c` is the count of numbers on a line and later a
buffer size.

8. Misleading names

The functions have witch-themed names that say nothing about what they do.
`potion` is Tarjan's `strongconnect`, `magic` walks a node's edges, `bat`
prints one component, `witch` pops one node off the stack, `toad` adds edges,
`newt` sets up a new node, `familiar` grows the node table and `spectre`
parses one number. `meltdown` is just `realloc` with an error check, `cast`
doesn't cast, `hex` isn't hexadecimal and `bubble` isn't bubble sort. In
`main`, `argv` and `argc` are swapped.

9. Hidden exit through a zero-size realloc

There is no normal `return` from `main`. It ends in `goto _`, which runs the
setup code again. After the graph is printed, `O` sets `h = 0`, so the next
`B(T, h*sizeof*i)` asks `meltdown` for zero bytes. `meltdown` then calls
`exit(n ? y != *n : I-F)`. The exit code is 0 when the loop in `O` visited
every slot, and 1 when there was input but no nodes. `cast` looks like the
exit path, but it is never called.

10. Function pointer swapped to an error handler

`B` normally points to `meltdown`. When the node table size wraps to zero
(the node id is too large), `familiar` sets `B = brew` and calls it. `brew`
prints `"Out of range."` and exits. The message is chosen by `l?"":...`, so
you have to know `l` is 0 there to know what gets printed.

11. Character tests written as arithmetic

The program never uses `isdigit` or `'0'`. `(W)*Z-060>011` is "not a digit"
written in octal (`060` is `'0'`, `011` is 9). `(W)*Z-(M*D+F)<=U` uses
14*3+6 = 48 = `'0'` and `U` = 9. `U>=(W)*Z-I*I+L` uses 7*7-1 = 48. Casting to
unsigned also makes characters below `'0'` wrap around to huge numbers, so
one comparison covers both ends of the range.

12. Hiding NULL

`NULL` never appears. `T=argc[argv]` copies the null pointer that always ends
the argument list, and `T` is then used as NULL everywhere. Examples are
`realloc(T, ...)` used as malloc, `strtok(T, " \t")` and `v=T`.

13. Loop control through side effects

How long the parse loop in `main` runs depends on `spectre` setting `_` back
to `spectre` only when the next token is a digit. Otherwise `_` stays `toad`.
After the loop, `_` is then either `toad` (add the edges) or `spectre` (do
nothing). In `O`, `bat` sets the shared loop variable `y` back to 0 in the
middle of the loop. This is harmless only because nodes already visited are
skipped.

14. Odd conditional expressions

`for(t=0;t<H?G:Y;t++)` in `hex` reads as `(t<H) ? G : 0`, so it runs while
`t<H`, because `G` is never 0. `exit(g?1:1)` always exits with 1.
`b[L+C]=b[L]^=b[L]` zeroes two fields at once with XOR.

15. Shaped source code

The code is laid out as a bubbling cauldron sitting on a big underscore. The
spacing breaks up tokens and expressions, so the structure is hard to see.
