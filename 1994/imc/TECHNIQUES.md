# 1994/imc

Difficulty: Hard

1. Single letter names

The functions are called `r`, `p`, `o`, `s` and `e` and the variables are
`i`, `j`, `k`, `l`, `m`, `q`, `z`... Nothing hints that `o` makes odd squares,
`s` makes the 4x4 square, `p` shuffles an array or that `h` and `k` count how
many anti diagonal and main diagonal cells still need to be swapped.

2. Shared global variables

`i`, `j`, `k`, `l`, `m`, `t`, `u` and `v` are globals used as scratch space
by every function. `p()` silently changes the globals `i`, `j` and `k`, and
`e()` hides some of them with locals of the same name (`i`, `k`, `l`, and `m`
which becomes a function pointer) while still using the global `j`, `u` and
`v`. The reader has to track which `i` or `m` is meant at every line.

3. Whole loops packed into `for` headers

Almost all the work happens inside the three parts of a `for` statement with
an empty body. For example the odd square fill
`for(p(q,n-1),q[n-1]=l,u=a,i=j=0; i<n*n; u[i]=q[j]*m,j++,++i%n||(++j,u+=d),j>=n&&(j-=n));`
shuffles, initialises, stores a cell, advances the column, handles the end of
a row and wraps the index, all with commas. In simple.c these are ordinary
nested row and column loops with the index written as `(row + col) % n`.

4. `||` and `&&` used as `if` statements

`++i%n||(++j,u+=d)` means "if this was the end of a row, do the extra work",
and `j>=n&&(j-=n)` means "if j went past the end, wrap it". Because `||` only
evaluates its right side when the left is false (and `&&` only when the left
is true), they act like hidden `if` statements.

5. Search loops with side effects in the condition

`for(j=g;q[--j]||(q[j]=1,q[z]=0););` walks backwards to find a free slot and
then moves a bit into it, all inside the loop condition. The assignment part
evaluates to 0 so it also ends the loop. simple.c uses the helpers
`move_to_highest_clear()` and `highest_set()` instead.

6. Dangling `else` with nested `if`s

In `e()` the code is laid out as
`if(...) if(k)k--; else ... else if(k>i) ...` without braces. Each `else`
belongs to the nearest unmatched `if`, so the reader has to count carefully to
see which branch is which.

7. Nested conditional operators choosing a function

`(n!=4?n%2?o:e:s)(n,a,b,0)` picks the right square maker with two `?:`
operators and calls it immediately. In `e()` the same choice is stored in the
local pointer `m` while also being called inside the first `for` header. This
also hides the recursion: an even square of order `n` calls itself for order
`n/2`. simple.c uses a named `choose_maker()` function.

8. Odd array indexing

`0[t++]`, `1[v]`, `t++[0]` and `l[u]` use the fact that `a[b]` is the same as
`*(a+b)` and `b[a]`. `l[u]` is especially confusing because `l` is an integer
offset (half the square further down) and `u` is the pointer.

9. Exclusive-or arithmetic

The 4x4 square picks each digit with `q[j^(i&m)^(i/2)]` and builds the digit
table with `1^q[r(2)]` and `3-...`. These bit tricks generate the pattern of
digit pairs without spelling out a table, so the reader has to work through
the bits to see why each row sums to the same value.

10. Hidden row gap parameter

Every maker takes a gap `d` that is added to the pointer after each row
(`u+=d`, `t+=d`). This lets `e()` fill a quadrant of a bigger square in place,
but it means the array index `u[i]` is not where it looks: the real position
also depends on how many gaps were skipped. simple.c computes each row start
as `square + row * (n + gap)`.

11. Macro for `sizeof(int)`

`#define f (sizeof(int))` turns the allocation into `malloc(n*n*f)`, which
looks like a multiplication by an unknown variable.

12. K&R function definitions

`void o(n,a,q,d)int n,d,*a,*q;` declares the parameter types after the list,
in a different order from the parameter list itself.
