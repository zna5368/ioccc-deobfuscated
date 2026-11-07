# 2025/jhshrvdp

Difficulty: Medium

1. Shaped source with filler

The code is drawn as a little character, with the `#define` lines making
the head and the rest of the code squeezed into the body.  Words and
numbers are cut across lines with backslash-newline (`\` at the end of a
line continues it), e.g. `D[i]=!\n(((-~` and `mv\nprintw`, and `return endwin\n( )`
spreads one call over two lines.  Big empty comments `/* ... */` fill in
the shape and the comment `/* Q -- A S W D -- T-=3 argv[1] */` is only a
hint.  The compiler ignores all of this, so simple.c uses normal formatting.

2. One letter names and reused globals

Every variable is a one letter global: `D` is the map, `E` the enemies, `I`
the gold, `S` the stairs, `c` the floor, `h` the hit points, `v` the score,
`s` the seed, `q` how many levels were made, `t` the tick counter, `H` the
key and `p` the maximum hit points.  The same `i` and `j` are used by every
loop and are even changed as a side effect inside `N()`.  `p` (10) is used
for four different things: maximum hp, the regeneration interval, the
chase range and the top of the open corridor.  simple.c gives each value
its own name such as `map`, `enemy`, `gold`, `MAX_HP` and `CHASE_RANGE`.

3. Macros inside macros

The program is built from many one letter macros that call each other.
`B(y,x)` reads the screen character, `w(y,x)` uses `B` three times to ask
"can the player step here", `W` uses `w` and `B` again for enemies, `K`
uses `w` to move the player and `L` handles the stairs.  `J` and `F` hide
`for` loops, `X` and `Y` hide "pick a random floor cell" and `A` hides
three ncurses calls.  After expansion one line of enemy code becomes
hundreds of characters.  simple.c turns them into small functions such as
`can_walk()`, `enemy_can_walk()`, `draw()` and `first_entity()`.

4. Magic numbers in other bases and as character codes

Characters are written as plain numbers: `35` is `#`, `46` is `.`, `36` is
`$`, `94` is `^`, `64` is `@`, `62` is `>`, `60` is `<`, `119 97 115 100`
are `w a s d`, `116` is `t` and `0x71` is `q`.  Some numbers are in hex,
`0x64` is 100 and `0x63` is 99, and `1839` is the last cell before the
bottom row.  `360` is the sum of nine cells that means "5 or more walls".
Colors are plain numbers too (`6` is cyan, `3` yellow, `1` red).  simple.c
uses character constants and names like `WALL`, `GOLD` and `COLOR_CYAN`.

5. Caesar cipher string

The HUD text is stored with every byte moved down by 3, so it reads
`P\`lob7"ayem7...`.  At start up the loop `while(*(g+j)^0){*(g+j++)+=3;}`
adds 3 to each byte, giving
`Score:%d|hp:(%d/%d)|floor:%d|Argv{:%x:%02x:...:} `.  simple.c stores the
decoded text directly.

6. Digraphs

`<:` and `:>` mean `[` and `]`, and `<%` and `%>` mean `{` and `}`.  They
are used in `S[4:>`, `E[j:>`, `char g[ ]=<%"..."` and `&v%>`.

7. Bit tricks

`-~i` is `i+1` (`~i` is `-i-1`), and `(-~i&~1)%80==0` clears the lowest bit
so it is true for both column 0 and column 79, which tests "is this on the
left or right edge".  `^` is used as "not equal" (`H^119` is `H!=119`) and
`!(H^a)` means `H==a`.  `|` and `&` replace `||` and `&&` in places like
`G(i)|((rand()%0x64)<45)`, which also means `rand()` is always called.
simple.c writes these as `is_border()`, `!=` and `==`.

8. Reversed array indexing

`a[b]` is the same as `b[a]` in C, so `r[E]=r[I]=j` means `E[r]=I[r]=j`
and `k[b]=0` means `b[k]=0`.

9. Ternaries and commas instead of if statements

Most of the logic is written as `?:` and comma expressions.  Picking up
gold is `I[j]=...?++v,A(...),move(y,x),0:I[j]`, which adds to the score,
redraws the player and returns 0 to remove the gold.  The enemy step is
one huge nested `?:` that chooses chase or wander and then the direction.
`j==6?M():(void)0` is an `if`, and `return endwin(),+printf(...)` runs two
calls and returns the result of the last.  simple.c uses `if`, `switch`
and separate statements, and moves the enemy step into `enemy_step()`.

10. Booleans used as numbers

A step is "is the move allowed" times the size of the step, e.g.
`W(a,b,-1,0)*Q(...)*-l` is -80 (one row up) when the move is allowed and 0
when it is not.  The stairs use `S[0]=(i/l)*j` with `j=c<100`, so the
stairs are put at (0,0) when they should not exist, and `h+=h<p` adds 1
only while hp is below the maximum.

11. Label and goto instead of a loop

The main game loop is the label `ff:` at the top and `goto ff;` at the
end.  simple.c uses `for (;;)`.

12. Recursion instead of a loop

`N()` returns a random floor cell by calling itself again whenever the
random cell is not floor: `D[i=rand()%m]==46?i:N()`.  simple.c uses a
`do`/`while` loop in `random_floor_cell()`.

13. Hidden data inside a loop

The map smoothing loop also contains `r=i%600,r[E]=r[I]=j`, which has
nothing to do with smoothing.  It marks all 600 enemy and gold slots as
"present" so that they are placed again on the new level.

14. Filler that does nothing

The test `a+1!=E[0]/l` in the macro `Q` compares against the first two
enemies of floor 0 and has no real meaning.  The array of pointers is
called `cows`, `main` names its argument count `_`, there is an extra
empty statement `;;`, and `volatile void * race[]` holds a lava lamp
message that is never used.
