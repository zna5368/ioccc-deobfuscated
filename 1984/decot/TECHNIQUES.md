# 1984/decot

Difficulty: Medium

1. Redefining keywords and type names with macros

`#define double(a,b) int`, `#define char k['a']` and `#define union static struct`
make familiar words mean something else. `double (x1, y1) b;` actually declares
`int b;`, `char x {...}` becomes `k['a'] = {...}` (an array of 97 ints because
`'a'` is 97), and `union tag u` is really `static struct tag u`. A reader who
trusts the keywords will completely misread the declarations.

2. Operator hidden in a macro

`#define x =` turns the letter `x` into an assignment. So `*k *= * __FILE__ *i`
multiplies `k[0]` by the first letter of the file name, and `union tag u x{4}`
is `static struct tag u = {4}`. Single letters normally look like variable names,
so the hidden `=` is easy to miss.

3. String literals that look like code

`"'\",x);	/*\n\\"` contains a quote, a comma, `x);`, and the start of a comment
`/*`. Because the `\"` is escaped, the string does not end where it seems to, and
everything up to the final `"` is plain text that gets printed. This is the only
output of the program.

4. Comments that look like code (and code that looks like comments)

`/*/*\*/` is one complete comment, not a nested one. The last comment starts at
`/*}` and runs all the way to `*/`, swallowing what looks like the closing `;`
and braces. Meanwhile `/ *    ((main) ...` looks like the start of a comment but
the space makes it a division followed by a pointer dereference.

5. Misleading layout and brace placement

The braces and indentation suggest a `for` loop containing a `do` block, a
separate `while` loop, and a separate block. In reality `do { ... } while(b = 3, i);`
is the body of the `for` loop, and the `{ k['a'] = b,i; _0: ... }` block that
looks like the `while` body comes after the loop. The trailing `}}}` closes
blocks that were opened much earlier.

6. goto into another block and unused labels

`goto _0;` leaves the loop and jumps into the middle of the following block, past
`k['a'] = b,i;`. The label `_O` (letter O) looks just like `_0` (zero) but is
never jumped to, so the `while` loop after it is dead code.

7. Short-circuit evaluation hiding dead code

At `_0:` the condition is `b && ...`. The global `b` is still 0, so `&&` stops
right away and the scary expression with `sin`, the recursive `main` call and
the `struct tag` pointer cast is never run.

8. Unusual and misleading names and types

Local variables are named `signal` and `dup` (names of standard library
functions), the struct members are `x0` and `xO` (zero vs. letter O), `main` is
declared to return a pointer (`*main()`), and `floor` is called through a cast
to `int (*)()` with an `int` argument. Its result is passed to `printf` as an
extra argument that the format string never uses.

9. Using `__FILE__` as data

`*__FILE__` is the first character of the source file's name (`'d'` for
`decot.c`). It is only used to make the loop condition non-zero, but it makes
the program's behaviour seem to depend on its own file name.
