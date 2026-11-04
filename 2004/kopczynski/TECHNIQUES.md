# 2004/kopczynski

Difficulty: Insane

1. Keyword redefined by the compiler command line

The Makefile passes `-Dif=while`, so every `if` in the source is really a
`while` loop. `if(I=l*4){...}` is the main loop over input lines, and the two
inner `if`s are the loops that read a line and that scan it. Someone reading
the code without the Makefile would think nothing ever repeats.

2. Meaningless and look-alike variable names

The variables are `O`, `I`, `Q` and `l`, which look like the digits `0`, `1`
and the number `1`. `O` is `argc`, used as the running total. `I` holds the
previous row, `l` the current row, and `Q` is first the last character read and
later the bit position being looked at. The unused `S` parameter (`argv`) just
adds noise.

3. One variable used for unrelated jobs

`Q` holds `getchar()-2`, so `Q-8` is zero exactly when the last character was a
newline (10-2 = 8). Once the line is done, `Q` is still 8, and `Q*=2` turns it
into 16, 32, 64, ..., the bit masks used to walk across the row. When `Q`
overflows to 0 the loop stops, which also resets it before the next line is
read. In the same way `O` starts as `argc` (1) and then becomes the total.

4. Rows stored as negative bitmaps

Each line becomes the bits of an `int`. It starts at `l=6` and each character
does `l = 2*l - bit`, so the line is built by subtracting instead of adding.
After the newline, `l` keeps doubling until it overflows into a negative number,
which lines the row up against the top bits. Because of the starting value 6,
the negated value `-l` is a marker bit followed by the row's pixels, with the
first column in a fixed spot. The `l>5` test is the loop condition for both
reading and shifting. The trick depends on signed overflow wrapping around,
which is why the Makefile forces `-O0`.

5. Division by negative powers of two in place of shifts

`I/-Q&12` and `l/Q%4` take two neighbouring bits out of the previous and the
current row. Since both rows are negative, dividing by `-Q` and by `Q` (with C's
round-toward-zero division and remainder) gives the same bits as right shifts on
the positive bitmaps. Subtracting the second part instead of adding it fixes the
sign. Together they form a 4-bit index describing one 2x2 square of the picture.

6. A string used as a table of numbers

`"has dirtiest IF"[...]` is not text to print. It is a 16-entry table (the
`'\0'` at the end is the 16th entry) of weights for each 2x2 pattern. Only the
total modulo 4 matters, so any character whose ASCII code has the right
remainder mod 4 works. The author picked a rule for counting corners, edges and
squares that happens to spell readable words.

7. Euler characteristic hidden behind modular arithmetic

Adding up the weights of all 2x2 windows gives the number of separate `#`
shapes minus the number of holes in them. The answer is always 8 to 11, so it
is enough to know the total modulo 4, and `8+O%4` turns it into the digit
printed. Extra values added by the marker bits and the end of each row are
multiples of 4, so they make no difference.

8. Pixels detected by odd character codes

`(Q=getchar()-2)%2` is 1 for any character with an odd code (`#` is 35) and 0
for even ones (space is 32, `.` is 46, newline is 10). That is why dots work as
well as spaces, and it means there is no `'#'` anywhere in the code.

9. End of input found through a zero row

There is no EOF check. The main loop stops when `I=l*4` is 0, which happens
only for a row with no pixels, because multiplying by 4 pushes the marker bits
out. That is why the only line without `#` has to be the last one, and why the
program loops forever if the input has no blank line at the end.

10. Everything on one line

The whole program is a single 131-byte line with no headers, an old-style K&R
`main(O,S)char**S;` definition, implicit `int` and implicit declarations of
`getchar` and `printf`, so the code is as short as possible.
