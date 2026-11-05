# 2015/duble

Difficulty: Hard

1. Every function merged into one

The only working function is `_()`. It reads the global `O`, which says which
job to do, and copies it to the local `O9`. The macro `Q` expands to `O9--||(`,
so each block in the function subtracts 1 from `O9` and only runs when `O9` has
reached 0. This makes one long expression act like a `switch` with ten cases,
but no `switch` is visible. `simple.c` gives each case its own function, such as
`draw_glyph`, `plot_pen` and `write_character`.

2. Recursion in place of loops

The code has no `for` or `while`. To repeat something, it sets `O` and calls
`_()` again, as in `O=3,_()` at the end of the drawing step or `O=4,_()` after
each input character. The call acts as the jump back to the top of a loop.
`simple.c` uses normal loops, which also removes the stack limit the README
warns about.

3. Conditions written as arithmetic

There is no `if` or `?:`. A comparison in C gives 0 or 1, so multiplying by one
turns a value on or off. In `_O+=(C<2)*12+(C==6)*(12-2*_O)+...` only the term
whose test is true adds anything. `&&` and `||` are used for the rest, as in
`(C==5)&&(...)`. `simple.c` uses `if` and `switch` statements.

4. Macros with unbalanced parentheses

`Q` opens a bracket that is closed much later in the code, and `k(RAFT))` needs
its second `)` to finish the `getenv(` call that `k` leaves open. This means you
cannot match brackets in the source to see the structure, only in the output of
the preprocessor. `k` also splits the name, so the text `"DRAFT"` never appears.

5. A fake main function

The macro `main( )` rewrites the line it is used on. The real `main` is hidden
inside the macro, and the visible `main()` at the bottom turns into a function
called `f` that nothing calls. Its `puts("hello world!")` is only a decoy, so
`simple.c` drops it.

6. Macros that inflate the code

`mu(a)` repeats its argument 5 times and `Q_` uses it three times, giving 125
copies. So `Q_({)` writes 125 opening braces, `Q_(})` closes them all, and
`Q_(!!)` writes 250 `!` signs, which is the same as testing for "not zero".
`!!!!!!!!!!` does the same job in the draft check. This is why the preprocessed
file is so large and so deeply indented.

7. Data read with a stride

The font string `O_o` looks like noise because neighbouring characters are not
related. The program reads every 97th character, starting at index 234, so the
real text is shuffled through the string. `Q_(O_o)` pastes 125 copies of the
string together so the pointer can keep going forward without wrapping around.

8. A key hidden in the size of a buffer

The stride of 97 is written nowhere in the code as a number. It comes from
`O1=sizeof(O5)`, and `O5` looks like an ordinary temporary buffer. Changing the
size of that buffer breaks the font. `O0+='a'` is the same step in disguise,
because `'a'` is 97, and `O0+=194` is two steps.

9. Numbers stored in base 92

Each glyph is stored as digits of a base 92 number, decoded by `__*92+*O0-35`
with `!` marking the end. A character such as `k` is followed by its digits, for
example `k%[A6+~KX!`. `simple.c` decodes all of this ahead of time into the
`glyphs` table.

10. A tiny drawing language packed into numbers

After decoding, each group of 4 bits in the number is one pen command, as in
`C=__&15`, so a whole letter fits in one `long long`. Command 5 is a subroutine
call: the next 4 bits say how many records to skip, and that is how `h` reuses
the stem of `l`. Repeats are done by changing the number itself. `__-=3` turns
the command into a shorter one, and `(__<<4)+C-3` adds a new command in front.
This makes one command take 2 or 5 steps with no counter. `simple.c` lists the
commands by name, such as `FORWARD_2` and `LEFT_5`, using the names in
`codes.txt`.

11. Direction calculated by a formula

The eight compass directions come from
`((_O%6>2)-(_O%7<2))*QQ+((_O+7)%8<3)-(_O>4)`, which gives the right x and y step
for each value of `_O` through a set of odd comparisons. `simple.c` replaces this
with two small tables, `step_x` and `step_y`.

12. Screen position stored as one number

`_Q` holds both x and y as `y*QQ+x`, so one addition moves the pen in two
directions. The code gets the pieces back with `_Q/O3`, `_Q%QQ` and
`_Q%O3/QQ*2`, none of which explain what they mean. `simple.c` keeps the single
number, because it must wrap at the edge in the same way, but it calls it
`pen_position` and names each piece in `plot_pen`.

13. Unicode built by hand

Each canvas cell stores the last two UTF-8 bytes of a Braille character, so it
can be printed with `*Q3>>8` and `85*3&*Q3` after a fixed `\xe2` byte. `85*3` is
just 255. The base value 41088 is hex A080, the bytes of a blank Braille
character. `*Q3%O4+O4|...` adds a dot without losing the others. The string
`"@CADBEHI"` is really the list of bit numbers 0, 3, 1, 4, 2, 5, 8, 9 with 64
added, which is the order of the dots in Braille. `simple.c` calls it `dot_bit`.

14. Standard input and output swapped

`close(dup2(3-dup2(1,dup(0)-3),1)*0+2)*0` copies stdin and stdout over each
other and closes stderr. After this the program reads with `read(1,...)` and
writes with `write(0,...)`, the opposite of what you expect. The expression runs
on every call of `_()`, but only the first time does anything. After that `dup`
gives 2 instead of 3, so the numbers passed to `dup2` are invalid and it fails
without any harm. `simple.c` uses the normal file descriptors.

15. A prompt that detects the terminal by itself

`write(1,"> ",2)` runs after the swap, so the prompt goes to standard input. A
terminal can be written to, but a pipe or a file opened for reading cannot, and
the failed write is ignored. So the prompt appears only in interactive use, with
no call to `isatty()`. `simple.c` keeps this behaviour and explains it in
`print_prompt`.

16. A secret input character

The check `OO==35` is really a test for `#`. Typing it draws the characters that
come before index 234 in the font string, and then the program quits. Those
characters spell "segmentation fault", which looks like a crash but is only
handwriting. The test `Q6%strlen(O_o)-O7` stops at the start of the font. As C
files begin with `#include`, this is what you see for `./prog < prog.c`.

17. Variables with more than one job

`O5` is the buffer for the window size from `ioctl`, the buffer for `sprintf`,
and the source of the stride. `O4` is the Braille base value, the delay for
`usleep` and part of the random threshold `9*O4*O1`. `O` and `QO` also hold
different things at different times, and `*((short*)O5+1)` reads the width out
of the buffer without using `struct winsize`. If the output is not a terminal,
this width stays 0 and the first division by it crashes the program.
`simple.c` uses separate named values.

18. Expressions used only for their side effects

`ioctl(Q0=0,TIOCGWINSZ,O5)^--O&(O3=...)` combines three unrelated jobs with `^`
and `&`, and the answer is thrown away. Only the assignments inside matter.
`*0` is used to ignore a result, and `_()|O++&_()` calls the function twice with
`O` changed between the calls, so it depends on the compiler working from left
to right.

19. Numbers without names

`signal(13,1)` means ignore `SIGPIPE`, `OO==10` means newline, and `OO-32` means
space. `58`, `12` and `4` control the line wrapping, and `7*QQ+Q0+2` is where a
glyph starts. `simple.c` uses names such as `ROOM_FOR_WORD` and `PEN_START_ROW`.

20. Names that look alike

Almost every name is built from `O`, `Q`, `0` and `_`, such as `O0`, `OO`, `Q0`,
`_0`, `_O`, `_Q` and `__`. They are very hard to tell apart when reading, and
the only function is called `_`.

21. Source layout that splits tokens

The code is formatted into a picture, and the gaps fall in the middle of
expressions. `O     ++&_()` is `O++&_()`, and one `_()` call is split so that
`_` ends a line and `()` starts the next. This hides where each statement starts
and ends.
