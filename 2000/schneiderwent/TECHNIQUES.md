# 2000/schneiderwent

Difficulty: Medium

1. `setjmp()`/`longjmp()` used as function calls

There are no functions besides `main()`. The macro `C(f, m)` acts as a "call":
it stores an argument in the global `mNb`, saves the current position with
`setjmp(sT[sI++])` onto a hand-made stack of 256 `jmp_buf`s and then does
`goto f`. The macro `bck` acts as a "return": `longjmp(sT[--sI], 0)` jumps
back to the most recent saved position. Because `longjmp()` with a value of 0
makes `setjmp()` return 1, the `if (!s) goto f;` is skipped the second time
and the code continues after the "call". The "functions" `doMin`, `doHr`,
`doIt`, `out` and `mA` are just labels. simple.c turns them into real
functions (`print_phrases()`, `print_message()`, `decode_messages()`).

2. Code hidden inside an `else` that is never entered

All the labelled "functions" live inside the `else` branch of
`if (info != NULL)`. That branch is only entered normally when no data file
was given (it prints the time and jumps to `eoj`). The rest of the time the
code is reached only through `goto`, so the program looks like it skips that
block when it actually runs most of its work there.

3. Nested `setjmp()` to sequence the minutes and hours

At the end of the data file branch, two more `setjmp()` calls are stacked in
`if (s) { s = setjmp(...); if (s); else goto doHr; } else goto doMin;`. The
first time through it jumps to `doMin`, when that returns it jumps to `doHr`,
and when that returns the empty `if (s);` falls out to `exit(0)`. This is just
"print the minutes, print the hours, quit".

4. Messages stored as EBCDIC numbers in a data file

All of the words ("midnight ", "a quarter ", "'til " ...) are stored in
schneiderwent.data as EBCDIC character codes, an old IBM character set where
for example `a` is 129 and a space is 64. Nothing readable appears in the
source. simple.c lists the decoded messages in a comment.

5. EBCDIC to ASCII conversion with a chain of `?:` operators

The `mA` label converts each message with one long expression:
`129<=m I ? 137>=m I ? m K=m I-32 : 145<=m I ? ... : j : j : j : ...`. The
letters a-z come in three blocks in EBCDIC, so each block is checked and moved
by a different amount (-32, -39, -47). The lone `j` values are "do nothing"
results for when a test fails. The macros `I` and `K` hide the array indexes
`[i][j]` and `[i-1][j]`, so `m I` means `m[i][j]` (the EBCDIC row) and `m K`
means `m[i-1][j]` (the ASCII row just above it). simple.c writes this as an
`ebcdic_to_ascii()` function with plain `if` statements.

6. Character set detection with `'J' % 2`

`c1 = 'J' % 2` is 0 when the compiler uses ASCII (J is 74) and 1 when it uses
EBCDIC (J is 209). Every message is a pair of rows, the ASCII row followed by
the EBCDIC row, so adding `c1` to a message number picks the row that will
print correctly on the current machine. This is `native_row` in simple.c.

7. Bit masks and table lookups choose the words

The current minute (0-59) is used as an index into `m5` to get a group number,
and that group number is used as an index into `m1` to get a bit mask. Each
bit that is set in the mask means "print this message". For minutes the bit
number is doubled to get the message (`q = q << 1` before the call and
`q = q >> 1` after), for hours the bit number is used directly and the hour
looks in `m5` starting at index 61. The same `doIt` loop handles both cases
by checking `if (z == 22)`, where `z` is the number of bits to test (22 for
minutes, 16 for hours).

8. Trigraphs

Every square bracket is written as the trigraph `??(` or `??)`, for example
`m5??(84??)` is `m5[84]`. The compiler needs the `-trigraphs` flag to turn
these back into `[` and `]`.

9. Short, unhelpful names and macro shortcuts

Names like `m`, `m1`, `m5`, `sT`, `sI`, `q`, `r`, `z` and `c1` give no hint of
their meaning, and the macros `P` (for `printf(`, with no closing parenthesis)
and `S` (for `strtok(NULL, ",\n\0")`) make lines hard to read. simple.c uses
names like `slot_to_phrase_set`, `phrase_set`, `messages` and `native_row`.

10. Global state shared between "functions"

Because the fake functions cannot take arguments, all data is passed through
globals: `mNb` is the message number, `q` is the bit counter, `r` is the bit
mask being tested and `z` is the loop limit. `doHr` has to reset `q` and `r`
itself before reusing `doIt`, and it reuses the local `n` (the minutes) to
hold the hour's table index.
