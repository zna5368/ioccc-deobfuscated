# 2020/ferguson1

Difficulty: Medium

1. Digraphs and `%:` for `#`

The code writes `<:` and `:>` for `[` and `]`, and `<%` and `%>` for `{` and
`}`. For example `static k p <:5:>= <% 1 %>` is just `static unsigned long
p[5] = {1}`. Inside macros `%:x` is the same as `#x` (turn `x` into a string)
and `%:%:` is `##` (glue two words together). So `Z(YOU WIN!)` becomes
`"YOU WIN!"` and `m(PAIR(L))` becomes `COLOR_PAIR(L)`.

2. Macros that hide common code

`F(x)` reads an environment variable and the macro `d` finishes the statement
with `=strtoul(z,NULL,0));`, so `F(GROW)N d` means "if `GROW` is set, store it
in `N`". `q` is `mvaddch(`, `v` is `stdscr` and `J` is the whole status line
format string with its arguments. `d` is `#undef`ed half way through the file
so it can not be used later. `simple.c` uses small functions like
`read_setting` and writes the calls out in full.

3. Macros defined on the command line

The `Makefile` passes `-D'f(a,b)'=...` which is a "smaller of two values"
macro, and the colours `WALL`, `HEAD`, `BODY`, `WB`, `HB` and `BS`. Someone
reading only `prog.c` can not see what `f` does. `simple.c` writes the minimum
out with `?:` and keeps the colour names so the colour scripts still work.

4. One-letter names

Every variable is one letter and the names are misleading: `Y` is the column of
the head and `X` is the row, while the status line prints them the other way
round. `A` is the snake length, `g` the maximum length, `c` the bugs eaten,
`e` and `n` are the snake columns and rows. `simple.c` uses names like
`head_x`, `size`, `max_size` and `snake_x`.

5. Many variables packed into arrays and reached through pointers

`p[5]` holds the walls setting, the evade counter, the shed counter, the shed
interval and the cannibal setting. The array `j[8]` holds the wait time,
screen sizes, bug position and direction. Pointers into it are made with
`y = j+3`, `U = y+2`, `V = U+1` and `D = 1+V`, so the same value can be
written as `y[3]`, `*V` or `j[6]`. The pointers are even assigned in the middle
of the environment variable code (`U=y+2;`) so they are easy to miss.
`simple.c` gives every value its own named variable.

6. Swapped array indexing

C allows `a[i]` to be written as `i[a]` because both mean `*(a+i)`. The code
uses `1[y]`, `4[p]`, `3[p]` and `1[V]` mixed with the normal form, and
negative indexes like `y[-2]` and `y[-3]` that reach back to before `y`.

7. Calculated constants

`l` is always 4 and is used to make the direction numbers: `l-3` is 1 (left),
`l/2` is 2 (right), `l-1` is 3 (up) and `l` is 4 (down). The index `l/l` and
`l/4` both mean 1. `simple.c` uses an `enum` with `LEFT`, `RIGHT`, `UP` and
`DOWN`.

8. Head character chosen with a bit mask

The head is drawn with `s[*D+2&(1<<3)-!Q]` where `s` is `"; +<>^v"`. While
playing the mask is 7 and it picks `+`, `<`, `>`, `^` or `v` from the
direction. If the game is over the mask is 8 and the result is index 0, `;`.
`simple.c` writes this as `game_over ? ';' : "+<>^v"[direction]`.

9. `&&` and `||` used instead of `if`

Statements such as `!h&&endwin();`, `L&&puts(L);`, `z&&(g=...)` and
`(!P && (P=1))||(P=0)` only run the right side when the left side is true (or
false for `||`). The last one is just a toggle, `paused = !paused`.

10. Function pointer table

`void (*H[])(a)={b,S,B}` stores the three game functions, and they are called
as `(*H)(0)`, `H[1](G)` and `H[2](0)` so it is not obvious which function is
used. `simple.c` calls `show_status`, `move_snake` and `place_bug` directly.

11. `goto` instead of loops

The main game loop is the label `x:` with `goto x;` at the end and `goto y;`
to quit, and the bug placement retries with another label also called `x:`
(labels in different functions can share a name). The variable `x` also has
the same name. `simple.c` uses a `while` loop, `break`, `continue` and a
`do`/`while` loop.

12. Reused variables and odd flags

`x` is set to the screen height before reading a key and cleared by the
`default:` case, so it acts as "an arrow key was pressed". Then
`P=!*y` is used to clear the pause flag because the height is never 0. The
`q` key falls through into the space case and into `default:` so quitting
also toggles pause. `simple.c` uses a `key_handled` flag and sets
`paused = 0` directly.

13. Layout

The source is shaped into columns that look like an ASCII picture, with
statements split across lines and tabs in the middle of words, so the normal
shape of the code can not be seen.
