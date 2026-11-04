# 2011/fredriksson

Difficulty: Hard

1. Program hidden inside compressed data

The real program (the approximate grep) is not in the C code at all. It is
stored in the string `p[]`, together with the file name `ag.c`, the format
`%.*s` and the command `gcc -O3 ag.c -o ag;./ag`. The outer program only
unpacks this string, writes `ag.c`, and asks the shell to compile and run it.
Reading the C code tells you nothing about what the program does.

2. Burrows-Wheeler transform

The text in `p[]` was shuffled with the Burrows-Wheeler transform, so it looks
like random characters. To undo it, the code counts how many times each
character was seen before each position (`D[i]=~-(i[p][b]=-~i[p][b])`), adds
the counts up (`z=b[i],i[b]=s,s+=z`), and then follows the chain
`j=D[j]+j[p][b]` from the start row `1601`, writing `O[]` from the end
backwards. This is the "LF-mapping" step of the inverse transform.

3. Unary run-length encoding with whitespace

Before the transform is undone, `p[]` is unpacked from right to left: every
space becomes a copy of the character to its right, `@` becomes a real space
and `$` becomes a NUL byte (`p[i]==' '&&(p[i]=(-~i)[p])||...`). So a run of 5
`x` is stored as 4 spaces and an `x`. This is why the string is mostly
spaces, and why removing even one space breaks the program.

4. Recursion instead of loops, `main` as every function

`main` is the only function. It is stored in the pointer `Q` and called again
with a special first argument (`-1`, `-2`, `-3`, `-4`, `-5`) to pick which
"subroutine" runs. Each subroutine does one step and then calls itself
(`i=-~i,Q(_,v)`) until its condition fails, which replaces a `for` loop.

5. Short-circuit operators instead of `if`

There is no `if`, `for`, `while` or `?:`. The whole of `main` is one
expression joined by `&&` and `||`. `A&&(B)` means "if A then B", and
`X||Y||Z` means "try X, else try Y, else try Z". The comma operator puts many
statements inside one pair of brackets.

6. Numbers made without digits

No digit appears in the code. `!!"\b"` is 1 (a string is a non-null pointer),
`~!!"\b"` is -2, `-~!!"\b"` is 2, `~-~!!"\b"` is -3. Character arithmetic is
also used: `'C'*' '-'-'` is 67*32-45 = 2099 (the payload length),
`-~('('*'(')` is 40*40+1 = 1601 (the start row), `'['+']'` is 184 (size of the
count array) and `'-'-'-'` is 0.

7. Two's complement tricks for +1 and -1

`-~x` is `x+1` and `~-x` is `x-1`, because `~x` equals `-x-1`. So
`i=-~i` is `i++` and `i=~-i` is `i--`. `x^=x` is used to set `x` to 0.

8. Raw backspace characters in the source

The `"\b"` strings are written with a real backspace byte instead of the
escape `\b`. When the file is shown with `cat`, the backspace moves the cursor
back and hides the opening quote, so the code looks broken on screen.

9. Reversed array indexing

`i[p][b]` means `b[p[i]]` and `i[O]` means `O[i]`, since `a[b]` is the same as
`*(a+b)` and addition can be swapped.

10. Undefined behaviour in building the command

`sprintf(p,"%s %s",p,i[v])` reads from and writes to the same buffer `p`. This
is not allowed in C. On current glibc it erases the buffer first, so only a
space and the last argument are left, and the shell runs the last argument as
a command (`./fredriksson -k3 cat` really runs `cat`). simple.c keeps this
call so that it behaves exactly like the original.

11. The generated program is obfuscated too

`ag.c` uses the same tricks (only `main`, recursion, `&&`/`||`, numbers from
`!!"\b"`, `-~x`) plus one-letter variables used for several jobs (`e` is the
"escape" flag, then the "parse options" flag, then the match counter; `q` is
"ignore case" and later "line already matched"). Its search is Myers'
bit-parallel edit distance algorithm, where each bit of a `long` holds one
pattern position (`O=(t&v)+v^v|t|n`, `h=n|~(O|v)` ...). simple.c writes this
file unchanged and does not rewrite it.
