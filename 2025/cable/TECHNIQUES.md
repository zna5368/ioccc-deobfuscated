# 2025/cable

Difficulty: hard

1. Macro that hides a complex expression

`#define o s[1&s[t=e++]?s[t]/4:t]/4,t` fetches one operand of the current
instruction (with the optional indirect lookup) and ends in `,t`. Every `o` in
`_=o=y=o=o` turns into this whole expression, so the short line actually reads
three operands. In `simple.c` it is the `fetch_operand()` function.

2. All the work done inside the `for` header

The loop's init part creates the window, the condition part fetches the three
operands, and the "increment" part runs the whole instruction (subtract, branch,
interrupt, keyboard). What looks like the loop body, `_^64||timespec_get(...)`,
is just the clock update. Because the condition ends in `,t`, the loop stops
when the third operand (the jump target) is 0, which is how the machine halts.

3. Nested conditional (`?:`) operators instead of `if` statements

`~_?~y?e=1>(...)?t:*s&&++b>8e5?...:e:e:SDL_PollEvent(V)?...:0` is five `if`/`else`
decisions packed into one expression. You have to match every `?` with its `:`
to see which branch belongs to which test. Branches that do nothing are written
as `:e` (assign `e` to itself) or `:0`.

4. Short circuit operators used as `if`

`_^64||timespec_get(s+_,1)` only calls `timespec_get` when `_^64` is 0, which means
`_ == 64`. `*s&&++b>8e5` only counts an instruction when `s[0]` (the interrupt
vector) is not 0.

5. Unusual ways to write comparisons

`~_` is 0 only when `_` is -1, so `~_?` means "if `_ != -1`". `1>(s[y]-=_[s])` means
"the result is less than or equal to 0". `_^64` is non-zero when `_` is not 64.

6. Reversed array subscripts

In C `a[b]` is the same as `b[a]`, so `_[s]` is `s[_]`, `6[s]` is `s[6]`, `3[g]` is
`g[3]`, `6[V]` is `V[6]` and `y[s]` is `s[y]`. This makes indexes look like arrays.

7. Implicit `int` and untyped declarations

`b,y,t,e,s[3<<27],V[32],*w,**g;` declares everything as `int` (or pointers to
`int`) without writing a type, and `main(_)` has an implicit `int` return type.
The window, the surface and the SDL event are all stored in `int` variables.

8. Struct members accessed by position

The window surface is stored in `int **g`, so `3[g]` reads the fourth pointer
sized slot of the `SDL_Surface` struct, which happens to be `pixels`. The event is
an `int V[32]` array, so `*V` is the event type and `6[V]` is the key scancode.
`simple.c` uses `surface->pixels`, `event.type` and `event.key.scancode`.

9. Reused variables

`_` is `argc`, reused as the first operand. `b` is 0 at start and is used as the
file descriptor for standard input in `read(b,...)`, then becomes the instruction
counter for the timer interrupt, and finally the index where the return address
is saved. `t` is used both as a temporary address while fetching and as the jump
target.

10. Return values used as numbers

`!read(...)` turns the number of bytes read into the window flags (fullscreen only
when the input was empty). `SDL_UpdateWindowSurface` returns `true` (1), which is
used both to reset the counter `b` and as the index `s[1]` where the program
counter is saved before jumping to the interrupt handler.

11. Comma operator inside an array subscript

`s[memcpy(...),b=SDL_UpdateWindowSurface(w)]=4*e,*s/4` copies the framebuffer,
updates the window, saves the program counter and finally gives the new program
counter, all inside one subscript and one comma list.

12. Encoded and misleading constants

`'frog'` is a multi-character constant equal to `0x66726f67`, used as a huge read
size. `3<<27` is the number of memory words (1.5 GB), `25<<16` is the screen size
in bytes (800*512*4), `8e5` is 800000 instructions per time slice, and
`1537-2**V` is `1537-2*(*V)`, not a power, giving +1 for a key press (type 0x300)
and -1 for a key release (type 0x301). The `1` in `timespec_get(s+_,1)` is
`TIME_UTC`.

13. Memory mapped addresses with no names

Special words of memory such as `s[0]` (interrupt vector), `s[1]` (saved program
counter), `s[6]` (framebuffer location), `s[64]` (real time clock) and operand -1
(keyboard input) are only plain numbers. `simple.c` gives each one a name.
