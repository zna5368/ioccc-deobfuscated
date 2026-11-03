# 1995/dodsond1

Difficulty: Medium

1. Code shaped as ASCII art with filler macros

The source is drawn as a picture made of `X` characters. Lines such as
`#define X` and `#define XXX` turn every run of `X`s into nothing, so the
compiler skips them and only sees the C code woven between them. The
`#define orfa for` line is slipped in among the `X` macros so it is easy to
miss.

2. Pig Latin identifiers and macro-renamed keywords

All the names are in Pig Latin: `ainma` is `main`, `archa` is `char`, `orfa`
is `for`, `etcharga` is `getchar` and `utcharpa` is `putchar`. `#define` lines
turn them back into the real names. The variables follow the same pattern:
`atinla` ("latin") is the current character, `igpa` ("pig") is a pointer into
the `oink` buffer, `apca` ("cap") is `'A'`, `owla` ("low") is `'a'`, `umna`
("num") is 26 and `iocccwa` holds whether the word starts with a capital.

3. Character classes hidden in multiplication

Instead of `c >= 'A' && c <= 'Z'`, the program tests
`(atinla-apca)*(apca+umna-atinla)>=0`. The product is zero or positive only
when `c` lies between `'A'` and `'A'+26`. That range ends at `'['`, not
`'Z'`, so `[` and `{` also count as letters. Adding two of these tests
together (`upper + lower`) gives a "this is a letter" test. In simple.c these
became the small functions `is_upper`, `is_lower` and `is_letter`, which keep
the extra character.

4. Vowel lookup in a disguised string

The string `"-Pig-" "Lat-in" "COb-fus" "ca-tion!!"` looks like a message,
but the compiler joins the pieces into one 27-character table indexed by
`letter - 'a'`. The `-` signs sit at positions 0, 4, 8, 14 and 20, which
are a, e, i, o and u. Checking `table[...] - '-'` is non-zero only for
consonants. simple.c compares against the vowels directly in `is_consonant`.

5. Whole program inside `for` loop headers

Every statement is packed into the parentheses of one of six `for` loops,
and each loop has an empty body. Work is done in the condition and
increment parts using the comma operator, such as
`utcharpa(atinla),atinla=etcharga()`. simple.c turns these into ordinary
`while` loops with the work in their bodies.

6. Side effects in short-circuit conditions

The consonant loop's condition also builds the suffix:
`((igpa==oink)&&!(*(igpa++)='w'))||!(*(igpa++)=owla)`. When the consonant
test fails, `||` moves on. If no consonants were stored, `'w'` is written,
and `!` makes that part false. The last part then writes `'a'`, which is
also false, so the loop ends with `"wa"` or `"a"` appended. In simple.c
this is a plain `if` followed by an assignment.

7. Comparing to EOF without naming it

`atinla` is a `char`, and the loops test `atinla+1` instead of
`c != EOF`. `getchar()` returns -1 at the end of input, so `atinla+1` is
zero only there. Because the variable is a `char`, a byte of `0xff` in the
input also reads as -1 and stops the program early. simple.c keeps the
`char` type and tests `c != -1`.

8. Repeated inline expressions instead of helpers

The upper and lower case range tests and the case conversions
`atinla-apca+owla` and `atinla-owla+apca` are copied out in full a dozen
times. This makes the loops very long and hard to match up. simple.c gives
each one a name (`to_lower`, `to_upper`) and writes it once.
