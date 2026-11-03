# 1991/fine

Difficulty: Easy

1. Bit masks in place of character comparisons

The code never writes `'A'`, `'Z'` or `isalpha()`. It uses `b=64^a&223`
instead. `a&223` clears bit 5 (value 32), the bit that makes a letter lower
case, so `'a'` and `'A'` both become 65. `64^` then flips bit 6 (value 64),
so `'A'` becomes 1 and `'Z'` becomes 26. Any character that is not a letter
ends up as 0 or a number above 26. In simple.c the masks are written in hex
(`0xDF`, `0x40`) and the result is stored in `letter_index`.

2. Range check folded into one comparison

`(b=...)&&B<27` checks that the value is "not zero and less than 27", which
is the same as "between 1 and 26". The lower bound is hidden in the `&&`
truth test, and characters that did not have bit 6 set end up above 26 after
the flip, so they fail the `<27` test without a separate check. simple.c
writes `letter_index >= 1 && letter_index <= 26`.

3. Case restored with an OR instead of a branch

`a&96|...` keeps bits 5 and 6 (values 32 and 64) of the original character
and ORs them back onto the rotated letter number. That turns 1..26 back into
`'A'..'Z'` or `'a'..'z'` depending on the input's case, with no `if` for
upper versus lower case. simple.c names this value `case_bits`.

4. Combined arithmetic constants

`(B+12)%26+1` does three steps in one expression: subtract 1 to get a
position from 0 to 25, add 13 to rotate, take `% 26` to wrap around, and add
the 1 back. The `-1` and `+13` are merged into `+12`, which makes the
rotation by 13 hard to spot. simple.c writes `(letter_index - 1 + 13) % 26 + 1`.

5. EOF test written as `+1`

`while((a=getchar())+1)` loops while `a+1` is not zero. `getchar()` returns
`-1` (EOF) at the end of input, and `-1+1` is `0`, which is false. So this
is a short way of writing `!= EOF`. simple.c writes `!= -1` because the entry
includes no headers that define `EOF`.

6. Whole program in a ternary inside `putchar()`

The `if`/`else` that decides between "rotated letter" and "unchanged
character" is a `? :` expression passed straight to `putchar()`, together
with the assignment to `b`. Everything is in one statement on one line.
simple.c splits it into an `if`/`else` with two `putchar()` calls.

7. Makefile macros that change `main` and reuse a pointer as an integer

The Makefile adds `-D'main(a,b)'='int main(a,b)char**b;'`, so the source can
write `main(a,b)` while the compiler sees an old style definition where `a`
is an `int` (the argument count) and `b` is a `char **`. The program then
abuses these two parameters as its only variables: `a` holds the character
and `b`, a pointer, holds a small number. Because comparing a pointer with
27 would not compile cleanly, a second macro `-DB='(int)b'` casts it back to
an `int` wherever `B` appears. simple.c uses ordinary local `int` variables,
and writes `(main)` in parentheses so the Makefile's `main(a,b)` macro is
not applied to it.
