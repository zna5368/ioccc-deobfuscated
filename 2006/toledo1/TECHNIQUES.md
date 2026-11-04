# 2006/toledo1

Difficulty: Medium

1. Checkerboard source layout

The source is cut into blocks of 4 characters with blank gaps between them so
that it looks like a chess board. Tokens such as `atoi`, `char` and `main` are
split across the "squares", and string literals are broken into pieces
(`"\1" "\7" "(d" ...`) that the compiler glues back together. Reformatting the
code with a code formatter undoes this.

2. One letter names and implicit int

All variables are single letters (`e`, `t`, `f`, `g`, `h`, `i`, `d`, `m`) with
no hint of what they hold. `d,m;` is declared with no type at all, which old C
treats as `int`. `main(c,b) char **b;` uses the old K&R style of declaring
parameters, and `c` (normally the argument count) is reused as the "best score
so far" variable.

3. Reversed array indexing

`d[t]` is used instead of `t[d]`. In C `a[b]` means `*(a + b)`, and addition
works in either order, so `d[t]` is just the `d`-th character of the array `t`.

4. Whole program inside `for` headers

Both loops that do the work have empty bodies (`;`). The board drawing is
done entirely in the condition of the first `for`, and the knight search puts
its assignments, tests and updates inside the three parts of nested `for`
headers joined with the comma operator. `366>++d` is the loop test hidden at
the end of a comma list, after the assignment that does the real work.

5. Nested conditional expression instead of if/else

`d%3 ? 60<d&300>d&6<d%30 ? 0 : 32 : d%30 ? 32 : 10` picks the character for
each board position (empty square, space or newline) using chained `?:`. The
`&` operators combine three comparisons; because each comparison gives 0 or 1,
the bitwise `&` acts like a logical "and".

6. The board is the output text

There is no separate 8x8 array. The array `t` is the exact text that `puts`
prints: lines of 30 characters, 3 characters per square, with a border of
spaces around the real squares. Free squares hold `'\0'`, visited ones hold
the move number as two digits. Moving the knight is done by adding a
character offset to a pointer (one square = 3 characters, one line = 30
characters), so the knight moves become numbers such as 63, 57, 36 and 24.

7. Knight moves encoded as a string

The string `"\1\7(d\177yX\34"` holds the 8 knight jumps. Each character is a
jump offset plus a bias: the code subtracts 64 (`*f++-64`) to get the offset
from the target square, and 65 (`-65+*i`) to get the offset from the ones
digit of the current square. For example `'d'` is 100, and 100 - 64 = 36,
meaning one line down and two squares to the right. The bias keeps all values
positive and non-zero, so the string's ending `'\0'` stops the loops.

8. Arithmetic in place of comparisons

The Warnsdorff score is `d = *h` followed by `d -= !!h[...]` for every
neighbor. `!!x` turns any non-zero value into 1, so this counts the used
neighbors. Because a free square starts at 0 and a used square starts at a
digit or space character code (32 or more), one subtraction loop both ranks
free squares and rules out used ones, with no explicit "is it free?" test.

9. Hidden digit printing and starting position

`++m/10+48` and `48+m%10` write the move number as two ASCII digits (48 is
`'0'`). `3*atoi(*++b)+34+t` converts the two digit argument "RC" straight into
a pointer to that square in the text: 3 characters per column, 30 per row,
plus 34 to skip the border.

10. Pointer used as the loop condition

The outer loop condition `e=g` copies the chosen square into the cursor and
stops when no square was chosen (the pointer is null). `g=d<c?c=d,h:g` keeps
the best square found so far, updating the best score `c` inside the
conditional with a comma expression.
