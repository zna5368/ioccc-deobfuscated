# 2014/skeggs

Difficulty: Hard

1. Code that writes and compiles more code while it runs

Parts of the program do not exist in `prog.c` as normal code. The `struct` macro opens
`t.c`, writes C source to it, runs the compiler to make a shared library, and loads the
library with `dlopen`. This happens three times: for the ncurses wrapper, the screen
drawing and the key handling. To follow the program you must picture the C code that
will be generated later, not only the C code you can see. `simple.c` keeps this design,
but moves it into `start_plugin()` and `load_plugin()`, and each library has its own
`compile_..._plugin()` function.

2. Real code and generated code mixed together

`E(x, v, w)` turns its first argument into a string with the `#` operator and writes it
to `t.c`, so `E(int f(int x) { switch(x) { , 0, 0)` looks like code but is really output.
Real statements sit between the `E` calls, such as the `while (*p)` loop in `w()`. The
braces of the generated code and the real code do not match up, so a function seems to
start or end where it does not, and tools that reformat C get confused. `simple.c` uses
ordinary `fprintf` calls with quoted strings, so it is clear what is output.

3. Macros that only work in the generated code

Because `E` turns its argument into a string, a name like `J(3)` or `A(%td)` inside it is
not expanded. It is copied to `t.c` as it is. It only means something because the first
line of `t.c` is `#include "prog.c"`, so the program includes itself to get its own
macros. This also copies all of `prog.c`, even `main`, into every library, and the `-DCC=`
flag is there so that this copy still compiles. In `simple.c` the generated code uses plain
names. It still includes `simple.c`, but only for the headers and the `GameState` type,
because `GENERATED_PLUGIN` hides the rest of the file.

4. Passing memory addresses through source code

A shared library cannot normally see the variables of the program that loaded it. The
line `#define Q (intptr_t)&` is used to print the address of a variable as a number into
`t.c`, like `V = *(W *)94280126214272;`. The generated code turns the number back into a
pointer, and the same trick is used for function addresses in the `case` lines. Each run
therefore generates different code. `simple.c` keeps this, and prints `&game` and `message`
into the generated code with clear names around them.

5. Constructor instead of a function lookup

`main` never asks a library for a function, and the result of `dlopen` is thrown away.
Instead the generated `bitand()` is marked `__attribute__((constructor))`, which makes it
run by itself the moment the library is loaded. It writes function pointers into
`V.T[13]` and `V.T[14]`, so they just appear in a table that looks empty when reading
`prog.c`. `simple.c` keeps the constructor but names it `plugin_init()`.

6. Hiding the ncurses library

`prog.c` never includes `ncurses.h` and is not linked with ncurses. The generated
`static_assert` library is the only one built with `-lncurses`, and it copies the
functions into a table. This is done from the string `"\0keypad\0getmaxx\0..."`, where
each name is printed into `*++G = (and_eq) %s;`. After that the calls look like `J(3)(...)`,
with nothing to say that 3 is `mvaddstr`. `simple.c` gives each function a named field in
`GameState`, so the same call reads `game->mvaddstr(...)`.

7. Numbers instead of names in a struct

All the game values live in `int U[6]` and are read with `B(n)`, and all the functions
live in `and_eq T[15]` and are read with `J(n)`. So `B(1)` is the player's health and
`B(4)` is the enemy's magic, but nothing says so. Index arithmetic makes it worse:
`B(2-n)`, `B(3+n)` and `B(1+n)` pick a different value for each side. `simple.c` replaces
both arrays with fields such as `player_hp` and `enemy_mp`.

8. Misleading names taken from C keywords

The size tool counts keywords as one character, so they were reused as names. `struct`
is a macro that builds a library, `restrict` is `sprintf`, `and_eq` is a function pointer
type, `xor_eq` is the draw function and `bitand` is the constructor. The libraries are
called `volatile`, `thread_local` and `static_assert`. The other names are single
letters, like `x`, `w`, `u` and `N`, and `x` is also a local variable in `k()`. `simple.c`
uses descriptive names, and only keeps the library file names because `.gitignore` and
`make clean` expect them.

9. One macro used for both sides of the fight

`X(a, n, b, g, f)` holds the whole attack, regenerate and heal logic, and is used once
for the player and once for the enemy. The arguments change the words, the numbers and
even the code: `g` adds a statement that limits how much MP is stolen, and `a` is pasted
straight after `B(n)` to build the `switch` value. The `n` also forces an early return.
`simple.c` writes this out as `player_action()` and `enemy_action()`.

10. One long conditional for the enemy decisions

The enemy's choice is a chain of nested `?:` operators given as the first argument of
`X`, starting `< 9 ? 0 : B(2) < 20 ? B(4) < 2 ? B(3) >= 1 : 2 : ...`. It even uses the
result of a comparison, which is 0 or 1, as the action number. `simple.c` turns this into
the `if` statements of `choose_enemy_action()`.

11. Misleading code layout

The source is formatted to look normal, but function boundaries are hidden. The closing
brace of `x()` is on the same line as the start of `w()`, and in `w()` a line that closes
the `while` loop is followed by more indented code. `while (*++x) for (...); { R(0, ...) }`
looks like a loop with a body, but the `;` ends the loop and the block runs once after it.
`simple.c` is formatted normally and each function stands on its own.

12. Packing several strings into one

The screen picture in `xor_eq()` is a single string with `\0` between the rows, and it is
walked with `*++x`. The string starts with `\0` because the pointer is moved before it
is read, and it ends with an extra `\0` to stop the loop. The list of ncurses names works
the same way. `simple.c` uses arrays of strings called `screen_rows` and
`curses_functions`.

13. Key table stored as differences

The keys are hidden in `(int[]) { 1, 3, 14, 5, 49, 32, 0}`. Each number is added to a
running total that starts at 9, which gives 10, 13, 27, 32, 81 and 113, the codes for
newline, return, Escape, space, `Q` and `q`. The handlers are in a separate array next to
it. `simple.c` uses one table of pairs, such as `{ 'Q', quit_game }`.

14. Bit tricks in place of a simple comparison

`j & (~((1+~1U) >> 1))` looks complicated, but `1+~1U` is all ones, shifting it right
clears the top bit, and `~` leaves only the top bit. That is the sign bit, so the test is
the same as `j < 0`. The arrow keys are returned as -1 and -2 and then added straight to
the menu position in `(B(0) + j + 3) % 3`. `simple.c` writes `key < 0` and names the
values `MOVE_LEFT` and `MOVE_RIGHT`.

15. Hiding values in unused arguments

The `E` macro always passes two values to `fprintf`, even when the format uses fewer, so
in `E(%cinclude "prog.c"\n ..., 35, 9)` the 9 is junk that is never printed. The 35 is
used: it is the character `#` printed with `%c`, which keeps a `#include` out of the text.
The same idea sets `D = 33` in the heal case, so that `Y(b need more MP%c)` prints `!` from
a number. `simple.c` writes these characters directly.

16. Tangled end-of-game loop

The death and win messages come from the single loop in `N()`, where `sleep(++c)` is
both the delay and the counter. `((B(1) < 1) + c) & 1` picks the name and `c & 1` picks
the word, so the first pass says who died and the second says who won. `simple.c` works
out `loser` and `winner` first and writes the two steps one after the other.
