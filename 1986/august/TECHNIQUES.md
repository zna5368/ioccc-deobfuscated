# 1986/august

Difficulty: Hard

1. Lazy infinite streams built from self-modifying linked lists

Every `t` node has a kind `a`, a value `b` and a next pointer `c`. A node whose
kind is not 0 is an unfinished computation (a thunk). Calling `q(p)` "forces"
it: the node computes its value, fills in its next pointer with another
unfinished node, and stays valid forever after. This turns a linked list into
an infinite stream where only the parts that are actually needed get computed.
The program never says this anywhere, so the reader has to work out the idea
from pointer assignments alone.

2. Function pointer table instead of a switch

`int(*y[4])()={r,m,g,h};` is an array of functions, and `q(p)` calls
`(*y[p->a])(p)`. The kind number stored in a node chooses which function runs.
`r` is an empty function for nodes that are already computed, `m` multiplies a
stream by 10, `g` produces the decimal digits and `h` propagates carries. Since
the kinds are plain numbers like `z->a=2`, you must look up the table every
time to see which function is meant.

3. Nodes that change their own meaning

The functions overwrite the node they are given instead of making new ones.
For example `g` takes the head of its input stream `z`, copies the value out
and then sets `z->a=2`, turning that old data node into the generator for the
next digit. `h` does the same with `z->a=3,z->b=c+1`, turning a digit into a
carry step for the next larger base. The same piece of memory means different
things at different times, which hides how the data flows.

4. A hidden mathematical algorithm

The number printed is e written in the factorial (mixed radix) number system,
where it is simply `2.1111...` with the n-th digit having base n+1. `main`
builds this as a node holding 2 followed by a node holding 1 whose next pointer
points to itself (`o->c=o`), an endless stream of ones. Each decimal digit is
found by multiplying all fractional digits by 10 (`m`) and carrying from right
to left (`h`). The test `e+9>=c` skips the carry when a carry from further
right could never reach the base, which is what lets the infinite stream be
processed lazily. None of this is explained, only the operations are visible.

5. Meaningless one letter names reused everywhere

The struct members are `a`, `b` and `c`, the functions are `f`, `g`, `h`, `m`,
`q` and `r`, and the local variables are `e`, `p`, `u`, `v`, `w`, `x` and `z`.
`c` is the next pointer of a node but also a local `int` holding a base in `h`,
and `e` is a node pointer in `main` but an `int` in `h`.

6. Comma operator and `&&` instead of statements

Whole function bodies are a single expression joined by commas, such as
`z=u->c,q(z),u->b=z->b*10,w=u->c=f(),...`. In `h` the `if` statement is
written as `e+9>=c&&(q(z),e=z->b,...)`, so the right side only runs when the
left side is true. `f` uses `i||(i=1000,k=...)` in the same way to refill its
supply of nodes only when `i` reaches 0.

7. Unusual indexing of a string literal

`&e->b["0123456789"]` looks like it indexes `e->b`, but `a[b]` in C means
`*(a+b)`, so `x["str"]` is the same as `"str"[x]`. It gives the address of the
character for the digit in `e->b`, which is passed to `write`.

8. Hand-made memory allocator with `sbrk`

`f` asks the operating system for room for 1000 nodes at a time with `sbrk`
and hands them out from the top down with `k+--i`. Nodes are never freed and
the fresh memory is relied on to be zero, so new nodes start as kind 0 with
value 0 without being set. The program keeps going until it runs out of memory
or stack space, which is why the judges mention a core dump.

9. Bit-fields and reused return values

`int a:3, b:29` packs the kind and value into one `int` using bit-fields, which
beginners rarely see. In `main` the return value of `write` (the number of
bytes written, 1) is stored back into `e->b`, an assignment that does nothing
useful and only distracts from the printing.

10. Old style function definitions and implicit types

Functions are written as `m(u)t*u;{...}` with the parameter type after the
parameter list, and they have no return type, relying on old C rules that make
it `int`. `q` and `write` are called before being declared, and the code is
packed into lines of equal width so function boundaries are hard to see.
