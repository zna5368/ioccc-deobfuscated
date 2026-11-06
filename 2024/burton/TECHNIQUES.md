# 2024/burton

Difficulty: Hard

1. Meaningless names and type aliases

`typedef void q; typedef int _; typedef char p;` hide the three basic types
behind one-letter names, so `_ s` is an `int` and `p*` is a `char *`. Every
function (`W`, `O`, `R`, `D`, `L`, `e`, `S`, `o`, `l`, `v`, `E`, `r`) and most
variables are single letters, and the same letters (`i`, `j`, `k`, `l`, `o`)
get reused in nested scopes with different meanings. The global struct `i` is
even shadowed by a local `p i` in `D()`, and `o` is both a function and a
local variable inside that function. In simple.c these are `copy_word`,
`contains`, `die`, `add_word`, `filter_words`, `merge`, `sort_descending`,
`break_ties`, `choose_guess` and `process_feedback`, and the struct fields
became `black`, `misplaced`, `green`, `yellow`, `num_guesses` and `num_yellow`.

2. Macros that hide loops

`J(x)` expands to `for (_ j=0; j<5; ++j) x;` and `A(x)` expands to a loop
over a word's letters that stops as soon as `o` becomes true. `L()` stacks
several `A(...)` calls and a `J(...)` with no visible `for` keywords, so you
cannot see where each loop starts or ends. The macro `I` expands to the six
parameters `_ W,_ O,_ R,_ D,_*L,_*E`, so `e(I)` and `o(I)` look like they take
one argument but really take six. Its parameter names are the same letters as
the global function names, which shadows them inside those functions.

3. A table holding three kinds of data

The single array `a[]` holds (1) nine start offsets into itself, (2) sorting
network comparisons packed as hex nibbles (`0x12` means "compare items 1 and
2"), and (3) how common each letter is in English, starting at index 55
(reached with `a[~-9]`, which is `a[8]` = 55). `S()` finds its comparisons
with `&a[a[O]]` up to `a[O+1]`. In simple.c the sorting networks are replaced
by a simple insertion sort, which gives the same result because all the values
it sorts are different, and the letter table became `english_rank[]`.

4. Packing two values into one integer

The letter counts are stored as `count * 32 + letter` (`c[t[l][j]-97] += k`
where `k` is 32), so sorting the number sorts by count while the low 5 bits
remember which letter it was. `l()` then swaps the upper bits for the English
frequency (`f[j] &= 31, f[j] |= a[...] << 5`) to break ties. simple.c keeps
this but names it with `LETTER_BITS` and `LETTER_MASK`.

5. Merge sort spread over nested calls

`v()` calls `o(2,3, o(0,1,0,a,w,k), a,w,k)`: the inner call's return value is
used as an argument to the outer call, and the numbers `0,1,2,3` are
multiplied by the length to choose which part of the array to read. The
helper `e()` copies leftover items. Swapping the last two arguments in the
third call reverses the source and destination arrays. simple.c uses a normal
`merge(dst, left, right, len)` function called three times.

6. Arithmetic and bitwise tricks for constants

Numbers are written in strange ways: `-~O` is `O+1`, `~-9` is `8`, `~-k` is
`k-1` (31), `j-~5` is `j+6`, `~0x66` is `-103` (minus the letter `'g'`),
`u^n` is `32^2` = 34 (the fgets size), `~-s` is `s-1`, `~(n=getchar())` is
zero only at `EOF`, and `u` is the space character used as both the number
32 and the lower-case bit (`u|*a`). `*d-120` checks for the letter `'x'`.
`a[]` also contains an octal literal `06` and a `+17` to look odd.

7. Swapped array index

`s[t]` is the same as `t[s]` because `x[y]` means `*(x+y)` in C, and addition
works in either order. It is used in `D()` to get the next empty word slot.

8. Pointer arithmetic outside the array

`main()` uses `i.c - 97` and `i.c - 97 - 11` as the dictionary line buffer and
the user input buffer. These point into the unused upper half of the
`i.a[257]` array, which is only indexed by lower case letters. simple.c gives
these buffers their own arrays.

9. Expressions used as statements

Conditions are written with `&&`, `||` and `?:` instead of `if`: for example
`j-5||++s`, `n-2?R(...):5`, `l<j&&R(...)`, `o?W(t[l],t[--s]):++l`, and the
long `!o ? i.a[n]=!++k : O(i.d,n) || (*(o<0?&i.a[n]:&i.d[i.f++])=l=n)` which
picks the array to write by taking the address inside a `?:`. `!++k` adds one
to `k` and gives 0 at the same time. `(o?i.b[i.e]:i.c)[j]=n` chooses which
array to index with a `?:`. `e[i][j]||(e[i][j]=t[l],j=4)` stores a word and
then ends the loop by setting `j` to its last value. `k=!++j` moves to the
next item and resets `k` in one step.

10. Misleading return values

`W()` returns 0 for no reason so it can be used inside `?:`. `O()` returns
`5-(a-c)`, a strange number between 6 and 10 when the letter is found, but it
is only ever used as true or false. `R()` is declared to return `int` but never
returns because it calls `exit()`.

11. Layout and comments

The code is formatted into columns and blocks with spaces in the middle of
names and expressions (`*a  ++`, `w +++ D*O`), and fake comments such as
`/*IOCCC*/`, `/* ;  ; */` and a trailing `//` break up the text. A
`#define` is continued over several empty lines with `\` and the string
`"xyzzy"` and the messages are split into pieces like `"inp" "ut: guess"` that
the compiler joins back together.
