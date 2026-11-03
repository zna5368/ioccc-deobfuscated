# 1994/ldb

Difficulty: Medium

1. Trigraphs hide the brackets

`??(` and `??)` are trigraphs. With `-trigraphs` the compiler swaps them for `[`
and `]` before it does anything else. So `__??('}'??)` is really `__['}']` and
`&__??(_??)` is really `&__[_]`. A trigraph looks like two question marks
belonging to a `?:` operator. That confuses readers and code beautifiers: running
`clang-format` on the file turns `??(` into `? ? (`, which breaks the program.

2. Underscore and look-alike identifiers

The only names are `_`, `__` and `O`. `_` and `__` differ only in how many
underscores they have. `O` (the capital letter) looks like the digit `0`, so
`O+++_` and `rand()%O++` look like they use the number zero.

3. A character constant used as a number

The array size `'}'` is the ASCII code 125, and the offset `':'` is the ASCII
code 58. Nothing about these numbers has to do with text. They are just a hard
way to write 125 and 58.

4. An int array used as two text buffers

`__` is declared as `int __[125]` (implicit `int`), which is 500 bytes. The code
passes `__ + 0` or `__ + 58` to `fgets`. Adding to an `int` pointer moves by 4
bytes at a time, so `__ + 58` points 232 bytes into the array. That gives two
232 byte character buffers in one `int` array: the first holds the chosen line
and the second is a scratch area for lines that were not chosen. `fgets` reads
at most 231 characters plus the ending `'\0'`, which is why the author says lines
must be under 232 characters.

5. Global variables as hidden zeros and loop state

`_` is a global that is never changed, so it is always `0`. It is used as a
stand-in for zero in `(...?':':_)`, `&__[_]` and `O+++_`. `O` starts at `0` (all
globals do) and doubles as both a "have we started yet?" flag and the line
counter.

6. The whole program is the condition of a while loop

`while(...);` has an empty body. All the work happens in the condition. When
`O` is `0` the `?:` takes the second branch, which seeds the random number
generator and sets `O` to `1`. After that the first branch runs each time and
reads one line. When `fgets` returns `NULL` at end of input, `||` moves on to
`puts`, which prints the chosen line. `puts(...)&_` is `something & 0`, which
is always `0`, so the loop ends after printing.

7. Side effects buried inside expressions

`O+++_` is read by the compiler as `O++ + _`. It is `0` the first time, so it
works as the `NULL` argument to `time`, and it also bumps `O` to `1`.
`rand()%O++` both tests the random number against the line count and increases
the count in the same expression. The comma operator in `(srand(...),0)||O`
calls `srand`, throws away the result, and then checks `O` to keep looping.

8. Reservoir sampling hidden in a ternary

`rand()%O++?':':_` chooses where the next line goes. With line number `n` the
remainder is `0` with chance `1/n`, so the line goes to the chosen buffer.
Otherwise it goes to the scratch buffer and is thrown away. This is a known
way to pick a random line evenly without storing the whole file, but here it is
squeezed into one small expression with no comments.
