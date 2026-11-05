# 2013/birken

Difficulty: Hard

1. Line that fools the size tool

`char*_ = "'""/*";` is never used by the program. The quote and `/*` inside the
string confused the contest's size counting tool, which believed a comment had
started and stopped counting the rest of the file. To the compiler it is only
an ordinary string.

2. Loops hidden in macros

`#define o(l, k) for(l=0; l<k; l++)` and `#define n(k) o(T, k)` remove almost
every `for` from the source. `n(42)o(e,m)` is really two nested loops, with `T`
as the outer counter and `e` as the inner one. Since `n` never shows its counter,
`T` seems to appear from nowhere inside the loop body.

3. Macro arguments that split apart

`n(4&&L)` looks like a loop that runs `4&&L` times, which would be at most once.
The macro pastes its argument in without parentheses, so it becomes
`T<4&&L`, which means "four times, but only when `L` is not zero". In the same
way, `o(a, m&&e==m)` becomes `a<m&&e==m`, which means "every column, but only
when the row was full". The argument reads as one value and acts as an extra
`if`.

4. A macro that replaces a library function

`#define exit(x) return system("reset")` makes the final `exit(0)` look like a
normal way to leave the program. It really runs the `reset` command and returns
its result, and the `0` is thrown away.

5. Piece shapes packed into a string

`"5'<$=$8)Ih$=h9i8'9t=)83)l4(99(g9>##>4("` holds the shape of every piece. Each
coordinate only needs 2 bits, so four of them are packed in one character. Then
35 is added so that every byte is a printable character. `(... -35)>>e*2&3`
removes the 35, shifts to the wanted coordinate and masks off the other three.
Even characters hold the x coordinates and odd characters hold the y coordinates.

6. Piece colors packed into a string

`"+%#,45>GP9$5-,#C?NX"` holds 38 color numbers of 3 bits each, two in every
character. `f[T+t+T]=(...-35)>>t*3&7` unpacks them into one list. The first half
of the list is for normal pieces and the second half is for mirrored pieces,
which is why `f[p?19+l:l]` adds 19. The list is split in the middle, while each
character holds two neighbours, so a pair in a character has no relation to a
normal and mirrored color of one piece.

7. Move lists written as a long string

The huge string that runs over nine lines looks like a mathematical formula, but
it is a list of moves. Every two characters are a piece number and a column,
each with 40 added. Adjacent strings and backslash-newlines cut it into pieces
to fit the shape of the source.

8. List lengths that are not stored

Nothing in the big string says where one list ends. `s=T>9?m:(T&3)-3?15:36`
works the lengths out from the list number. `(T&3)-3` is zero only for lists 3
and 7, and those are the long ones. `E+=s+s` keeps a running position in the
string, so a list can only be found by decoding all of the lists before it.

9. Indexing a string backwards

`T["+%#,4""5>GP9$5-,#C?NX"]` has the array and the index swapped. In C, `a[b]`
means `*(a+b)`, and addition works in either order. So this is the same as
`"..."[T]`, but it is much harder to recognize.

10. Unrelated data sharing one array

`c` has shapes in rows 0 to 18, the move lists in rows 19 to 28 and more move
lists in rows 29 to 33, so `c[l][T][0]` is an x coordinate in one place and a
piece number in another. `g[0]` counts how often each color is used, `g[1]`
turns a color slot into a color and `g[2]` turns a color into a color slot.
`h[0]` is the game and `h[1]` remembers what is on the screen. A single name
gives no hint of which meaning is in use.

11. Arithmetic in place of comparisons

C treats any non-zero value as true, so subtraction and addition can act as
tests. `h[0][T][e]-h[1][T][e]` means "these two are different", `r-R` means
"this is not the first row to be built" and `G[T][e]+i` means "this is not -1". The loop
`while(R+i)` means "while `R` is not -1". No `!=` is needed anywhere.

12. Decrement used as initialization

Global variables start at zero. `R--`, `G[T][e]--` and `h[T][e][t]--` make them
-1, the value for "empty". The code looks like it is counting down, but it runs
exactly once per variable to set a start value.

13. A variable that is really a constant

`g[i=1][T]=7-T` hides an assignment inside an array index. `i` is never changed
after this, so `K-i`, `c[V][T][i]` and `T>i` are all just using the number 1. It
looks like a loop counter, and `c[V][T][i]` looks like a loop over a table when
it is reading the second field. In the same way, `T=0; ... h[T][e][t]--` only
uses `T` to write `h[0]`.

14. Names that spell words

The names were picked to spell things instead of to describe anything:
`T,e,t,r,i,s` is "Tetris", `E,L,O,R,G` is "ELORG" and `D,V,K` is "DVK". That
produces phrases like `[T][e][t]--`, and the letters `l` and `O` look a lot like
the digits 1 and 0 in `l=0` and `O=0`.

15. Variables with several jobs

`L` and `O` are globals used by both functions. Inside `q` they are the falling
row and the "still falling" flag. `L` is then a loop counter when clearing rows.
Inside `main`, `O` is a position in the palette and later a column, and `L` is a
pixel and later a piece number. `s` is a list length and later a count of
unsupported squares. `V` starts as a column pair, is folded in half, and then
has 29 added to become a row of `c`.

16. One number doing two jobs

`O=T-L` looks like a meaningless subtraction of a color from a column. It works
only because of how the first three pieces are ordered. Number 0 has its lowest
square at its left edge, number 1 in the middle and number 2 at its right edge.
So the color slot of a pixel is also the piece number and also the distance back
to the left edge of that piece. The same `L` and `O` are then passed to `q`, and
are overwritten for every later piece with `if(K)`, so the first drop of a list
is hidden inside the loop that performs the others.

17. Mirroring with index tricks

Only half of the move lists are stored. `p?20-e:e` flips a column, `e=O>9` and
`18-O` fold a position in the right half onto the left, and `V=9-V` does the same
for the platform lists. `K&&e` is used as the flip flag so that it is off for the
first drop only. Each of these looks like unrelated arithmetic.

18. A bit trick as a range test

`(e&7)==e` keeps only the lowest three bits and checks that nothing changed. That
is only true for 0 to 7, so it is a hidden way to write `e>=0 && e<=7`. Together
with `d[T]-48`, it tests for a digit from `'0'` to `'7'` without mentioning a
character. `d[T]=0` is on the same line, and it blanks the line so that a shorter
line that follows does not pick up old pixels.

19. Unexplained escape codes and numbers

Terminal commands such as `"\33[2J\33[?25l"` (clear the screen and hide the
cursor) are written as raw octal escapes, and `"\33[%d;%df\33[4%d" "m  "` is
split in two. `v(e+4+e,T+2,...)` turns a square into a screen position with
no explanation. `42` is the height of the field, but it is also the size of the
line buffer and of arrays that have nothing to do with the field, so the number
tells the reader nothing.

20. Layout that hides the structure

The source is drawn as a Tetris field in the middle of a game, with a `T` piece
falling into a block of text. Lines are cut wherever the shape needs it, even
in the middle of an expression like `h[\n0][T][e]` or between `int l` and
`,int k`. Line breaks therefore say nothing about where a statement ends, and
nothing is indented to show which code is inside which loop.
