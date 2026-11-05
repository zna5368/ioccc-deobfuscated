# 2012/zeitak

Difficulty: Terrifying

1. Booleans stored as functions

`n __(n _,n __){return _;}` and `n _(n _,n __){return __;}` are the only values
of true and false in the program. `__` returns its first argument and `_`
returns its second. Asking a question is therefore a function call instead of
an `if`: `x(yes,no)` gives back `yes` when `x` is `__` and `no` when `x` is `_`.
This is how the source avoids `if`, `?:`, `while` and every comparison.

2. Logic operators built from function calls

The source has no `&&`, `||`, `!` or `==`, so each is spelled as a choice
between two values. `a(b,_)` means "a and b", because it returns `b` when `a` is
true and false otherwise. `x(_,__)` means "not x", because the two answers are
swapped. `a(D,D(_,__))` means "a equals D": it returns `D` when `a` is true and
"not D" when `a` is false. The code that compares two bracket types is two of
these equality tests joined by an "and".

3. Recursion instead of loops

There is no loop keyword. `f` reads one character and calls itself to read the
next one, so a file of 1000 characters needs about 1000 nested calls. The last
line, `return q(_,v)(r(g(__,a),g(__,b)),r);`, does this: `r` holds `f` when the
program should continue, and holds `_` or `__` when it should stop. Calling
`r` either runs `f` again or just picks an argument, and the code looks the same
either way.

4. Functions chosen by a condition

`u(_,i(f,_))(D2,D3)` looks like one call but is a decision about what to call.
The first part evaluates to either `f` or `_`, and the second part calls
whichever one was chosen. If it is `f`, the program recurses to handle a new
opening bracket. If it is `_`, nothing happens and an argument is returned. This
is why `f` may call itself once, twice or not at all.

5. Variables kept in a tree of pairs

`w=C(C(h,h),C(h,h))` builds a small binary tree whose eight leaves are the
global flags, such as `_((t)_(_(w)))` for "inside a string" and
`_(_((t)_(w)))` for "which quote opened it". Instead of a name, each flag has
a path through the tree, so the only way to tell two flags apart is to count
parentheses and casts in a long expression.

6. Two structs that select a field by cast

`struct s` and `struct t` have the same layout, but the field named `a` is first
in `s` and second in `t`. The macro `_(x)` is `((*x).a)`, so `_(x)` reads the
first field and `_((t)x)` reads the second. Every "second field" read is hidden
as a cast, and the field called `UNUSED` is in fact used.

7. Character data packed into a table of pointers

`l[]` describes each character with four true or false flags. The globals `a`,
`b`, `c` and `d` hold the four combinations of two booleans, and each table entry
is two pointers to them, which makes four flags. `(`, for example, is `&b,&d`.
The flags say whether the character is a bracket, whether it closes or is a
quote, and which type it is. No character constant such as `'('` appears: the
meaning comes only from the position in the table.

8. Table written with repeating macros

`w(_)` writes its argument twice and `z(_)` three times, so nesting them acts as
multiplication. `w(w(w(w(w(w(&d))))))` writes `&d` 64 times. This runs the table
out to the right positions without digits. The table also has no inner braces,
so C fills the pairs one pointer at a time, which makes the entry count half of
what it appears to be.

9. A table with more than one job

`l[fread(&k,s(e),s(e),stdin)]` indexes the table with the return value of
`fread`, which is 1 for a character read and 0 at the end of the file. The rows
for bytes 0 and 1 are reused as a true or false end-of-file flag. `e k2=s(l)`
also uses the size of the table, cut down to one byte, as the starting "previous
character".

10. Numbers written without digits

`s(e)` is `sizeof(unsigned char)`, which is always 1.
`(e)s(s[s(s[s(s[s(s)])])])` nests `sizeof` around array types until the size is
a multiple of 256, and then the cast to one byte leaves 0. Those two values are
the "write one character" and "write nothing" counts given to `fwrite`.

11. A counter made from two flags

The program stops after a few characters of error context, but no `+` is
available. Two flags in `w` form a two-bit binary number instead. One flag is
flipped on each step, and the other is flipped only when the first one wraps
around from true to false, which is how a person counts in binary.

12. Reused names

`_` is a function, a macro, a parameter and a local variable. Function-like
macros only expand when followed by `(`, so `_(x)` and `C(_,__)` mean very
different things on the same line. `t` is a typedef, while `t()` is a macro for
`(p)`. `s` is a macro and a type, `w` is a macro and a variable, and `C` is a
global function in `main` but a local flag inside `f`. Inner blocks also declare
new `a`, `b` and `u` that hide the outer ones.

13. Variables that change meaning

`i` starts as "this is a bracket outside a string" and is later overwritten with
"this is an opening bracket". `g` means "bad closing bracket" and then becomes
"keep printing". `v`, `q` and `r` are reused in the same way, and `a` gets a
new job through an assignment inside a function argument. The same name means
something different depending on the line.

14. Work hidden in function arguments

Side effects are placed where they are easy to miss. In
`fwrite((k2=k,&k),...)` the comma operator updates the previous character just
before passing the pointer `fwrite` needs. In `v(f,(_(_(_(w)))=(s*)m,_))` an
assignment that restores a global flag is hidden inside an argument, with `_` as
the value that is really passed.

15. Values passed through the wrong types

`p` is a function pointer type that returns a function pointer four levels deep,
so that results can be called again without a cast. The same `void *` arguments
carry booleans, functions, characters and strings: `C(k2,k)` chooses between two
characters and `f(_,_)("OK\n","\n ^ Error\n")` chooses between two strings. The
exit code is simply the return value of `printf`, the length of the chosen
message.

16. Code that does nothing

Several parts only exist to mislead the reader. The expression `k=k` in the first
`fwrite` has no effect. The quote type is written twice, and the second write
never changes anything. The empty string `""` after the error message is joined
on by the compiler and adds nothing.

17. Layout that hides the structure

The source is drawn as giant parentheses and is only lined up with tab width 4.
Statements are cut at arbitrary points to fit the shape, so a line break says
nothing about where one statement ends and the next begins.
