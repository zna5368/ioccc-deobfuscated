# 2006/grothe

Difficulty: Easy

1. Meaningless identifiers built from underscores

Names like `_`, `__`, `_x`, `__x`, `___`, `_X_`, `_x_` and `_l` look almost
the same, so it is hard to keep track of which is which. For example `___` is
really `argv`, `__x` is `argc`, `_` counts pixel clock ticks, and `_x` is the
carrier rate. The globals `b`, `d`, `e`, `f`, `g`, `i`, `j`, `k` and the
function `l` are single letters that hide that they are the display, window,
graphics context, screen height and width, horizontal total, pixel clock,
carrier frequency and tone-drawing function.

2. Misleading variable roles

In `l()` the variable named `x` walks down the rows of the screen and `__`
walks across the columns, which is the opposite of what the names `x`/`y`
normally suggest. `y` is not a coordinate at all but the audio frequency
divided by the pixel clock.

3. Mixed number bases

The same kind of value is written in octal, hex and decimal so that patterns
are hidden. Zero is written `00` and `0x0`, the array size `0377` is 255, and
the sleep time `03563262400` is octal for 500000000 nanoseconds (half a
second). The tone frequencies mix all three: `01763` is 1011, `0x4b9` is
1209, `948` is plain decimal.

4. Character codes instead of character literals

The tone table is indexed by the ASCII codes of keypad keys written as
numbers: `061` is `'1'`, `0x32` is `'2'`, `043` is `'#'`, `42` is `'*'`,
`48` is `'0'`. Without an ASCII table it is not clear that the table maps
phone keys to frequencies.

5. Reversed array subscripts

`52[_X_]`, `065[_X_]` and `55[_X_]` mean the same as `_X_[52]` etc., because
`a[b]` is defined as `*(a + b)` and addition does not care about order.

6. Comma operator chains

Many statements are joined with commas instead of semicolons, such as the
usage message and `exit()` in one expression after `if`, the four argument
assignments, and the window setup. This lets several actions hide behind one
`if` or inside one block, and makes code formatters produce odd layouts.

7. Redundant expressions and casts

Argument indices are written `(1+1)` and `(1+1+1)`, constants as `02.0`, and
every X11 call is wrapped in a `(void)` cast that does nothing but add noise.

8. Hidden initialization with `static` and nested blocks

The tone table `_X_` and the `timespec` `_l` are declared `static` inside
extra `{ }` blocks so they start out zero without any visible
initialization. That is how every non-keypad character gets a frequency of 0
(silence) and how `tv_sec` becomes 0 without being set. The nested blocks also
hide where variables such as the `XSetWindowAttributes` live.

9. Shape formatting

The source is squeezed into a block shape with statements broken at odd
places, so the structure of loops and functions is hard to see.

10. Hidden pixel-clock timing algorithm

The tick counter `_` is advanced once per drawn pixel and then by
`i - __` at the end of each row, so it always equals `row * horizontal_total`
at the start of a row. Checking whether `tick * rate` rounds to an even or odd
number makes a square wave: the audio rate decides whether a whole row is
drawn and the carrier rate decides which pixels in that row are white. This
trick is not explained anywhere and is only clear once the counter is traced
by hand.
