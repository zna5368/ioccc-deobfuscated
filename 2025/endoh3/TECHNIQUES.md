# 2025/endoh3

Difficulty: hard

1. Declarations hidden in a macro

Line 2 is `#define q FILE*F; int i,j,n,m; char*x=__FILE__,w[9<<18],*y=w,*z="--- %s\0"`
and line 3 begins with `q`. Every global variable is declared through that one
letter, so a reader looking for the declarations sees only `q/*/;qq(*/`. The
part after `q` is a C comment, so the compiler sees `q` and then the string
that comes next.

2. C and Perl in the same file (polyglot)

Perl does not run the C preprocessor, so to Perl `q/*/` and `qq(*/ ... )` are
just two quoted strings that swallow all of the C code. The last line is real
Perl (`print"Input two numbers: "...`) hidden from C inside a `/* ... */`
comment. The same file runs as a C patch tool and as a Perl "difference"
calculator.

3. Hidden program inside a string

The big string after `q` sticks to `"--- %s\0"` (C joins string literals
that sit next to each other), so `z` points to `"--- %s"`, then a NUL byte,
and then the full source of the next program. The patch tool only uses
`z` as the `sscanf` format `"--- %s"`, because `sscanf` stops at the NUL
byte. The rest is only read by the hidden second stage at `z[7]`. In
`simple.c` that source is the string `diff_engine`.

4. Encoded characters in the hidden string

The hidden string cannot hold `"`, `\` or meaningful spaces without breaking
the shape of the code, so it writes `W` for `"`, `X` for space and `Y` for
`\`. Real spaces in it are only there to draw the shape and are thrown away.
The decoder `29-(i=z++[7])/3?i:"\" \\"[i%3]` works because `W`, `X` and `Y`
are 87, 88 and 89: those are the only letters where `c/3` equals 29, and
`c%3` (0, 1, 2) picks the right real character. `simple.c` keeps the decoded
text.

5. Self-patching comment switch

Line 23 ends with `/*` and line 24 ends with `//*/`, so line 24 (the code
that reads the program's own source) is inside a comment. The last four lines
of `prog.c` are a unified diff that adds a line holding only `*/` after line 24
of `prog.c` itself. After `./patch < prog.c`, the comment closes earlier, line 24
becomes live code, `F` is no longer NULL, and the `if(F){...}` branch (which
was dead code before) runs and prints the next patch. `simple.c` uses
`#ifdef SELF_PATCHED` for this switch.

6. One buffer used for everything

`w[9<<18]` holds several things at once: the current filename at `w`, a
scratch area for the `--- name` token at `w+499`, and the patched file text
from `w+997` onward. The pointer `y` both receives each new input line and
marks the end of the text built so far. If a line should be kept, `y` moves
past it. If it should be dropped, `y` stays and the next read writes over it.
`simple.c` names these offsets and splits the steps into `read_line` and
`keep_line`.

7. Character arithmetic in place of comparisons

`j=+45-getchar()` turns the hunk line marker into a number: `-` (45) gives 0,
space (32) gives 13 and `+` (43) gives 2. The test
`!j||(m--,j==13)?G(1),--n,0:j-2` then handles all four cases in one
expression, because `j-2` is 0 (no error) only for `+`. Also `47-j?x+1:w`
means "if the character is not `/`, move on, otherwise start again" and is
used to strip directory names. `simple.c` uses `if`/`else` on `'-'`, `' '`
and `'+'`.

8. Comma operators and side effects in conditions

Nearly every statement is packed into a `for` header or a `?:` expression.
For example the whole "copy the rest of the file and write it back" step is the
left part of
`if(F?fread(y,1,9<<17,F)[y]=0,fprintf(F=freopen(w,"w",F),"%s",w+997),fclose(F):0,!x)`,
and only `!x` (end of input) decides the `if`. `G(int t)` picks its input
with `t?++i,F:stdin`, which counts file lines as a side effect. In
`simple.c` these became separate statements and the functions
`finish_file`, `set_filename` and `read_line`.

9. Reversed array indexing

The hidden stages write `j[M]`, `4[M]`, `C[i[B]]` and `8[i+M]` in place of
`M[j]`, `M[4]` and so on. `a[b]` means `*(a+b)`, so the order does not matter
to C, but it makes the code harder to read.

10. Layout as a picture and hidden comments

The code is drawn as the partial derivative symbol. In the real C code,
words are split across lines with backslash-newline (`r\` + `eturn`,
`"malformed\` + ` patch"`), and extra spaces are added inside `sscanf`
formats (`"@@ -    %d,%d +%d    ,%d"`), where a space matches any amount of
whitespace, including none. Inside the hidden string, words are broken up with
padding spaces (`cha      r`, `fo      r`) that the decoder removes. The comments `And now for something completely
DIFFERENT` and `THE DIFFERENCE ENGINE` (written with `X` for spaces) are part
of the drawing.

11. Single-letter names

`F`, `i`, `j`, `n`, `m`, `x`, `y`, `z`, `w` and `G` say nothing about their
job. In `simple.c` they are `file`, `file_line`, `old_start`, `old_count`,
`new_count`, `got_line`, `cursor`, `diff_engine`, `buffer` and `read_line`.
