# 2012/hou

Difficulty: Insane

1. ASCII art built from useless keywords

`#define long` and `#define do` define these keywords as nothing, so the
preprocessor deletes them. They are scattered through the code, as in
`H=a[p ]do.h` and `2]long=`, only to fill space in the picture. The art had to
be drawn with keywords, strings and character constants so that it would look
good with syntax highlighting.

2. Macros that rename keywords

`If` means `char`, `IF` means `for`, `R` means `return`, `fi` means `"%s"` and
`FI` means `"r"`. `If` and `IF` look like `if` but have nothing to do with it,
so the reader's first guess about the code is wrong.

3. Numbers hidden in strings

`*"\2void"` reads the first character of the string, which is the number 2. The
text after it is only decoration. The same trick gives `*"\1do"` (1),
`*"\0while"` (0), `*"\btypedef"` (8), `*"\nshort"` (the newline) and
`*"-for"` (`-`). `"signed"[6]` reads the hidden NUL at the end of the string and
`(4+5)["volatile*"]` does the same with the array and index swapped, because
`a[b]` and `b[a]` are the same in C.

4. Numbers written in strange ways

`70-94+'@'` is 40, which is `'('`. `124` is `'|'`, `46` is `'.'`, and `'\14'` is
the octal way to write 12. `'\10'` is 8, so `<<'\10'` is a shift by 8.
`'('*32` is 1280 and `'@'*4` is 256. `0 * !"\0auto"` is always 0, and `s=s` does
nothing at all.

5. Two values packed in one integer

The `c` field of an instruction holds a jump target in its upper bits and a
character in its lowest 8 bits. `W(O+1<<8,J)` stores a target and `a[p].c>>8`
reads it back. The character is read by `e=a[p].c`, which works because `e` is
a `char`, so the assignment silently throws away the upper bits. A pure jump
therefore has character 0, and a character of 0 means "match anything".

6. Return values that are inverted with bitwise NOT

The interpreter runs `p=~a[p].f(p)`, so the sign of `p` says what happens next.
If a handler returns a positive number, `p` becomes negative: the thread stops
and `~p` is where it continues on the next character. If a handler returns a
negative number, `p` becomes positive and the thread continues right now. That
is why `J` returns `~a[p].c>>8`, and why `S` returns `~p-'\1'`, which comes out
as `p+1` once it is inverted. Returning 0 gives `p=-1`, which queues a thread at
address 0, and address 0 is never executed.

7. A compiler that runs on the machine it compiles for

There is no parser for the regular expressions. The first 12 entries of table
`b` are a program for the virtual machine: each entry says "if the character is
`*` call `Z`, otherwise fall through to the next entry". The function `V` runs
this table over the syntax file, and it then runs again over the input with the
program that the first run generated. In `simple.c` this table is a `switch`.

8. Program and compiler in one array

`r=b+'\14'` makes the output program start in the middle of the same array as
the compiler program. The entries after index 12 in `b` are not more compiler
code but the fixed start of the generated program, and `a` switches between the
two with `a=b` and `a=r`.

9. Text messages that are working code

`"Make a contract with me"` looks like decoration for the art, but it is the
`t` field of the first entry of the generated program. The loop in `V` tests
`*a->t`, so without a non-empty string there the matching loop never starts. The
`"-"` in the first entry of `b` does the same job for the compiler program.

10. Operators built from other operators

`?` is `Z` (the code for `*`) followed by `r[O-3].c+=256`, which adds 1 to a
jump target so that the loop no longer repeats. `|` also calls `Z`, and saves
the address of the loop's jump in `c[g]` so that `D` can overwrite it later with
the end of the group. Character classes have no code of their own for matching:
`[abc]` is expanded into `a|b|c`, and `[^abc]` into tests that jump to address
0 for each listed character. One instruction layout has several meanings, so
it cannot be understood by reading a single function.

11. Placeholder instructions that are rewritten later

`C()` emits a jump to the next instruction, which does nothing. It exists only
so that an operator that comes later, like `*`, can replace it through
`r[A].c=...`. `Y` does the same with `r[x]`, turning it into the split that
links one rule to the next. You cannot understand the generated program by
reading the code that emits it in order, because old instructions change.

12. Flat arrays instead of structures

`c` is a stack in which each group takes two slots, so `g` moves by 2 and
`c[g]` and `c[g-1]` are two different fields. `_` holds match records as three
pointers in a row (format, start, end). They are read through negative indexes
hidden in strings, such as `l[-*"\2$"]` for the start. `I` holds two thread
lists at once: `I[N++]` writes the list for the next character over entries of
the current list that have already been read.

13. Shadowed names

`V` is a function, but in `E` it is a local variable. In `j`, `r` and `D` hide
a global and a function. In `main`, `B` and `j` hide functions too, and the
argument list `j` is later reused to walk through the match records. Macro `X`
and function `Z` declare a local `y` that hides the global rule counter. Macro
`X(B)` saves `A`, runs the code passed to it, and restores `A`, which looks like
a function call but wraps whole loops.

14. Global variables with several jobs

`s` is the read position in the syntax file and then in the input, and is also
the temporary pointer in macro `Q`. `e` is a character to compare in `V`, a
loop counter in `E` and a saved character in `Q`. `H` is the start of the
current match and later the printing position. All functions pass data through
one-letter globals, so a call does not show what it reads or changes.

15. Conditions written as expressions

There are few `if` statements. `F` is one `return` that uses `?:` and commas to
emit three instructions. `j` pushes a record with
`*l++=r,*l++=H,*l++=s,l` inside a ternary, and `a[p].h&&a[p].h-H>0&&(H=a[p].h)`
is an `if` written with `&&`. Pointers are compared by subtracting them, as in
`a[p].t-s` and `r-l[-3]<=0`, which hides what is being compared.

16. Printing a part of a string

Macro `Q` puts a NUL in the middle of the text to end the string early, prints,
and then restores the character. It always passes the text three times, as in
`printf(q,b,b,b)`, because a rule's format may contain `%s` up to three times.
The extra arguments are ignored when the format has fewer.

17. Old style C

Functions like `k(p){...}` have no return type and no parameter type, and the
function pointer `int (*f)()` takes unknown arguments. Some handlers use the
parameter and some ignore it, and `C` and `V` take none, so the declarations
give no hint about how a function is used.
