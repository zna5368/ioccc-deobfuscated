# 1993/schnitzi

Difficulty: Medium

1. Look-alike variable names

Every variable is spelled with only the letters `i` and `l` and the digit
`1`: `l`, `l1`, `li`, `ll`, `lll`, `lli`, `l11`, `l1l`, `ll1`, `il`, `il1`,
`i1l`, `li1`, and the function `ii`. In most fonts these look almost
identical, so `l1[lli]`, `li1[l1l]` and `il1` are very hard to tell apart.
Even `main`'s parameters are named `ll` (argc) and `lll` (argv).

2. Visual layout as a decoy

The code is laid out in two columns of short tokens with lots of space
between them, and every line starts with either `(` or `)` in column one.
This hides where statements begin and end. For example a call like
`puts(" Beats me.")` is split so that `puts` ends one line and
`(" Beats me.")` starts the next one far away.

3. Hidden punctuation string

The function `ii` holds `char *l="\"; !l ? 1.1 : l; l,printf(";`, which
looks like a piece of C code inside a string. Only every third character
starting at index 3 is ever read: `l[3*l1+3]` gives `!`, `?`, `.`, `:`,
`;`, `,`. `ii(word, 3)` asks "does the word end with `!`, `?` or `.`?"
(end of a sentence) and `ii(word, 6)` also includes `:`, `;` and `,`.

4. Fake comment and fake subtraction

`int l1=1-/*+&|");  &+|-*/1,li=!1;` contains `");` in a comment, so it looks
like the string above continues. After removing the comment it is just
`int l1 = 1 - 1, li = 0;`, so the loop counter starts at 0, not 1.

5. Constants written as expressions

`!1` is used for `0` throughout (`lli=l=ll1=il=!1`, `lll[!1]`, `li1[!1]`),
and the string terminator is written as `=!1`. `fscanf(...)+1` is used as a
loop condition, which is false only when `fscanf` returns `EOF` (-1).
`if(ll-2)` means `if (argc != 2)`.

6. Macro that hides an `if` statement

`#define iii(I) if(ii(I,6))I[strlen(I)-1]=!1` looks like a function call
`iii(word)` but expands to a full `if` statement that cuts off one trailing
punctuation mark. A second macro redefines `gets(x)` as
`fgets((x),111,stdin)`, so the code appears to use the unsafe `gets`.

7. Negative indexing of argv

The loop over the question words uses a counter that goes down from 0
(`while(l1l>-ll)`) and reads `lll[-l1l--]`. Negating a negative counter
gives the indexes 0, 1, 2, ... so it is just a normal walk over every
`argv` entry, including the program name and the file name.

8. Short-circuit operators instead of if

`strcasecmp(...)||(l11=++il)` only runs the assignment when the strings are
equal (strcasecmp returned 0). `l11||lli++` only moves to the next word
slot when the word did not match the question. Both are `if` statements in
disguise.

9. Recursion through system()

There is no loop that asks for questions. Instead, `main` prints a prompt,
reads a line and calls `system("argv[0] file question")`, which runs a new
copy of the same program with the question words as extra arguments. The
shell splits the question into words for free. When there are exactly two
arguments the program just prints the file with `system("cat file;echo")`.

10. Reused variables

The same variables do several jobs. `il1` holds the `cat` command, then the
question line. `l1l` is the argv counter, then the copy counter, then the
print counter (reset with `l1l=!1` inside an index expression:
`*li1[l1l=!1]=toupper(*li1[!1])`). `li` is a return value in `ii` and an
"end of sentence" flag in `main`.

11. Old K&R style function definitions

`int ii(ll,lll) char*ll; {` and `main(ll,lll) char*lll[]; {` declare the
parameter types after the parameter list, and `lll` in `ii` and `ll` in
`main` have no type at all, so they default to `int`.
