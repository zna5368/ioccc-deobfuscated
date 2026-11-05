# 2015/howe

Difficulty: Medium

1. Single letter names

Every type, function and variable has a one letter name, so nothing says what it
is for. `E` is one edit, `X` is the end of a diagonal, `H` is the table of lines,
`F()` hashes a file, `K()` moves one diagonal forward and `T()` runs the whole
comparison. Even struct members are letters, such as `z` for the capacity and `j`
for the line count. `simple.c` uses names like `Edit`, `LineTable`, `hash_lines`
and `extend_diagonal`.

2. The same letter reused for different things

C keeps some names apart, and an inner name hides an outer one, so one letter can
mean several things at once. `h` is a type, a local hash variable in `F()` and a
local struct in `K()`. `Q` is a global array, but it is a `FILE *` inside `F()`,
`O()` and `W()`. `O` is a function and also the parameter of `N()`. `K` is a
function and the parameter of `R()` and `D()`. `y` is a struct member in three
different structs and the `argv` parameter of `main()`. To read one line you must
first work out which meaning is in scope there.

3. Comments removed and blank lines squeezed out

The struct `E` lists all of its members on a single line, separated by long runs
of tabs, and stray tabs trail several other lines. These gaps are where comments
used to be. No function has a blank line or a comment, so nothing shows where one
step ends and the next begins. `simple.c` has one member per line with a comment
and explains each function.

4. Multiplication hidden as shifts and additions

`h += (h<<1) + (h<<4) + (h<<5) + (h<<7) + (h<<8) + (h<<40)` in `F()` does not look
like a known formula. Shifting left by n multiplies by 2 to the power n, and
adding `h` to all of the shifted values gives `h * 1099511628211`. That number is
the FNV prime, so the line is one step of the FNV-1a hash. `simple.c` writes
`hash *= FNV_PRIME`.

5. Unexplained numbers

`14695981039346656037ULL` appears twice with no name. It is the FNV starting
value. `65536` is used as both the first table size and the read buffer size, and
`2` is the exit status for errors. `simple.c` gives each one a named constant.

6. Arguments passed through global variables

`A`, `B`, `f`, `g`, `Q`, `q`, `I` and `P` are globals with no comment, and `K()`
and `T()` read and change them without showing it. `T()` takes no arguments at
all, so a call gives no hint of what it works on. Two pairs look almost the same
but differ: `Q` and `q` are the same array, with `Q` moved to the middle so that
negative diagonal numbers work, and `A`, `B` can swap while `f`, `g` never do.
`simple.c` keeps the globals but names them `diagonals`, `diagonal_storage`,
`table_a`, `table_b`, `file_1` and `file_2`.

7. Swapping the files with a flag and XOR

The algorithm needs the shorter file first, so `T()` swaps `A` and `B` and sets
`I`. From then on every edit is backwards: an insert is really a delete. `K()`
fixes this with `e->op = op ^ I`, which flips the 0 or 1 in `op` only when `I` is
1. It then swaps the fields of the edit afterwards, using `c` as a temporary
variable. `simple.c` has one `if (files_swapped)` with each case written out.

8. Assignments inside conditions

`if (!(Y = realloc(...)))`, `if (!(Q = fopen(u, "r")) && !strcmp("-", u))` and
`if ((Q = tmpfile()))` do the work and test the result in the same expression.
The `!` in front of a pointer or of `strcmp()` also reads the opposite way to what
it checks. `simple.c` assigns first and then compares with `NULL` or `0`.

9. Backwards comparisons

Conditions are written with the constant first, as in `0 < y` and `0 < i`, so
they read the opposite way to normal speech. `while (0 < n--)` also changes `n` as
it tests it. Together with `b++` inside `if (*b++ == '\n')`, the loop counter and
the pointer are both moved inside tests. `simple.c` moves both into the `for`
statement.

10. A list that is built backwards

Each new edit in `K()` points to the edit before it, so that many paths can share
the same earlier edits. The finished list runs from last to first, and `R()`
reverses it before printing, without saying why. `D()` then walks it with three
pointers, `K`, `a` and `b`, where `K` is also moved by the printing loop and is
the only thing that ends the outer `for ( ; K; )`. `simple.c` names them `first`,
`last` and `after` and uses a `while` loop.

11. A sentinel and an odd loop start

`q[k].y = -1` marks every diagonal as not reached, and `p = -1` with `p++` at the
top of the `do` loop makes the first round use 0. This works only because of how
`v.y < h.y+1` behaves with -1, so that the very first step lands on point (0, 0)
with no edit, which `if (0 < y || 0 < x)` then has to skip. `simple.c` starts the
loop at 0 and explains the first step in a comment.

12. A one element array used as a longer array

`H` ends with `h b[1]`, yet the code uses `b[L]` with `L` in the thousands. The
struct is allocated with extra room by `realloc()`, so the array really runs past
its declared end. `sizeof (*O)` and `sizeof (O->b)` are also taken from `O` when it
is a null pointer, which looks like a crash but is safe because `sizeof` never
reads the value. Element 0 is left unused so that the index is the line number.

13. Forcing an error through a library variable

In `main()`, an unknown option runs `optind = x`. This does not report anything
itself. It sets the index of the next argument to the end of the list, so the
test `x <= optind` that normally catches missing files prints the usage message
instead. `main()` also calls its parameters `x` and `y` in place of `argc` and
`argv`, the same letters used for line numbers everywhere else.

14. Checks that can never fail

`main()` tests `if (!(A = F(f)))` and prints an error, but `F()` can never return
a null pointer because `N()` exits the program when memory runs out. The test
makes it look like `F()` has a failure case to think about. `simple.c` leaves it
out.
