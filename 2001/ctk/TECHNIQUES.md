# 2001/ctk

Difficulty: Medium

1. Single-letter global names

Everything is called `x`, `w`, `r`, `l`, `T`, `d`, `p`, `o`, `k`, `g`, `G`,
`X`, `P`, `f` and `s`, and the functions are `u` and `e`. You have to work out
from how they are used that, for example, `x` holds the center column of each
road row, `w` holds the road's half-width, `g` is the current speed and `G` is
the target speed for the gear. `simple.c` gives them names like `road_center`
and `speed`.

2. Arrays disguised as strings

`x[]="(((((...."` and `w[]="\b\b\b..."` look like text, but they are really
arrays of 22 small numbers. `'('` is 40 (the middle of an 80-column screen) and
`'\b'` is 8 (the starting half-width of the road). The NUL character at the end
of each string also marks where the array stops, and the scrolling macro uses
it for that.

3. Characters written as numbers

The edge characters are written `{92,124,47}`, which are `'\\'`, `'|'` and
`'/'`. The key cases are `case 113` (`'q'`), `case 91` (`'['`), `case 44`
(`','`), `case 49` (`'1'`) and so on. `*V[1]!=113` checks whether the first
argument starts with `q`. `n&7936` is really `n&0x1F00`, and `signal(14,...)`
is `SIGALRM`.

4. Strings read as integers

`*(int*)getenv("TERM")` treats the first four bytes of `$TERM` as one integer
and compares it with `0x756E696C` and `0x6C696E75`. Those are the letters
`"linu"` stored in little-endian and big-endian byte order. It is a sneaky way
to check whether the program runs on the Linux console without calling a string
function.

5. Macros that hide code

`m(b)` is a whole loop that moves every element of an array down by one place
(it scrolls the road history). `h(u)` picks a gear and sets the keyboard LEDs.
`c(n,s)` expands to `case n:s;continue`. It is used inside other case labels,
so `case 91:case 98:c(44,k=-1);` really means three keys that all mean "steer
left".

6. `&&` and `||` used in place of `if`

Most of the logic is written as expressions such as
`(n=rand())&255||--*w||++*w;`, which means "if the low 8 bits are zero, shrink
the road, and if it reaches zero, grow it back to 1". Another one is
`!d&&--*x<=*w&&(++*x,++d)||d==2&&++*x+*w>79&&(--*x,--d);`, which bends the road
left or right and goes straight again when it hits the screen edge. These work
because `&&` and `||` only evaluate their right side when they need to.
`f&&putchar(7)` is just "if on the Linux console, beep".

7. Side effects hidden inside other expressions

The car moves with `p+=k`, which is buried in the argument list of the big
`printf`. The tree check `!(**P&&P++||n&7936)` also moves the tree pointer
forward as part of a condition. In `g+=((g<G)<<1)-1`, the comparison result
(0 or 1) is turned into -1 or +1 to move the speed towards the target.

8. Signal handler used as the game loop

The game does not have a normal loop for drawing. `main` calls `u(14)` once.
After that, `u` is the `SIGALRM` handler and `setitimer` calls it again and
again. `signal(i,u)` at the end puts the handler back. The speed of the game is
set by changing the timer interval, so `main` only reads keys.

9. Terminal escape sequences

The drawing is done with ANSI and Linux console escapes in hard-to-read
strings: `\e[H\e[L` moves the cursor home and inserts a line so the whole
screen scrolls down, `\e[;%uH` positions the cursor, `\e[%uq` sets the keyboard
LEDs, `\e[10;%u]` sets the bell pitch for the engine sound, and `\ec` resets
the terminal. The tree shape `"%\\|/%"`, `" %%%"` is drawn from the top down,
but because the screen scrolls down it appears upside down from how it is
listed.

10. Dense layout

The whole program is squeezed into a block of text with no indentation.
Declarations, functions and statements run into each other across line breaks,
and there is an `e();` after the endless `for(;;)` that can never run.
