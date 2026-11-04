# 2004/vik1

Difficulty: Terrifying

1. Macros named after the Xlib functions they finish

The left column looks like a list of Xlib function names, but each name is
really the start of the code on the right. `#define J (K,` means
`XDrawLine J M,N,...)` reads `XDrawLine(K,M,N,...)`, with `K` the display.
`#define I ; for(` hides the starts of the `for` loops, `U` is the cast
`(struct timeval*)H`, `Z(a)` is `main(a,T)`, `X(a)` draws one line with `a` set
to `-` or `+`, and `V(a)` uses `#a` to build the `"Time: "` and `"High: "`
parts of the text from the bare words `Time` and `High`.

2. main() called recursively as a drawing routine

`main` checks its first argument with `--S&~1`. When the program starts, `S`
is 1 or 2 (the argument count), the options are read and the game loop runs.
Any other value is a colour: `main(colour,T)` sets that colour, draws two
horizontal lines on row `o` (from `m` to `n` pixels left and right of
`D+l`) and returns the time since the lap started. In simple.c this is
`draw_spans()`, and the global variables `l`, `m`, `n` and `o` are its
"pen".

3. Everything inside for loop headers

The whole game is a `for` loop whose condition reads the options and whose
step copies the picture to the window and waits. Its body is five more
`for` loops with empty bodies. All the work is done in long chains of `?:`
and commas in the loop conditions and steps. For example the loop that
builds the track also opens the window when its counter `r` reaches the track
length, and later reads the key presses, all in one condition.

4. Loops used as state machines

A single loop draws several different things by changing its own state. The
first drawing loop draws black rows while `o` counts down, then the sky,
then (when `o` hits 0) the sun until `k` reaches 17, then mountains until
`i` reaches 5. The car loop draws the black shape of the opponent car, then
its paint, then resets `x`, `a`, `l` and `f` and draws the player's car the
same way. simple.c uses a separate loop or function for each part.

5. Single letter variables reused for many jobs

All letters `a`-`z` are globals and most have several meanings. `r` counts
the track steps, then counts event polls, then holds the white colour, and
the car paint colour is `r*r`. `i` is the curve of the track, then 60 (both
the crash speed and the seconds in a minute), then a loop counter. `a` is the
track segment type, then the car row, which is also the top speed (163 mph)
and the bottom of the sky. `h` is the lap time and also the road colour.
`B` is the track shape and later the lane of the opponent cars. Leftover
values also matter: in the first frame `a` is still 5 from building the
track, so almost no sky is drawn.

6. Constants with many uses

`E` (53300) is the start time 8'53"00, five laps (so the first frame starts
a new lap) and the grass colour `0x00D034`. `F` (65793 = `0x010101`) turns a
number into a grey colour, is the offset of the key table in the array `C`
and is the `select()` timeout. `p = E/5` is the track length and divides
the microseconds into hundredths. `D` (200) is the window height, the centre
of the screen and the draw distance. `A<<17` is the red colour `0xC80000`.

7. Hidden function call in an unused printf argument

`sprintf(P, ..., s, main(A<<17))` has one more argument than the format
needs. The extra `main()` call is only there to set the text colour to red
before `XDrawString` uses it.

8. One array for everything

`C[1<<18]` holds the sideways road position (`C[i]`), the road height
(`C[p+i]`) and the keyboard state (`C[F+keysym]`). Reading past the end of
the first part reads the second one.

9. Tricky expressions

`C[F+XLookupKeysym(&O.xkey,0&XNextEvent(K,&O))]^=...` hides the
`XNextEvent` call inside an argument that is always 0. `w[z+*T]` is
`argv[1][z+w]` written backwards. `z|w-2` is -2 on a summer day and -1
otherwise, which sets how fast colours fade. `!a|a%2?g:a-3` makes the
track go down, up or flat. The macro `W(a)` fades each colour byte with
the distance `y` and is pasted three times to build the full colour.

10. Struct punning with an int array

`H[4]` is used as a `struct timeval` for `gettimeofday()` and `H+1` as the
`select()` timeout. This only works as intended where `long` is 4 bytes; on a
64-bit system the time is read from the wrong half and the timeout comes from
memory after the array.
