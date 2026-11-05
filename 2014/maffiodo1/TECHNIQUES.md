# 2014/maffiodo1

Difficulty: Medium

1. Meaningless identifiers

Every variable and function has a name of one or two letters, such as `e`, `h`,
`bd` and `bl`. Nothing in `bd(r,0)` says that it moves the player sideways, so
you have to read each function and work out its job before the rest of the
program makes sense.

2. Keywords hidden behind macros

`#define b if (` makes `b B&8){` mean `if (B&8){`, and it leaves an opening
bracket with no partner, which looks wrong to the eye. `a(X,0)` expands to a
whole `for` loop over the object table. The macro `a` even names its own
parameter `b`, the same name as the `if` macro.

3. Source code shaped into a picture

The code is arranged into a picture that only lines up when tabs are 4 wide.
Statements are cut at odd places to fit the shape, for example `u=C+X *7` and
`return` sit next to large gaps, and a single expression can run over several
lines. Comments such as `/*IOCCC/2014*/` and the letters `N E S` fill the
spaces, and they carry no meaning.

4. Comments inside include directives

`#include /*!!TAB=4!*/<stdio.h>` puts a comment between `#include` and the file
name. It is valid C, but it makes the line look unusual and tools that search
for a plain `#include <...>` line do not find it.

5. Implicit int and old style functions

`e,h,f,g,i,j,...;` declares global variables with no type at all, and C assumes
`int`. Functions such as `Z (X,m,n,o)` leave out their return type and the types
of their parameters too. This removes the words that normally tell you what a
declaration is, so declarations look like plain expressions.

6. A flat array used in place of a struct

All the game objects live in one array, `C[333*7]`, with seven numbers for each
object. Fields are reached by position, so `u[0]` is x, `u[3]` is the flags and
`u[-1]`, a negative index, is the "active" field that sits just before the
pointer. The reader has to remember what each number means.

7. Hidden results in global variables

`W(X)` looks like a lookup, but it also fills in the globals `u`, `x`, `y`, `H`,
`z`, `A` and `B`. Later code uses those values with no sign of where they came
from: `bd` reads `B` and `u` straight after calling `Z`, because `Z` calls `W`
inside it. To follow one line you need to know which function ran last.

8. Name shadowing

Local names hide globals with the same name. `Z (X,m,n,o)` has parameters called
`m`, `n` and `o` that are also the player's global position and height. `bc(e)`
and `bh(m,e,k,bi)` do the same thing. Inside `bd`, `int I[]` is an array while
the `I` a few lines above is a global number. In `main`, the first loop has a
local `int u[6],*I`, but the next loop uses the global `I`.

9. Reusing one variable for unrelated jobs

The global `I` is the rightmost allowed position in `bd`, the "was blocked" result
in `bj`, and the key code in `main`. In `main`, `H` starts as a colour mask and
later becomes a loop counter. The same name means something different each time.

10. Magic numbers

Plain numbers stand in for named constants. `276`, `275`, `32` and `27` are the
left, right, space and escape keys, `2` and `3` are key down and key up events,
`0xffff` starts every part of SDL, and `B&9` tests for "enemy or power-up".
The player's sprites are also just numbers, such as `bc(24)` for the dead sprite.

11. Positional struct initialisers

`SDL_AudioSpec bu={8000,8,1,0,256,0,0,bq}` sets the audio settings with a list of
bare values. You need the SDL header next to you to see that `8` is the sample
format, `1` is the channel count, `256` is the buffer size and `bq` is the
function that plays the music.

12. Colour masks built with shifts, and a pointer trick for byte order

`H=255,bz=H<<8,bA=bz<<8,bB=bA<<8` builds the four colour masks by shifting one
value, instead of writing them out. `!*(char*)&N` looks at the first byte of the
number 1 to find out the byte order of the computer, and the masks are then
swapped around with more shifts.

13. Side effects packed into expressions

Assignments are placed inside other expressions so several things happen at once.
`r=by?0:(p=-1)` sets the speed and, only on a key press, the facing direction.
`by?0:(t?(s=-9):0)` is really an `if` statement written as nested `?:` operators,
and `n+=o=i` changes two variables in one go. `b I=(X!=bk&&Z(bk,E,F,i)&&(B&6)))`
assigns, calls a function and tests the result in a single condition.

14. Filler in loops and statements

`a(X,0)` passes a `0` as the "step" part of the loop because the macro needs
something there, and that `0` does nothing. Empty statements such as `; ;` and
the unused globals `f` and `g` are noise that gives the reader more to rule out.

15. Mismatched pointer types

Pointers are declared as `int` or `char` pointers whatever they really hold, to
save bytes. The audio callback `bq(int H,int*br,int bs)` should take a `void *`
and a byte pointer, and the Makefile has to turn off the compiler's complaints
about this. The declared types therefore give no help in working out what the
data is.

16. Rules hidden in the level loading loop

`b!u[3]){q=I+1;m=u[0];n=u[1];}` runs for every object that has no flags. The
result is that the last such object in the file becomes the player, a rule that
is stated nowhere. The same loop stops by checking only the last column for a
negative number.
