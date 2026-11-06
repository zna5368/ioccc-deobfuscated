# 2020/kurdyukov3

Difficulty: Medium

1. One loop working as a state machine

`for(srand(time(0));i;) j=j? ... : ...;` is the whole program. It has no
separate loop for reading a word and another for printing it. The variable `j`
does two jobs. When `j` is 0 the program is reading letters. When `j` is not 0
the program is printing the scrambled word, and `j` is the index of the next
letter to print. Each pass of the loop does one step of whichever phase is
active and then sets `j` to say what happens next. In simple.c the two phases
are two ordinary `while` loops inside an outer loop.

2. Nested ternary operators instead of if/else

The loop body is one long expression built from chained `?:` operators, for
example `--i<1&&a>>8?0:(putchar(...),i++)?t[j]=t[--i],i>2?rand()%(i-2)+2:1:0`.
`?:` groups from the right, so you have to match each `?` with its `:` by hand
to see which part belongs to which condition. The `if` statements and `while`
conditions in simple.c are the decoded version.

3. Comma operator and side effects inside conditions

Expressions such as `(putchar(i?t[j]:a),i++)` and `t[i++]=a,0` do some work and
then give a value for the surrounding condition. `(putchar(...),i++)` prints a
character and then tests the old value of `i` to decide whether the word is
finished. Because `i` is changed by `--i`, `i++` and `--i` in the same
expression, the reader has to keep track of its value at every step.

4. Bit tricks for character tests

`(a|32)-97<26` checks for an ASCII letter. OR-ing with 32 turns an upper case
letter into lower case, and subtracting 97 (`'a'`) gives 0 to 25 for letters.
Because `a` is `unsigned`, every other character wraps around to a huge
number. `a>>7==1` is true only for bytes 128 to 255 (the high half of a
single-byte character set), which also count as letters. `a>>8` is non-zero
only for `EOF`, because `getchar()` returns `-1`, which becomes `0xFFFFFFFF`
when stored in an unsigned variable. simple.c puts the letter test in an
`is_letter()` function and compares with `EOF` directly.

5. Hidden swap-and-shrink shuffle

The shuffle is never named. The word lives in `t[1..n]`. The first letter
printed is always `t[1]`, and its slot is refilled with the last letter
`t[n]`. After that the random index `rand()%(i-2)+2` never picks slot 1, so
the last letter is only printed once every middle letter is used up. Each
printed slot is refilled from the end of the array, and the array shrinks by
one. That is how the first and last letters stay in place while the middle is
shuffled. simple.c explains this in comments.

6. Magic numbers and short names

`97`, `32`, `26`, `64`, `>>7` and `>>8` stand for `'a'`, the case bit, the
number of letters, the buffer size, "is a high byte" and "is EOF". The
variables `a`, `i`, `j` and `t` are renamed to `c`, `len`, `pick` and `word` in
simple.c.

7. Shaped source layout

The code is broken into short lines of the same width so the file looks like a
card. Lines are split in the middle of expressions, for example between
`(26>((a=getchar())` and `|32)-97||1==a>>7)`, which makes it hard to see where
each expression starts and ends.
