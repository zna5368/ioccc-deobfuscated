# 1998/fanf

Difficulty: terrifying

1. Macros that write macros (two preprocessor passes)

`#define l #define` and `#define Xinclude #include` mean that lines like
`l i int` are not definitions yet. The first `cc -E` pass turns them into
`#define i int`, and only the second `cc -E` pass uses them. The same goes for
`p(p)`: `p (S)` becomes `#define S b ( m S u X )` after the first pass. You
have to run the preprocessor twice in your head to see the real program.

2. One or two letter macros for C syntax

The engine is written with macros such as `x` for `){ return (`, `A` for
`->`, `N` for `struct n *`, `t` for `) ? p : (`, `z` for `, p` and `Q` for
`= p`. Larger macros are built from these: `w` means "if there is no parent
return, otherwise climb to the parent", and `W`, `V`, `D` stack more pieces on
top. A line like `K W V L R f` is a whole function once all the pieces are
expanded, but the letters give no hint of that.

3. Same name used for different things

`p` is a macro, a function parameter and a global variable. `E` is a
function, a macro parameter and a function-like macro. `b` and `d` are both
functions that return 0 and function-like macros. Because a function-like
macro only expands when a `(` follows it, the preprocessor and the compiler
each see a different meaning for the same name.

4. Mutually recursive macros that turn brackets into code

`#define b(b) ( ( b a d` and `#define d(d) ( ( d a b` end with each other's
name. A macro can not expand itself, but `b` can produce a `d` that expands,
which produces a `b` that expands, and so on along a whole bracketed list.
With `a` being `) ) ) ? 0 : j ( !`, the list `(K I)` becomes
`(((push K))) ? 0 : j(!d(((push I)))) ? 0 : j(!d)`. Each push is 0, so `!0`
calls `j(1)`, which joins the top two nodes into an application. The last
`d` or `b` has no `(` after it, so it is a function name, `!d` is 0, and
`j(0)` does nothing. In this way the shape of the brackets in the source
becomes a tree of nodes in memory. In simple.c this is a small parser that
reads the same expressions from strings.

5. Linked list stack hidden in a tree field

While the tree is built, each new node's `U` field points to the node pushed
before it, so `U` works as a stack. Later, during evaluation, the same field
holds the parent application. One field does two unrelated jobs.

6. A whole program written in combinators

All the real work (reading input, skipping spaces, bracket abstraction,
optimising and printing) is written in the author's combinator language as
lines like `l opt b (S (S I (S (BB (CC B) CI ...`. Even knowing what `S`,
`K`, `B` and `C` do, these expressions are almost impossible to read by
hand. Data such as trees and true/false are encoded as functions (`K` is
true, `J` is false, `pair` and `atom` take functions to call).

7. Macro inlining blows up the code

Every name like `opt` or `trans` is a macro, so each use pastes in a full
copy of its definition, and those contain more names. The 2.2KB source grows
to about 700KB after both passes. simple.c also makes a fresh copy for each
use, but at run time from a short table of definitions.

8. Every function is a single expression

Each C function is `return ( ... );` built from `?:` and the comma operator,
with assignments inside, instead of `if` statements. For example
`(!p->U) ? p : (p->U->L = p, p = p->U, ...)` means "if there is no argument
stop, otherwise move up and continue". simple.c writes these as plain `if`
statements.

9. Function pointer as the node type

Instead of a tag field, each node stores a function pointer `F`. Calling
`p->F(p)` evaluates the node, and code compares `p->R->F != E` to check if a
node is a character. Nodes are changed in place, for example an application
becomes `v` (an indirection) when it is reduced, so the same memory changes
type while the program runs.

10. Hidden disabled code

`#if 0` hides a direct `Y` combinator and its `p (Y)` declaration, so the
`Y` actually used is the long combinator definition further down.
