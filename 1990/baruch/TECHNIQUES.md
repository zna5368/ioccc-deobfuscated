# 1990/baruch

Difficulty: Hard

1. Whole program in one `for` statement

The entire search runs inside the header of one `for` loop whose body is just
`;`. The initializer reads the input (`scanf("%d",&s)`), the condition
`*a-s` stops when the first queen has moved past the last column, and all of
the real work is a long comma-separated "increment" expression. Each piece
separated by a comma runs in order, so the increment is really a list of
statements squeezed into one expression.

2. Short-circuit operators instead of `if`

There are no `if` statements. `a && b` only runs `b` when `a` is true and
`a || b` only runs `b` when `a` is false, so they act like `if`. For example
`v || (...)` only does the "place or backtrack" step when `v` is zero, and
`...>=s*k && ++a[--i]` only backtracks (moves to the previous row and tries
its next column) when the current row ran out of columns.

3. A state machine hidden in global variables

Instead of separate loops for "check a queen", "place a queen", "backtrack"
and "print the board", one set of variables decides which job happens on each
pass of the loop. `i` is the current row, `j` walks over the earlier rows
(or over the rows being printed), `l` is the column being printed and `k`
(`i<s`) says whether we are still placing queens (1) or printing a finished
board (0). In simple.c these become the `is_safe` loop, the `print_board`
function and an `if`/`else if` chain inside `while`.

4. Reusing one variable for many meanings

`v` holds the column difference `a[j]-a[i]` between two queens, then the
true/false result of the safety test or print step, and then, through
`j*=v`, it resets `j` to 0 whenever the last step failed. In print mode,
`a[i]` is `a[s]`, which was set to 0, so the same `a[j]-a[i]` simply becomes
the queen's column in row `j`.

5. Diagonal test written as subtractions

`v && v-i+j && v+i-j` checks that the difference in columns is not 0 (same
column), not `i-j` and not `j-i` (same diagonal). Writing the comparisons as
"is this subtraction non-zero" hides that they are equality tests.

6. Pointer arithmetic on a string literal for the format

`2+"\n\n%c"-(!l<<!j)` starts at `"%c"` and steps the pointer back by 1 or 2
characters to include one or two newlines. `!l<<!j` is 0 in the middle of a
row, 1 at the start of a new row and 2 at the start of a new board, so the
same `printf` prints the line breaks and blank lines.

7. Indexing a string literal to pick a character

`" #Q"[l^v ? (l^j)&1 : 2]` chooses one character from the string: `'Q'` when
the column `l` equals the queen's column (`l^v` is 0), otherwise `' '` or
`'#'` depending on whether `row+column` is even or odd (`(l^j)&1`), which
draws a checkerboard.

8. Function results used as truth values

`!!printf(...)` turns the number of characters printed into 1, and `++l`
is always non-zero, so both keep the `&&` chain going while their side
effects (printing and moving to the next column) happen. `!(l%=s)` wraps the
column back to 0 at the end of a row and only then lets `j` move to the next
row.

9. Implicit `int` and implicit declarations

`v,i,j,k,l,s,a[99];` declares global `int`s without writing `int`, and
`main()` has no return type. No headers are included, so `scanf`, `printf`
and `puts` are used without declarations. Globals start at zero, which the
program relies on for the starting row, column and counters.
