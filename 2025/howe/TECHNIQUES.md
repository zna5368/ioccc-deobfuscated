# 2025/howe

Difficulty: Easy

1. Shaped source with filler

The code is drawn as a picture (an invader) using odd indentation, blank
lines, line breaks in the middle of expressions such as `g\n=28` and
`T[1\n].c_cc`, and filler that does nothing: empty statements `;;`, empty
comments `/***/` and `//` comments with nothing after them.  The compiler
ignores all of it, so simple.c just uses normal formatting.

2. One letter names

Every variable has a one letter name: `v` is the row of invaders, `g` is
where the screen starts in that row, `a` is the aim, `b` is the ammo, `d` is
the shield damage, `p` is the score, `z` is the kill count, `k` counts clock
ticks and `u` is the glyph table.  simple.c gives them names such as `field`,
`view`, `aim`, `ammo`, `shield_damage`, `score`, `kills`, `ticks` and
`glyphs`.

3. Fixed width glyph table

`u` looks like one long string, but it is a table where every entry is 4
bytes.  The full width digits `０`..`９` are 3 bytes of UTF-8 and are padded
with `\0` to 4 bytes, the emoji `👾` and `🛸` are already 4 bytes, and the
last full width space gets its 4th byte from the NUL at the end of the
string.  `u+a*4` finds entry number `a` and `"%.4s"` prints at most 4 bytes,
stopping early at the padding NUL.  simple.c names the stride `GLYPH_SIZE`.

4. Character escapes used as numbers

The numbers 11 and 12 are written as the characters `'\v'` (vertical tab)
and `'\f'` (form feed).  `'\f'` is the glyph number of an empty cell and the
number of aim choices (`a%='\f'`), and `'\v'` is the glyph number of the UFO
and, with `v[i]%='\v'`, turns a random byte into 0..10 (a digit or the
veteran).  `'\003'` is the code that ^C sends.  simple.c uses the `enum`
names `BLANK`, `UFO`, `VETERAN` and `TARGETS`.

5. Pointer arithmetic on a string literal

`"|||"+d` skips the first `d` characters of the string, so the shield is
drawn as `|||`, `||`, `|` or nothing as the damage `d` goes up.  The same
idea is used for the glyph table above.

6. Return values reused as arguments

`memset` returns the address it filled, so `getentropy(memset(v,'\f',28)+6,16)`
blanks the whole row and then fills 16 cells starting at `v+6` with random
bytes in one expression.  `c=fflush(stdout)` uses the fact that `fflush`
returns 0 to clear the key variable, so if `read` times out with no key the
`switch` sees 0 and does nothing.  simple.c splits these into separate
statements with comments.

7. Side effects hidden in a short circuit expression

`d+=!(++k%(20-z%64/16*PACE))&&v[g++]^'\f';` does a lot in one line.  `++k`
counts a tick.  `!(x%y)` is 1 only when the tick count is a multiple of the
period.  Because `&&` stops early, `g++` (moving the invaders one step) only
happens on those ticks.  `^` is used as "not equal" to test whether the cell
that left the screen held an invader, and the 0 or 1 result is added to the
damage `d`.  simple.c writes this as two nested `if` statements.

8. Exclusive or used as a comparison

`c^'\003'` in the loop condition is non zero when `c` is not ^C, so it works
like `c!='\003'`.  The same trick is used in item 7.

9. Terminal settings used as the game clock

There is no timer.  The terminal is put in raw mode with `VMIN=0` and
`VTIME=1`, so `read` waits at most a tenth of a second for a key.  Every
pass through the loop is therefore one tick, and the invaders move every 20,
17, 14 or 11 ticks.  `ISIG` is turned off so ^C arrives as a normal key
instead of killing the program, which lets the program print the score and
restore the terminal.  The final `return tcsetattr(0,0,T)` both restores
the saved settings and uses their result as the exit status.

10. Nested conditional expression for scoring

`p+=w<10?w+(i+1)*10:w==10?150:300;` chains two `?:` operators to give a
digit its value plus 10 points for each cell of distance from the base,
the veteran 150 and the UFO 300.  simple.c uses an `if`/`else if`/`else`.
