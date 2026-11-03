# 1987/lievaart

Difficulty: Hard

1. Macros that hide keywords and library calls

`#define Y return`, `#define R for`, `#define e while`, `#define I printf`
and `#define l int` replace keywords and `printf` with single letters
(`#define D define` is there too, but is never used). Code like
`R(;t<1100;t+=100)`, `e(lv>0)` and `Y w;` only makes sense once you translate
each letter back to `for`, `while` and `return`.

2. Macros that hide whole loops

`#define H(a,b)R(a=b+11;a<b+89;a++)` is a loop over every square of a board,
so `H(z,0)` walks through the squares 11 to 88. `#define C y=v+111;H(x,v)*y++= *x`
uses `H` inside another macro to copy one board into the next one. Because
`C` contains a whole statement and a loop, `if(!g){g=1;C;}` hides a loop
inside what looks like a short `if`. In simple.c this is the function
`copy_to_next_board()`.

3. Single letter names

Almost every variable and function has one or two letters: `S` is the search,
`GZ` makes a move, `V` holds the boards, `u` is the search depth, `lv` counts
empty squares, `bz` is the chosen move, `r`, `h` and `ih` are the direction,
corner and next-to-corner tables, and the search uses `c`, `bw`, `w`, `z`,
`zb` and `q`. simple.c renames them to `search`, `play_move`, `boards`,
`search_depth`, `empty_squares`, `best_move`, `directions`, `corners`,
`x_squares`, `move_count`, `best` and so on.

4. Global variables reused as scratch space

`m` holds the human's move in `main()`, but the scoring code in `S()` also
uses it (and `t`, which otherwise just catches the return value of `scanf`)
to remember a square it just looked at: `(m=v[h[i]])==f?...:(t=v[ih[i]])==f?...`.
The global pointers `x` and `y` are only used by the copy macro `C`. A reader
has to check that reusing them never breaks anything. simple.c uses local
variables instead.

5. Many boards packed into one array

`V[1100]` is eleven 10x10 boards placed one after another. Each level of the
search works on its own board: `n=v+100` is "the next board" and the move
function `GZ(v,z,f,o)` writes its result to `v+o`, so `o=100` plays the move
into the next board while `o=0` changes the board in place. The magic numbers
100, 111 and 1100 are the only hint of this layout. simple.c names them
`BOARD_SIZE` and `BOARD_COUNT` and explains them in a comment.

6. Board with a border instead of bounds checks

Squares are numbered `row*10+column`, so moving in a direction is just adding
one of `-1,-11,-10,-9,1,11,10,9`. The edge rows and columns are filled with
the value 3 by `m<11||m>88||(m+1)%10<2?3:0`, so the loop `e(*j==q)j+=h;` stops
at the edge without ever checking it. `(m+1)%10<2` is a tricky way of saying
"the column is 0 or 9".

7. Magic numbers

The players are `1` and `2` so `3-f` gives the other player, `3` is the
border, `99` means pass, `0` is printed when the computer passes, `9000` is
"infinity", `300` and `50` are the corner scores, `8003` is a winning score,
`60` is the number of empty squares at the start and `c<<3` is a hidden
multiplication by 8. simple.c gives each of these a name.

8. Long nested conditional expression

The position score is one line:
`w+=(m=v[h[i]])==f?300:m==q?-300:(t=v[ih[i]])==f?-50:t==q?50:0;`. It is a
chain of four `?:` operators with assignments inside the conditions. In
simple.c it is an `if`/`else if` chain inside `evaluate()`.

9. Negamax alpha-beta search written as a tiny function

`S()` searches the game tree, flipping the sign of the result every level
(`w= -S(d+1,n,q,-b,-bw)`) so the same code works for both players. Alpha and
beta are swapped and negated at the same time, the starting best value is
picked with `d<u-1?a:-9000`, and the result is returned with
`d>=u-1?bw+(c<<3):bw`. The chosen move is passed back through the global `bz`
instead of a return value. simple.c splits these steps up and comments them.

10. Old style (K&R) functions, implicit int and missing headers

`S(d,v,f,a,b)l*v;{` and `GZ(v,z,f,o)l*v;{` declare only one parameter type
after the parameter list and leave the rest (and the return types) to default
to `int`. `isdigit` and `atoi` are called without including any header.

11. goto loop and comma operator

The level question is a backwards `goto x;` instead of a loop, and
`I("move: %d\n",(lv-=GZ(V,bz,1,0),bz))` plays the computer's move inside the
argument list of `printf`, using the comma operator so the value printed is
`bz`. simple.c uses a `do`/`while` loop and separate statements.

12. Dense formatting

The program is squeezed into a block of lines with no indentation, and lines
break in the middle of statements (the name `GZ` of the move function sits at
the end of one line and its parameter list `(v,z,f,o)` at the start of the
next).
