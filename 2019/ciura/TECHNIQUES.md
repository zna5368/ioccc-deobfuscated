# 2019/ciura

Difficulty: Hard

1. Macros that hide code fragments

`#define A 1;O-L;L=0<L[o]?L[K][k]=L` and `#define N for(O=l[k];l-O;O=O[k])for(L=O`
are not whole statements, only pieces. In `y()` the code `N-A,...` becomes
`for(...)for(L=O-1;O-L;L=0<L[o]?...`, and in `b()` the code `N+A[k],...` becomes
`for(L=O+1;...;L=0<L[o]?L[K][k]=L[k],...`. The `-` or `+` placed after `N`
decides whether the loop walks left or right. The same macro `A` is reused in
both directions. Macro `G` similarly expands into the head of a `for` loop and
`R(A,M)` is just `wcschr(1+M,A)`. You only see the real code after running the
preprocessor.

2. Reversed array indexing

C lets you write `a[i]` as `i[a]` because both mean `*(a+i)`. The program uses
this all over the place: `l[V][v]` means `v[V[l]]`, `c[C]` means `C[c]`,
`2[j]` means `argv[2]` and `I[w]` means `w[I]`. Chains like `L[K][k]=L[k]` make
it hard to see which array is being written.

3. Short, confusing variable names

Single letters that differ only by case are used for related things: `k`/`K`
are the down/up links, `v`/`V` the right/left links, `c`/`C` the current level
and the stack of choices, `w`/`W` the start of the current word and the write
position, `z`/`Z` the letter list and a word scanner, `l`/`L` loop indices.
The array `o` is used two ways: for letter headers it holds how many words
contain the letter, and for every other node it holds which letter the node
belongs to (or a negative word number for a separator node).

4. Reusing `main`'s parameters and globals as scratch variables

`main(int I,char**j)` uses `I` (normally `argc`) as a loop counter after the
argument check, and stores the error message into `*j` (`argv[0]`) before
printing it. Global variables `l`, `O` and `L` are shared between `main` and
the helper functions `y()`/`b()`, which take no parameters and instead read the
item number from the global `l`.

5. Exit code computed from a string

`return fputs(*j,stderr),**j/36;` divides the first character of the error
message by 36. `'D'` (68) gives exit status 1 for "Dictionary too long" and
`'N'` (78) gives 2 for "Need an argument or two". Both errors share one
`return` reached with `goto H`.

6. Comparisons written as subtraction

`l-O`, `O-L`, `I-c`, `c[C]-I` and `WEOF-S` are used as loop or `if` conditions.
A subtraction is non-zero (true) exactly when the two values are different, so
each one is really a `!=` test.

7. Strange constants and operator precedence

`#define P 9<<23^1` is `(9<<23)^1` = 75497473, used for buffer sizes. Because
the macro has no parentheses, `98^P` turns into `98^9<<23^1` and `P>W-s`
turns into `9<<23^(1>W-s)`, since `>` binds tighter than `^`. That last one is
always non-zero, so the buffer size check that looks like it is there never
stops the input loop. `while(P)` is just `while(1)`, and `:P` in a `?:` is a
dummy value that does nothing. `33>(*W=S)` treats any character below `'!'`
(space, tab, newline, ...) as a word separator, and `putchar(10)` prints a
newline.

8. Tricky format string

`printf(!I+" %ls",...)` adds 0 or 1 to the address of the string. For the first
word (`I` is 0, so `!I` is 1) it skips the leading space; for the rest it
prints the space before the word.

9. Bitwise NOT to decode negative numbers

Each separator node after a word stores `p-q`, which is the negative of
(word number + 1). `~O[o]` turns that back into the word number, since
`~x` equals `-x-1`. The test `0>~O[o]` is a disguised `O[o] >= 0`, used to
walk right until a separator is found.

10. Packing work into loop headers and comma expressions

Most loops have empty bodies; all the work is in the `for` header, chained with
commas and `?:`. For example, the word insertion
`++o[O=R(I[w],z)-z],k[K[l[o+I+1]=O]=k[l[K+I+1]=O[K]]=l+I+1]=O;` does six
assignments in one expression to append a node to the bottom of a letter's
column. The init loop `V[l[v]=l+1]=l[K]=l[k]=l` builds the circular list of
letter headers in a single statement.

11. `goto` into the middle of a nested `if`

The search (Knuth's Algorithm X with dancing links) is written as one
`while` loop with `goto T` jumping back into the `if(l-c[C])` test, `continue`
to go one level deeper, and `goto M`/`goto H` to skip a bad word or reach the
error return. The steps "try the next option" and "backtrack" are not visible
as separate parts of the code. In simple.c these are split into functions
(`cover`, `uncover`, `cover_option`, `next_option`, ...) and two nested loops.

12. Dancing links data structure

All lists are kept in plain integer arrays (`k`, `K`, `v`, `V`, `o`) instead of
structs with pointers. Words are rows of nodes laid out one after another with a
separator node between them; the separator's links point to the first node of
the word before it and the last node of the word after it, so a row can be
walked around in a circle. Covering and uncovering a letter unlink and relink
nodes in place, which is fast but hard to follow without Knuth's description.
