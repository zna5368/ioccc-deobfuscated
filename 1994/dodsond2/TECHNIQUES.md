# 1994/dodsond2

Difficulty: Medium

1. Macros that rename keywords

Almost every C keyword is hidden behind a single capital letter: `F` is
`while`, `G` is `int`, `H` is `return`, `I` is `for`, `K` is `if` and `L` is
`else`. Library calls are hidden the same way: `E` is `printf(` (note the
missing closing parenthesis, the call is finished in the code), `M` is
`isdigit` and `P` is `getchar()`. Reading `K(v=='q')A;` you have to expand
three macros in your head to see `if (v == 'q') { print score; exit }`.

2. Macros that hide whole statements

`A` expands to a full block `{ printf("Your score is %d\n",j); exit(0); }`,
which is the "game over" code used in seven places. `B(x,y)` is a swap of two
values when `x<y`, used three times in a row to sort the three passageways of
a room. `C` is a complete `do { p = random() % N; } while (c[p] || d[p]);`
loop that picks an empty room, and `D` is `random()%N`, a random room number.

3. Array access hidden in a macro

`J(r,j)` expands to `a[r][j]`, the room that passageway `j` of room `r` leads
to. The macro parameter names `r` and `j` are the same as a function and a
global variable, which makes the code even more confusing to read.

4. Magic numbers in a macro

`N` is 30, the number of rooms. Other values are derived from it: `N*2` (60)
is used as "infinitely far from the Wumpus", and the loop
`for(o=0; o<N; o+=8)` runs four times, placing four bats and four pits.

5. Meaningless single letter names

All game state is in global arrays named `a` to `g`: `a` is the cave map,
`b` is the distance of each room from the Wumpus, `c` marks bats, `d` marks
pits, `e` counts free passageways while building the cave, `f` marks arrows
and `g` marks visited rooms. Scalars `i`, `j`, `k`, `l`, `X` and `Y` hold the
arrow count, score, robber room, stolen arrows, arrows shot and arrows found.
Functions are also single letters (`m`, `n`, `q`, `r`, `s`, `t`, `w`), and
local variables inside `n()` are named `r` and `n`, the same as two functions.

6. The Wumpus is a distance, not a position

There is no variable holding the Wumpus' room. Instead `w()` fills `b[]` with
the number of steps from every room to the Wumpus by walking recursively
through the passageways. The Wumpus is in the room where `b[r]==0`, and
"You smell a wumpus" is printed when `b[r]<3`. Moving the Wumpus after a
missed shot means resetting every distance to 60 and calling `w()` from a new
random room.

7. Random recursive cave building with backtracking

`r()` digs passageways from a room to random rooms that still have free
slots, then recursively continues from the room it connected to. If it gets
stuck after 1000 tries it returns 0 and the caller undoes the last tunnel. The
variable `u` makes it prefer rooms with many free slots and relaxes that rule
every 100 tries. `s()` sorts each room's passageways and reports if any room
still has a missing one (`-1`), in which case the whole cave is rebuilt. The
`do { ... s(); } while (s());` loop calls the sort function twice, which is
redundant.

8. Old style function definitions and implicit int

Functions are declared without return types (`m(){...}`) and in the K&R
style `w(o,d)G o,d;{...}`, so it is hard to see what the arguments are.
Functions like `exit`, `random` and `getpid` are used without including the
headers that declare them.

9. Compressed submission

As explained in the README.md, the original entry was submitted as a gzip
file to get around the size limit, which allowed the source to be longer than
the rules intended.
