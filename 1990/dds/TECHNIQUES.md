# 1990/dds

Difficulty: Hard

1. Macro that writes whole functions

`O(b,f,u,s,c,a)` expands into a complete function named `b`. It calls the next
lower function `f` for the left side, then looks at the next character: if it
is `u` it applies the C operator `s`, if it is `c` it applies operator `a`, and
otherwise it puts the character back. Five one-line calls such as
`O(V,W,'+',+,'-',-)` create the whole expression parser (`S`, `J`, `K`, `V`,
`W`), so the parser is never written out anywhere. Passing a bare operator like
`<=` as a macro argument is legal because the preprocessor only pastes text.
`simple.c` writes the five functions out by hand as `expression`,
`less_greater`, `less_greater_equal`, `sum` and `product`.

2. Macros that hide keywords and punctuation

`X` stands for `;break;case`, `_` for `return`, and `R` for `999`. Every
`X'R':` in `main` is really the end of one `case` and the start of the next,
which hides the shape of the `switch` statements. `N` is the start of a `for`
loop over every program line followed by `&&`, so `N printf(I)` means "for each
line, if it exists, print it", with `I` holding the format string and its
arguments. `t(e,d,_,C)` builds a whole `case` that opens a file, runs `C` and
closes it; it is used for both SAVE and OLD. `U(y,z)` is a `while` loop that
replaces a two character operator. `simple.c` uses ordinary `case`, `break`,
`return` and loops, and the helper functions `list` and `replace_operator`.

3. One letter names and shared global variables

All variables are single letters and almost all of them are global: `m` is the
program, `P` the BASIC variables, `C`/`E` the GOSUB stack, `L`/`M` the FOR loop
start lines and limits, `B` the line buffer, `p` the parser position and `l`
the current line number. Even the temporary pointers `x`, `y`, `z` used inside
`Q` are global. The same buffer `B` holds the typed command, the statement
being run and the text typed for INPUT. `simple.c` gives every variable a
descriptive name and makes temporaries local.

4. Hand written library functions

`Q(s,o)` is a home made `strstr`: it returns where the text `o` first appears
inside `s`. It is used to find `THEN`, `TO`, the newline at the end of a line
and even to test if a character is a blank, by putting the character in the
one letter string `F` and calling `Q(" \t",F)`. `simple.c` names it `find`.

5. Expressions instead of statements

Most of the interpreter is written with `&&`, `?:` and the comma operator
instead of `if` and `;`. For example `B[2]!='M'&&(l=*--C)` means "if this is
not REM, pop the GOSUB stack", and the IF statement is
`*(q=Q(B,"TH"))=0,p=B+2,S()&&(p=q+4,l=S()-1)`. Several are wrapped as
`(free(m[i]),0)` only so that a `void` function can sit inside `&&`.
`simple.c` turns them into `if`/`else` blocks.

6. Keywords recognised by single letters

A statement is identified only by a few of its characters. A `=` in the
second position means assignment, otherwise the first letter picks the
statement: `R` with a third letter `M` is REM and anything else is RETURN, `I`
followed by `N` is INPUT and otherwise IF, `G` with `S` at position 2 is GOSUB
and otherwise GOTO. Fixed offsets such as `B+5` skip the keyword, which works
because all blanks were removed from the line first. The immediate commands
work the same way (`R`un, `L`ist, `N`ew, `B`ye, `S`ave, `O`ld). `simple.c`
comments every case with the full keyword.

7. Two character operators rewritten into one character

Before running a line that has no string in it, `<>`, `<=` and `>=` are
replaced inside the stored program by `#`, `$` and `!` followed by a blank, so
the parser only ever needs to look at one character per operator. Because the
stored text is changed, a LIST after RUN shows these strange characters.
`simple.c` keeps this behaviour and explains it in comments.

8. Pointer to the last character

While a line is copied into `B` without blanks, `d` is left pointing at its
last character. That one pointer is used three ways: as the variable name of
`NEXT I` and `INPUT U`, and as the closing quote of `PRINT "..."` which is
overwritten with `'\0'`. For INPUT the variable letter is read through `d`
after the user's input has already overwritten `B`, so it only survives when
the typed text is short. `simple.c` passes this pointer as `last`.

9. Hidden END line and an always true loop

`m[11*R]="E"` stores a secret line 10989 that contains only `E`. RUN skips
over empty line numbers until it finds a line, so a program that runs off its
end reaches this line and stops as if it hit END. LIST and SAVE only go up to
`11*R`, so the line is never shown. The main loop is written as
`while(puts("Ok"),gets(B),B[strlen(B)-1]='\0',1)`, a comma expression whose
final `1` makes it loop forever while the other parts print the prompt and
read a line. `gets` itself is a macro that turns into `fgets`.

10. Right recursive parser

Each level of the parser calls itself for the right side of an operator, so
operators group from the right: `8-4-2` is `8-(4-2)` which gives 6. Variables
are stored in an array indexed by the character code of their letter
(`P[*p++]`), and numbers are read with `strtol(p,&p,0)`, which also accepts
octal and hexadecimal.
