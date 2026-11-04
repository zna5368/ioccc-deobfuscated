# 2004/schnitzi

Difficulty: Medium

1. Code laid out in two columns

Every line starts with a token padded to ten characters (`printf    (`,
`do do     c[b]...`, `while     (`), so the left column looks like a list of
keywords and the right column like their arguments. Lines are broken in odd
places, such as between `return` and `!` or between `(` and `int` in a cast,
which hides where each statement really begins and ends.

2. Single-letter and underscore names

All variables are single letters (`a` is the screen, `c` the sprite pictures,
`o`/`p` the start and end times, `u`/`v` the position, `w`/`z` the speed,
`x`/`y` the size, `q` the view count, `n` the current view), and the bounds
check function is called `_`. Arrays named `x` and `y` hold widths and
heights, not coordinates, which misleads the reader.

3. Macros hiding literals and functions

`l` and `m` are macros for a 50-character line of `=` and a 50-character
line of spaces. `gets(x)` is redefined to `fgets((x),100,stdin)`, so the code
looks like it calls the unsafe `gets` but is actually reading at most 100
characters.

4. Side effects in extra function arguments

Assignments are tucked into unused arguments of `printf`, `sprintf` and
`sscanf`. For example `printf("%dG0",j+1,e=0,g+=4)` both prints the command
and resets the column and counts output characters, and the `sscanf` call
ends with `j=n[b]=i=0` to reset counters. A `sprintf` is even nested as an
extra argument of another `sprintf` so that both screen buffers are filled
in one statement.

5. Nested do-while loops instead of for loops

All loops are `do ... while` loops, often stacked as `do do ... while ...
while`. The loop counters are reset inside the `while` condition, for example
`while (++j<20+(k=0))` resets `k` before the next row and `while (++i<20||(i=0))`
resets `i` once the loop is finished.

6. Work hidden in loop conditions

Some conditions do the real work of the program. After drawing a sprite,
`while (++j<y[i]+(k=0)||((++n[i]-q[i]||(n[i]=0)),u[i]+=w[i],v[i]+=z[i],0))`
advances to the next view, wraps the view number and moves the sprite, then
uses `,0` to end the loop. The sprite reading loop uses `|| !++b` to count
the sprite when all its views are read, and the outer reading loop uses
`|| (i=t=e=f=0)` to reset variables when the input ends.

7. Short-circuit operators instead of if statements

The cursor movement code is written as `f-j&&printf(...)` and
`k&&e<k&&k-e>1&&printf(...)`. The `&&` operator only runs the `printf` when
the conditions before it are true, so each line acts like an `if` statement.

8. Assigning a boolean to terminate strings

`c[b][i][j][x[b]]=!gets(c[b][i][j])` reads a row and then stores `!result`
at the sprite width. On a successful read this stores `0`, cutting the row
(and its newline) to the right length.

9. Hidden vi commands in strings

The output string `"i%s\033Yppppppppppppppppppp1G0"` is a series of vi
keystrokes: `i` to insert, the `=` line, ESC (`\033`), `Y` to yank the line,
nineteen `p` to paste it, then `1G0` to go to the top left. Other strings like
`"%dG0"`, `"0%d "` and `"r%c"` move the cursor and replace characters.

10. Double buffering with `h=1-h`

The two screen buffers in `a` are swapped by flipping `h` between 0 and 1.
Only characters that differ between `a[h]` and `a[1-h]` are written, and
the previous buffer starts as all `=` so that it matches what vi shows.

11. Timing by counting characters

The variable `g` counts how many characters were printed in a frame and the
loop `do printf("1G0",g+=3,...) while(g<s)` repeats a harmless "go home"
command until the time factor is reached, which slows down the animation.

12. Return value trick

`return !puts("")` prints the final newline and usually returns 0, because
`puts` normally returns a positive number on success and `!` turns any
non-zero number into 0.
