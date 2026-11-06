# 2019/lynn

Difficulty: Unreal

1. The program is a combinator interpreter running a hidden program

There is almost no compiler logic in the C code. The C code is a small lazy
graph reduction machine for 18 combinators (S, B, C, K, I, Y and some
others), and the Haskell compiler is a large combinator term packed into the
strings `y` and `n`. Reading the C code only shows you the machine. What the
program does is decided by data you cannot read until it is decoded. In
`simple.c` the combinators get names (`S`, `K`, `CONS`, `PUTCHAR`, ...) and
each reduction rule is a function such as `reduce_s` with the rule written in a
comment.

2. Huffman coded term in base 85

`y` is a base 85 string: every 5 characters (each minus 32) make one 32 bit
word, `M=85*M+*y++-32`. `W()` reads the words one bit at a time from the top
bit down (`M&1<<S`) and walks a Huffman tree: the code so far is `I=_*2+bit`,
and it is looked up in the first 11 characters of `n` (`" $6I7H#v Jw"`), where a
character `c` stands for code `c-32`. The position where it is found is the
symbol: application, reference, integer literal or one of the combinators. In
`simple.c` this is `read_bit`, `find_symbol` and `decode_term`.

3. Numbers written in whitespace (mixed radix)

Numbers in the term are not in `y`. They are in the rest of `n`, written with
the six characters `{`, vertical tab, `}`, form feed, `;` and tab. These
appear verbatim in the original source, so they look like ordinary blank space
and braces. `N()` turns them into a number with digits that switch between
base 6 and base 3 (`l+I/_*L`, `L*6/_`, `3-_`). Any other character ends the
number. The author did this because the IOCCC size tool does not count
whitespace or braces. `simple.c` writes the control characters as `\v`, `\f`
and `\t` and has a loop, `read_number`, in place of the recursion.

4. String literals and the output header hidden in the same stream

The words the compiler's lexer needs (`let`, `in`, `case`, `of`, `data`,
`->`, the operator characters) are mixed in with the whitespace numbers and
end with a tab. `Z()` reads them. The text left in `n` after decoding is
`#define x \`, which `puts(T)` prints as the header of the output. The text
left in `y` is `/*`, which `puts(y)` prints as the footer.

5. Self-gluing output through the comment trick

The compiler prints `#define x \`, a list of numbers, and `/*`. Then
`cat prog.c` is appended. Because C comments do not nest, that `/*` comments
out the first line of `prog.c`, `#define x 0/**/`, up to the `*/` after the 0.
Now `x` is the list of numbers and `j[L]={x}` loads the compiled program into
the heap. If `j[0]` (named `E`) is not zero, the program runs the loaded heap
instead of decoding the compiler. `simple.c` keeps `#define x 0/**/` as its
first line so the same trick still works with it.

6. Jump table built from a macro and a compound literal

`X(f,b,y,d)` defines each combinator as a function that overwrites the node
of its last argument with the pair `(y d)` and pops `b` stack entries. With
`B(f,o)` on top of it, every rule fits on a few characters. The functions get
one-letter names (`O,b,f,u,s,c,a,t,e,d,_,p,r,o,g,R,A,m`) and are called through
an unnamed array literal, `(void(*[])()){...}[M*(M<18)]()`, inside a ternary
with a dummy `,7` so both branches have a value. `simple.c` writes each rule
as a function that calls `rewrite()` and puts them in a named array `reduce[]`.

7. Numbers that apply functions to themselves (Reduceron trick)

The integer `n` is the node `(1 n)`, and `1 n f` rewrites to `f (1 n)`.
Combinators 18 to 23 do not exist in the jump table (`M*(M<18)` sends them to
entry 0, `O`). `O` rewrites `op a b` to `b (a (op-8))`, so both integers
evaluate themselves and then hand themselves to the real operator 10 to 15.
Arithmetic is strict without any explicit "evaluate this argument" code.

8. Scott encoded data and booleans

Lists are functions: Nil is `K` (8), and Cons is combinator 17,
`17 h t n c -> c h t`. True is `K` and False is `K I` (`l(8,9)`). The
comparison `_` and the input function `t` build these directly. The output
loop applies the output list to a Nil case and a Cons case,
`l(l(l(main,l(7,0)),0),l(23,9))`, and `R` passes the return value of `printf`
as the Nil case of the rest of the list. That value is never used, so it only
adds confusion.

9. Sentinels in place of end conditions

Nothing explicitly says when to stop. Decoding stops when a back reference
`7[D-C]` points before the first definition and reads a 0 from the empty
heap. Reduction stops when the zero cell below the root of the stack comes to
the top (`while(M=*i,M)`). The heap image puts its size in `j[0]` and the main
node in `j[1]`, and `*--D` picks up `j[1]` after `D=j+2`.

10. Recycled and shadowing variable names

Globals are reused for unrelated jobs. `M` is the 32 bit Huffman buffer while
decoding and the current combinator while reducing (`O` uses `M-8`). `S` is
the bit counter and also the character from `getchar`. `D` points at the
decoded definitions and is also the scratch node pointer in `X`. `N`'s
parameters are called `l` and `L`, which hide the function `l` and the heap
size `L`, and `G`'s second parameter hides the global `S`. `simple.c` gives
each job its own variable.

11. Typical C mischief

The code uses commutative indexing (`_[i]` for `i[_]`, `7[D-C]` for
`D[7-C]`, `I[v]`, `M[j]`), deeply nested ternaries with a comma operator in the
middle (`C?_?--_<2?C=N(0,1,1),_?l(_,C):...`), `if(S--); else`, `!!` to make a
bit 0 or 1, and `return E||puts(y)`. Several calls such as
`l(l(17,l(1,S)),l(7,0))` allocate in whatever order the compiler evaluates the
arguments. That changes where nodes go in the heap but not the result.
