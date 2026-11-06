# 2024/endoh1

Difficulty: Insane

1. Code generator hidden behind generator macros

`prog.c` does almost nothing itself: it only prints the real program `rt.c`. Every
line is printed by a short macro such as `d(s,a)` (`#define s a`), `g(s,a)`
(`#define s(a,b) a`), `e(s,x,y,z)` (`#define s x,y,z`), `b(s)` (`#if s`), `u(s)`
(`#undef s`), `l` (`#else`), `n` (`#endif`) and `L` (`#include __FILE__`). So a
line like `d(Z,l(f)-k(G,(d(V,V)-l(q))))` is really one line of output text, not code.

2. Stringification keeps macro names from expanding

The generator macros turn their arguments into strings with `#a`. Text inside
`#a` is never expanded, so `d(M,m(d(i,i)))` prints `#define M m(d(i,i))` even
though `d` is itself a macro in `prog.c`. Without knowing this, the nested `d`
looks like a recursive call.

3. The same one-letter names mean two different things

`prog.c` and `rt.c` reuse the same letters for unrelated jobs. In `prog.c`, `T`
emits the lighting code, `l` prints `#else`, `L` prints `#include`, `d` prints a
`#define` and `p` is `puts`. In `rt.c`, `T` is the checkerboard test, `l` squares a
number, `L` maps a function over a vector, `d` is the dot product and `p` is the
sphere center. Inside `rt.c` letters are also reused: `R` is first the sphere hit
distance and later the floor hit distance, and `Z` is the discriminant during
tracing but the final color at the end.

4. Unbalanced `#if`/`#endif` across macros

The macro `T(x)` in `prog.c` ends with an `#endif` that closes an `#if R` printed
before `T` was used, so the `#if` and `#endif` pairs never match up inside one
macro. `simple.c` prints the closing `#endif` where the `#if` is opened.

5. Loops hidden inside macros

`f(s)` contains a `for` loop that prints 30 `#if`/`#define`/`#else`/`#endif` blocks,
and `r(s)` and `J(a,b)` hold more loops. A single short call such as `f(M);`
therefore prints about 150 lines. The `for(i=31; --i; n)` loop also hides an
`#endif` (the macro `n`) in its increment part.

6. Copying a value bit by bit (in `rt.c`)

The preprocessor normally evaluates macros lazily, so a macro cannot keep an old
value. `f(s)` (now `copy_value`) tests each bit with `#if (s)/2^i&1`, which is
evaluated right away, and records the bits in helpers `s30` ... `s0`. Then `s` is
redefined to that frozen constant. `F(s)` (now `copy_vector`) does this for all
three parts of a vector. This also stops the macro expansions from growing out of
control.

7. Vectors stored as comma lists

A vector like the sphere center is a single macro `p` that expands to
`0,500,4000`. The helpers `A`, `B` and `C` pick the x, y or z part, and
`N(f,a,b)`/`O(f,a,s)` apply an operation to all three parts. The extra macro
`c(a,b)` is needed so the commas are split into separate arguments.

8. Fixed point arithmetic

There are no fractions in the preprocessor, so `1000` stands for `1.0` (`S`).
`k(a,b)` multiplies and divides by 1000 and `U(a,b)` divides and multiplies by 1000.
`m(x)` is a square root built from five Newton steps of `n(a,b)`.

9. Recursion through self inclusion

`#include __FILE__` makes `rt.c` include itself. The first pass (when `S` is not
yet defined) sets up the math and the primary ray, then includes itself to trace.
Inside the trace, a hit on the sphere sets a new origin and direction and includes
the file again to follow the reflected ray.

10. Printing numbers by gluing digits

The final colors are long expressions, so `print_component` picks each decimal
digit with `#if (c(A,Z))/100%10==N` and glues the digits together with the `##`
token pasting operator. The digit character comes from the loop variable `j`
counting from `'A'` to `'C'` (65 to 67), written as `for(j=64; j++<67; )`.

11. Packed format strings

Strings are put together from pieces such as `G#s" "#a`, where `G` is
`"#define "`, so the actual text of each line can only be seen after string
concatenation. `I(d,m,e)` even builds a `printf` format with `%%` from macro
arguments.
