# 2011/eastman

Difficulty: Medium

1. Source code shaped like a picture

The code is laid out as a round ball to match the bouncing ball it draws. The
line breaks fall in the middle of expressions and even keywords like `ioctl`
and `sqrt`, so the layout tells you nothing about how the code is organized.

2. Alphabetical single letter variable names

Each variable is named after the next letter of the alphabet in the order it
is declared (`a`, `b`, `c` ... `p`). So the names say nothing about what the
variables hold. For example `d` and `e` are the terminal's row and column
counts, `l` and `m` are the ball's position, `n` and `o` are its speed, and
`k` and `p` are how far it has spun and how fast it spins.

3. Array used in place of a structure

`ioctl(0,TIOCGWINSZ,&a)` should fill in a `struct winsize` and then read its
`ws_row` and `ws_col` fields. Instead it uses `short a[4]`, which has the same
memory layout, and reads the sizes as `*a` (the first element) and `a[1]`. The
clear field names are replaced by array positions.

4. Whole raytracer in one nested ternary expression

The colour of each character is picked by a single `printf` argument made of
many nested `? :` operators. It checks whether the point hits the ball
(`g*g>1-h*h`), then whether it is on the floor, the side borders, the shadow or
the wall, and if it hits the ball it works out the checkerboard square. In
simple.c this is split into `if` statements and the functions
`background_colour` and `ball_colour`.

5. Work hidden in the loop condition

The inner `for` loop calculates the point's position (`h`, `g`) and the tilted
axis height (`f`) inside its condition, joined with commas, before the actual
test `c<d`. A comma expression runs each part in turn and keeps only the last
value, so the calculations happen on every pass but look like part of the
test.

6. Two nested loops merged into one

Instead of one loop over rows and one over columns, a single loop does both:
`c+=(b=++b%e)==0` moves the column `b` forward and wraps it back to 0 at the
edge, and the comparison `==0` gives 1 exactly when it wraps, which adds one to
the row `c`.

7. Infinite loop disguised as a real condition

`while(printf(...),!usleep(79383))` looks like it is testing something. But
`usleep` returns 0 when it succeeds, so `!usleep(...)` is always 1, and the
loop runs forever. It also hides the screen clearing `printf` and the frame
delay inside the condition. The odd delay `79383` is used in place of a round
number just to look strange.

8. Assignments used as values

Values are assigned in the middle of other expressions, for example
`f=-.3*(g=(l-b)/i)+.954*h` sets `g` and uses it at the same time, and
`n=(l+=n)<i||l>e-i?p=-p,-n:n` moves the ball, checks for hitting a side, flips
the spin and flips the speed all in one line. In simple.c these become
separate statements and an `if`.

9. Unexplained magic numbers

The numbers `.3` and `.954` are the sine and cosine of the ball's tilt, `40`,
`47`, `100`, `101` and `107` are ANSI colour codes, and `g*(g+.6)+.09+h*h<1` is
the circle `(g+.3)^2+h^2<1` multiplied out so it is not recognizable as the
shadow of the ball. simple.c gives these names with `#define` and comments.

10. ANSI escape codes

Strings such as `"\x1b[H\x1B[?25l"` (move cursor home and hide it) and
`"\x1B[%dm "` (print a space with a background colour) are hard to read raw
escape sequences. Mixing `\x1b` and `\x1B` adds to the confusion even though
they are the same character.
