# 2005/mikeash

Difficulty: Medium

1. One file that is both a C program and a Lisp program

The C compiler and the Lisp interpreter read mikeash.c differently. Almost
every line starts with `;`, which ends a statement in C but starts a comment in
Lisp. So the C code is a Lisp comment, and only lines that start with `(` or
`)` are Lisp code: `(defvar q=` and the long string after it,
`(format t ;...`, and `(substitute #\nnewline (char-upcase #\y ) q= )q= );*/`.
That last line is hidden from C inside the comment that starts with `/*` at the
end of the line above. Running a code formatter on the file moves the line
breaks and breaks the Lisp program. simple.c is plain C and must be given
mikeash.c as input to print the self-reproducing output.

2. Typedefs named after Lisp words

`typedef m defvar;` and `typedef defvar*format;` create C types called
`defvar` and `format` only so that the Lisp words can appear in valid C. In
`for(defvar q="...";C!='\n';A());` the C part declares an unused `char *`
variable named `q` while the Lisp part reads `(defvar q= "..."`. In
`for(format t ;*a;a++)` an unused variable `t` is declared. These variables
do nothing in C, so simple.c removes them.

3. A copy of the program stored in an unused string

The string given to `q` is the whole source with every line break replaced by
the letter `Y`. C never uses it. The Lisp program evaluates
`(format t (substitute #\newline (char-upcase #\y) q) q)`: it turns every `Y`
back into a newline to get the format text and puts the quoted string itself
where `~s` is. This is the classic quine trick of printing a template with
itself filled in. The uppercase `Y` is built from `#\y` with `char-upcase`
because a literal `Y` inside the Lisp line would also be replaced.

4. Short and meaningless names

The types `n` and `m` are `char` and `char *`. The functions `A`, `B`, `R` and
`E` read a character, get a string length, read a token and evaluate an
expression. The buffers `b`, `w` and `c` hold the current token, the variable
`q` and small results. simple.c calls them `next_char`, `length`,
`read_token`, `eval`, `token`, `variable_q` and `scratch`.

5. Characters written as numbers

`34` is the double quote `"`, `126` is `~`, `37` is `%`, and
`n y[]={37,100,0}` is the string `"%d"`. `32` is the distance between a
lowercase and uppercase letter, used for `char-upcase`. `while(C+1)` means
"while `C` is not -1 (EOF)".

6. A chain of conditional operators

`z-'+'?z-'-'?z-'*'?d/e:d*e:d-e:d+e` chooses the arithmetic operation. Each
`z-'+'` is non-zero (true) when `z` is not `+`, so the expression is read
from the right: `+` gives `d+e`, `-` gives `d-e`, `*` gives `d*e` and any
other letter gives `d/e`. simple.c uses an `if`/`else` chain.

7. Everything in one function with shared buffers

`E` handles every built-in function in one long `if`/`else` chain, chosen only
by the first letter of the name. Many results point into the same global
buffers, so a value returned by `E` is changed by the next call. `format` does
not even return; it falls through to `return a`, which is the token buffer
holding `)`. `format` also writes a `"` after the end of its argument to make
`~s` look like a quoted string. simple.c gives each built-in its own function
and comments these effects.

8. No header files

The program has no `#include` lines, so every library function such as
`getchar`, `strdup` and `printf` is implicitly declared. The C99 `for` loop
declarations and missing headers produce the warnings the author mentions.
simple.c keeps this so it still has the same headers as the original.
