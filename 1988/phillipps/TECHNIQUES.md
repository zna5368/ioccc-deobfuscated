# 1988/phillipps

Difficulty: Hard

1. One recursive function does everything

`main(t,_,a)` (renamed `pain` in `phillipps.c`) is the only function, and it
calls itself for every job: printing a verse header, looping over the days,
looping over the gifts, finding a string in the table, decoding a character and
printing it. The first argument `t` picks the job. Values above 1 drive the
verses, `0` prints one string, `-1` to `-50` skip ahead to the wanted string,
`-51` to `-72` decode one character, and below `-72` restart the search at
the start of the string table. Each loop is written as recursion with a
changed argument, such as `main(t+1,_,a)` or `main(-65,_,a+1)`.

2. Arguments reused for unrelated purposes

`main` is started by the system with `t` as `argc`, which is `1` when the
program runs without arguments. That value starts the song. The second
argument `_` is a day counter in one call, a string number in another and
the character to decode in a third. The third argument `a` is sometimes the
string table, sometimes the decoding alphabet and sometimes a dummy.

3. Nested ternary and comma operators instead of `if` and loops

The whole function body is one `return` statement built from `?:` and `,`.
For example `t<_ ? main(t+1,_,a) : 3, main(-94,-27+t,a)` means "if more gifts
are left, print them first, then print this one". The values `1`, `3`, `9`
and `16` in the false branches are never used, they only fill the slot that
`?:` requires.

4. Misleading operator precedence

`return! 0<t?` looks like "return not (0 < t)", but `!` binds tighter than
`<`, so it is `(!0) < t`, which is `1 < t`. The test is really "is `t`
greater than 1", which is why `argc == 1` falls through to the branch that
calls `main(2,2,"%s")`.

5. Encrypted text

The long string of symbols like `@n'+,#'/*{}w+/w#cdnr/` is the whole song
written with a substitution cipher. The short string
`"!ek;dc i@bK'(q)-[w]*%n+r3#l,{}:\nuwloca-O;m .vpbks,fxntdCeghiry"` is the
key. Its first 31 characters are the scrambled letters and the 31 characters
after them are the real letters at the same positions. To decode a character
the program walks the key until it finds a match and prints the character 31
places later, `putchar(31[a])`. Writing `31[a]` instead of `a[31]` is the
same thing because `x[y]` is defined as `*(x+y)`.

6. String table separated by `/`

The encrypted text holds 27 pieces separated by `/`: `"On the "`, the twelve
ordinals, the `" day of Christmas..."` line and the twelve gifts. To print
piece `n`, the program starts with `t = -n` and adds one to `t` every time it
passes a `/`, `main((*a=='/')+t,_,a+1)`. When `t` reaches `0` it prints
until the next `/`. Calls such as `main(-87,1-_,...)` and `main(-94,-27+t,a)`
compute which piece they want, so the ordinal number and the gift number are
hidden in arithmetic.

7. Return values used as hidden ordering

`main(-79,-13,a+main(-87,1-_,main(-86,0,a+1)+a))` prints "On the ", the
ordinal and the rest of the header line. The return values are added to
pointers but never used. Nesting the calls as arguments makes them run from
the innermost outward, which forces the three pieces to print in the right
order.

8. Decoy format strings

`"%s %d %d\n"` and `"%s"` look like `printf` formats, but they are only
passed as the unused third argument and are never printed or read as a
format.

9. Unusual variable names and K&R style

The parameters are named `t`, `_` and `a` and declared in old K&R style with
`char *a;` spread across several lines, which makes the parameter list hard to
read. `phillipps.c` also adds a wrapper `main` that casts the arguments
and calls `pain`, so that the recursive function no longer has to match the
argument types that `main` is expected to have.
