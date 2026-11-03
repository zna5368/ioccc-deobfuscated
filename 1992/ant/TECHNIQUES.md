# 1992/ant

Difficulty: Medium

1. Single-letter names

Every function and variable is one letter. `M()` returns a file's modification
time, `T()` finds the rule for a target, `U()` updates a target, `t[]`, `d[]`
and `c[]` hold the targets, prerequisites and commands of each rule, `b` is the
buffer holding the whole makefile, `l` is the input line and `w` is the string
of whitespace characters. Inside `main()` the letters `i`, `j`, `k`, `m`, `n`,
`p`, `q` and `r` mean different things in different places, so the reader has to
work out what each one holds from how it is used.

2. Macros that hide keywords and library calls

`#define W while`, `#define R return`, `#define C continue;`, `#define S strspn`
and `#define F fprintf(stderr,` make the code shorter and less familiar. The
`F` macro even contains an unmatched `(`, so a line like
`F "am: ...\n", f);` looks like a broken statement until the macro is expanded
to `fprintf(stderr, "am: ...\n", f);`.

3. A tokenizer hidden in a macro with side effects

`#define N(p, i) (p += i, p += S(p, w), i = strcspn(p, w))` uses the comma
operator to do three things at once: skip past the current word of length `i`,
skip the whitespace after it, and store the length of the next word back into
`i`. Because it changes both of its arguments, `N(f, i)` quietly moves the
pointer `f` to the first word. It is used for very different jobs: getting the
first word of a name in `M()` and `T()`, walking the prerequisite list in
`U()`, trimming spaces around `=` in macro definitions, and the loop condition
`W (N(q, j))` that walks a target list. The expression `q = N(m, j) + m` relies
on `m` already being changed by the macro when it is added.

4. Words are never copied or terminated

Names are not copied into their own strings. Functions are given a pointer into
the middle of the buffer and only look at the first word. `M()` temporarily
writes a `'\0'` after the word, calls `stat()`, then puts the original
character back. `T()` compares words with `strncmp()` and a length. This makes
it hard to see which part of the string a function really uses.

5. One buffer and three parallel arrays instead of structures

The makefile is rewritten into the single buffer `b` with macros expanded,
continuation lines joined, and newlines and `:` replaced by `'\0'`. A rule is
then only three pointers into that buffer: `t[i]` (text before `:`), `d[i]`
(text after `:`) and `c[i]` (the line after the rule line). The commands of a
rule are tab-started lines packed one after another as `"\tcmd\0\tcmd\0"`, and
the loop `W (*q++ == '\t')` keeps going only while the next line starts with a
tab. Nothing in the code says that this is the layout.

6. Values that mean more than one thing

`c[k]` is the command list, but it is set to `0` once the commands have run so
that a target is never made twice. Rule `0` is a fake empty rule
(`*t = *d = *c = ""`) that `T()` returns for names with no rule, so `U()` never
has to check for a missing rule. In `U()`, `j` first means "the target is out of
date" (`j = !t`) and later means "stop on errors", and `i` is first a word
length and later means "echo the command".

7. Dense parsing loop with hidden state

`main()` parses the makefile in a single character loop. The condition
`W (!(k = *r == '\\' && r[1] == '\n') && *r)` both stops at a backslash-newline
and remembers in `k` that the next line is a continuation. Whether the previous
line was a rule (`t[i]` is set) and whether a macro definition is waiting to be
put into the environment (`m`) are only handled when the next line starts.
`if (*r == r[1]) ++r;` inside the `$` case is the code that turns `$$` into `$`.
The `C` (continue) macro is used both for the outer line loop and the inner
character loop, which makes it hard to see which loop it continues.

8. Old style (K&R) function definitions

Functions are written like `time_t M(f) char *f; { ... }` with the parameter
types listed after the parameter list, and `sleep()` is called without being
declared. This old style is harder to read for programmers used to modern C.
