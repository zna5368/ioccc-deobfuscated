# 2011/borsanyi

Difficulty: Medium

1. Data points stored in stack frames instead of an array

`p()` calls itself once for every number it reads, and the number stays in
that call's parameter `x`. So all of the data lives on the call stack, one
point per stack frame, and no array or `malloc` is needed. Once all calls
return, every frame looks at its own `x` again to check `(x>s&&x<=s+a)`, and
the results get added up on the way back. The rewrite does the same thing in
`read_points()`, but uses a local variable `value` and a normal `if`
statement.

2. Histogram bins made with `fork()`

There is no array of bin counters. When the input ends, the deepest call to
`p()` forks once for each bin (`x>1&&!fork()?p(t,s+=a,x-1):0`). A child
process gets a full copy of the parent's stack, data points included. The
parent keeps the current bin, and the child moves `s` up by one bin width and
forks again. Each process then counts only the points in its own bin as the
recursion unwinds, so every process is one bar of the histogram. The rewrite
puts this in a `while` loop inside `make_bins()`.

3. One function with three jobs chosen by the third argument

`p(t, M, x)` uses its argument `x` in different ways. When `x > 0` it is the
number of bins still left to fork. When `x <= 0` (the start call `p(1,0,0)`,
and later `--n`) it is ignored and overwritten by `scanf`. The first number
read is spotted by passing `t=1, M=0`: a minimum bigger than the maximum means
no data yet. `t` and `M` are the running minimum and maximum, but in the fork
step `M` is replaced by `s` because it is no longer needed. The rewrite splits
this into `read_points()` and `make_bins()` with clear names.

4. Recursion instead of loops

The program has no `for`, `while` or `if`. Every loop is a recursive call
joined with `?:`, `&&` and the comma operator. `main()` uses `argc` (`j`) as
its loop counter: the first call (`j==1`) builds the histogram and prints the
label, then calls `main(j*400/n)` with a negative number of bars. Each
following call prints a `#` and calls `main(j+1)` until `j` hits 0, then it
prints a newline. `putchar(35-25*!j)` gives `'#'` (35) or `'\n'` (10), and
`&' '` (32) is nonzero only for `'#'`, which is how the recursion stops. The
rewrite uses a `for` loop.

5. Negative counter

The global `n` is decremented (`--n`) for each number read, so it ends up as
minus the number of points. Dividing the positive bin count by it gives the
negative bar length that `main()` counts up to zero. The rewrite counts
`num_points` upward and loops down from a positive bar length.

6. Hidden ordering through argument evaluation

`printf("%+12g %4d",s+a/2,j,wait(NULL))` passes `wait(NULL)` as an extra
argument that the format string never uses. Its only job is to make each
process wait for its child (the next bin) to finish. Which argument is
evaluated first is not fixed by C. With GCC the `wait` runs before printing,
so the higher bins come out first. The rewrite calls `wait(NULL)` on its own
line before `printf`.

7. Disguised bin size calculation

`a=(M-t)/22` makes about 22 bins. `s=pow(10,(int)(log10(a))-2)` and
`a=s*(int)(a/s)` round the width down to three significant digits, and
`s=t-a/2` puts the center of the first bin on the minimum. Here the variable
`s` is used first as the rounding unit and then as the bin start. The number
of bins is `(int)((a+M-t)/a)`. The rewrite gives these values names
(`unit`, `bin_width`, `bin_start`, `bins_left`) and makes `22` and `400`
named constants (`NUM_BINS`, `HASHES_FOR_ALL`).

8. Short type macro and missing headers

`#define d double` shortens the type, and `fork()` and `wait()` are used
without including `<unistd.h>` or `<sys/wait.h>`, so they are implicitly
declared. The rewrite keeps the same two headers but writes `double` out.

9. Shaped layout and a fake comment

The code is laid out in columns with large gaps, and a comment
(`It is a simple tool to quickly analyze data.`) is put in the middle of the
expression `log10(a)) - 2`, splitting it across lines. The rewrite uses
normal formatting.
