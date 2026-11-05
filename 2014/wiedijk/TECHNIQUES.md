# 2014/wiedijk

Difficulty: Hard

1. Macros that write whole functions

`L(F,f,x,...)` and `l(F_,f,F(f))` look like ordinary calls, but each one expands
into complete function definitions. `L` creates two functions, and `##` (token
pasting) glues an underscore onto the given name, so `L(F,...)` makes both `_F` and
`F` even though `_F` is never written in the source. Searching for `_F` finds nothing
useful. `simple.c` writes each function out by hand.

2. Macros defined in the middle of other code

The `#define` lines for `L` and `r` sit inside the `union w{...}` declaration, between
`int _; void*p;w(*c)(w,...` and the closing `);};`. The preprocessor removes these lines
before the compiler looks at the code, so the union is really only three members long.
Each macro is also cut into short pieces with a `\` at the end of a line, which joins
the lines back together.

3. A keyword hidden behind a macro

`#define z return` means that every `z(...)` is really a `return` statement. It looks
like a function call, so it is hard to see where functions finish.

4. Short names that are reused

Nearly every name is a single letter, and they differ only by an underscore: `F`, `_F`,
`F_` and `a`, `a_`. The name `_` is used for both the `int` member of the union and the
function that forces a value. The name `r` is both a macro and a local variable in
`w*k=f.p,r=a(*k,k[1])`. This compiles because `r` is only a macro when `(` comes after
it. The name `w` is both the union tag and its typedef. `simple.c` gives each one its
own descriptive name, such as `force`, `delay` and `make_pair`.

5. One type for everything

`union w` can hold a number, a pointer or a function pointer, and every function takes
and returns this type. The compiler can no longer tell what a value really is, so the
reader has to work out by hand which member is valid at each point, like `._` for
numbers, `.p` for pointers and `.c` for code.

6. Functions built by hand as memory blocks

C has no way to make a function that remembers a value, so the program builds one. The
`r` macro allocates two `w` values with the code first and the remembered value second.
`a(f,x)` then calls the code and passes a pointer to the remembered value. Ordinary
function calls are all replaced by `a(...)`, which hides what is being called.
`simple.c` calls these closures and documents the layout above `union Word`.

7. Recursion without any function calling itself

There are no loops, and no function calls itself by name. `Y` and `W` are the
Y combinator, a trick from lambda calculus where a function is handed a copy of itself
as an argument. Repetition happens only because `a(W(f),W(f))` applies a function to
itself at run time. Reading the source shows no cycle, because the cycle only exists
in the data. `simple.c` keeps this design, since it is what the entry is about, and
names the parts `y_combinator` and `self_applier`.

8. Delayed evaluation

`a_(x,x)` looks like it applies `x` to `x`, but it only stores the two values in memory.
The real call happens later when `_(f)` runs it and frees the memory. Without this delay
the self application would continue forever. Because `a` and `a_` look nearly the same,
it is easy to miss which one runs now and which one runs later. `simple.c` calls these
`apply`, `delay` and `force`.

9. A hidden parameter passed through varargs

Each function is declared as `w _F(w x,...)` and appears to take one argument. The second
argument, the pointer to the remembered value, is passed by `a` and read back with
`va_arg`. Functions with nothing remembered, like `_Y`, ignore it. The `...` makes all
functions fit one pointer type without showing that a second argument exists. The
`argh` variable name is just a joke. `simple.c` keeps the varargs but names the value
`captures`.

10. Anonymous values created in place

`(w){._=x._-1}` and `(w[1]){{.c= &_##f}}` are compound literals, which create a value
or an array with no name right where it is used. The second one even creates a global
array inside an initializer. `simple.c` uses `make_number` for numbers and named arrays
like `y_combinator_cells` for the globals.

11. Comments that are part of the puzzle

The comments at the end of the first four lines must be read together, and spell out
`Y=\f.(\x.f(xx))(\x.f(xx))` in four pieces. The `/*=\*/` comments in the `L` and `l`
calls add `W=\x` and `Y=\f`, and `/*d */` marks the result as days. They are a hint
rather than an explanation, and they look like noise until they are joined up.
`simple.c` has this formula in one comment.

12. Disguised literals and arithmetic

The format string `"%.""f\n"` is split into two strings that the compiler joins back
into `"%.f\n"`, which means no digits after the decimal point. `simple.c` writes this as
`"%.0f\n"`. `sizeof(w)<<1` uses a bit shift in place of multiplying by two. The number
in `/60./60/24` has a trailing dot with no digits, which is what makes the division use
floating point. The conditional `x._?x._*...:1` packs the base case and the recursive
case into one expression, and `simple.c` uses an `if` statement.

13. Irregular layout

The code is packed with many statements on one line, with indentation and blank lines
that follow the shape of the text and not the structure of the program. Statements are
split at odd points, as with `w*d=f` and `.p;` on separate lines. A closing `) ; }` even
sits on its own after two blank lines.
