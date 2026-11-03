# 1998/schweikh3

Difficulty: Hard

1. Short macros for keywords and pieces of code

The top of the file renames common C words to single letters: `Z` is
`static`, `g` is `struct`, `u` is `return`, `H` is `if(`, `_` is `->`,
`I` is `char*`, `U` is `=malloc(` and `V` is `(void)`. Others stand for
fragments that only make sense once expanded, such as `O` for `.st_size`,
`W` for `.st_dev`, `Y` for `S.st_ino`, `b(x)` for `,(int)x.st_nlink`,
`h(x)` for `=open(x,O_RDONLY)` and `o` for `||read(`. Some even hold
unbalanced brackets, for example `v` is `;}else`, so the braces in the
source do not seem to match. The clean version writes all of these out.

2. Names hidden in the Makefile

`M0` to `M5` are not defined in the source at all. The Makefile passes
`-DM0=sizeof -DM1=long -DM2=void -DM3=realloc -DM4=calloc -DM5=free`, so
you have to read the build command to know that `M4((size_t)i,1)` is a
`calloc` call or that `M1 s,c;` declares two `long` values.

3. A macro that writes whole functions

`D(N,t)` expands into a complete function named `N` that allocates a `t`
with `malloc` and calls `exit(1)` if that fails. `D(a,t)D(E,f)` therefore
creates two functions, one for tree nodes and one for list entries, in a
handful of characters. The clean version uses a single `xmalloc` helper.

4. Digraphs and trigraphs

`%:define` is the digraph spelling of `#define`, `<:` and `:>` are `[`
and `]`, and `<%` and `%>` are `{` and `}`. They make array indexes such as
`A[i:>` and the braces hidden inside macros look mismatched. The string
also contains `??/t`, the trigraph form of `\t` (a tab). Modern compilers
turn trigraphs off by default (the Makefile even uses `-Wno-trigraphs`),
so the program really prints the four characters `??/t`. The clean
version writes `?\?/t` so the output stays the same.

5. Split up format string

The only string literal, the `printf` format `"%ld\t%s\t%s??/t%c\t%d??/t%d\n"`,
is broken into about twenty pieces like `"%""l""d"` that the compiler glues
back together. One tab is also written as the octal escape `"\11"` instead
of `"\t"`.

6. Meaningless one letter names and reused globals

Structures are `f` and `t`, their fields are `n`, `x`, `d`, `i`, `s`, `c`,
`l`, `L`, `R`, and functions are `J`, `Q`, `C` and `P`. Inside `Q` there
is a variable `d` and a label `d:`, and `D` is both a macro and a variable.
The global `y` is used for the line length in `main`, for the file size in
`Q` and for `Q`'s return value. The globals `S` and `T` are filled by
`fstat` inside `Q` and then quietly read again by the `printf` in `C`. The
clean version gives everything a descriptive name and passes the `stat`
data as a parameter where it can.

7. Layout that hides the structure

The code is packed into a block of text with big gaps, with comments at
the start and end that read as a sentence. The line breaks fall in the
middle of expressions and macro names, so the shape gives no clue where a
function begins or ends.

8. Packed triangle matrix walked with raw pointers

`C` compares every pair of files with the same size. It keeps one flag
byte per pair in a single block made by `calloc`, laid out like the upper
half of a table. Instead of nested loops it walks one pointer `e` through
the block and updates `i` and `j` by hand (`if(++j==N){j=i+++2;}`, where
`i+++2` means `(i++)+2`). When files `i` and `j` match, it uses the
pointers `w` and `l` and the shrinking step `s` to set the flag for every
pair `(k, j)` with `k` between `i` and `j`, so those pairs are never
compared. The clean version uses two `for` loops and a `pair_index`
function that computes where a pair lives in the block.

9. `goto` and chains of `||`

`Q` checks five failure cases (open failed, `fstat` failed, sizes differ)
in one long `||` chain built from the `k(x,y)` macro, and jumps over the
reading code with `goto d`. The size check is written as subtraction,
`(y=S.st_size)-T.st_size`, which is true when the sizes differ. The reads
are checked the same way with `read(d,m,y)-y`. The clean version uses a
normal `if` block with `!=` comparisons.

10. K&R style `main`

`int main(M7,M6)I*M6;` uses the old style of declaring parameters after
the parameter list. `M7` is never defined, so it silently becomes an `int`
parameter, and neither argument is used. The clean version uses
`int main(void)`.
