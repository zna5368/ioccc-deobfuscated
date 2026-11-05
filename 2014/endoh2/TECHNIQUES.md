# 2014/endoh2

Difficulty: Medium

1. Source code drawn as a double helix

The whole of `prog.c` is laid out as two twisting strands. To keep that shape, code
is cut at odd places and padded. Comments like `/*XYXY*/` and `/*X    YX*/` only fill
space, so `#define/*X    YX*/C Y(1)` is really `#define C Y(1)`. A backslash at the end
of a line joins it to the next, even in the middle of a string, so `"\40\"pro\` and
`g.c\"...` are one string. `simple.c` uses normal formatting and no filler.

2. Detecting an include with a tentative definition

`int(*f)(void),d,c,` declares a function pointer with no value. C calls this a
tentative definition: if nothing else gives `f` a value, it starts as zero. The macro
`GC` ends with `int(*f)(void )=m;`, which is a second, real definition of the same
variable. That macro is only used at the bottom of a helix, so `f` is only set when
`prog.c` was included by a helix. `main` then has `if(f)f();else`, which picks the
mode without any compiler extension. `simple.c` keeps this trick because the program
depends on it, but names the pointer `helix_decoder` and explains it in a comment.

3. The output is a C program made of macros

The helix does not look like C, but every piece of it is a macro. `A`, `C`, `G` and
`T` are the values 0 to 3, `AT` opens a function, `GC` closes it, and `CG` and `TA`
fill the rows where the strands cross. A compiled helix only needs to include
`prog.c` at the top to turn the picture into a working function. `simple.c` lists
these macros together at the end of the file with a table of what each one does.

4. Macros that break statements in half

`Y(z)` expands to `d;d=...;d=`. It starts by finishing an assignment left open by the
macro before it, and ends by leaving a new one open. So `A~~T` becomes
`...;d=~~d;d=...;d=`, which makes the `~` bonds real C operators (bitwise NOT) applied
to a value nobody uses. `AT` ends with `d=` to start the chain and `GC` starts with
`d;` to finish it. A single macro on its own is not valid C, so it is hard to tell
what it does. `simple.c` moves the real work into the function `decode_base()`, and
`BASE` only has the job of making the helix text compile.

5. Preprocessor lines hidden inside code

The `#define` lines are placed in the middle of other code. The statement
`for(;n--;)putchar(c)` has its closing `;}` on a line after three defines, and the list
`int(*f)(void),d,c,` continues as `x,q,r;` six lines later. The preprocessor removes
those lines, but when reading the source the declarations look broken up. In
`simple.c` the defines are all grouped together.

6. Macros used in the code that defines them

`TA` is a strand crossing in the helix, but it is also `q=`. The generator uses it
as a disguised assignment in `TA++c%8` and `TA(1+7*q-q*q)/3`. `CG` is the matching
`r=`. Inside a helix these assignments do nothing useful and only need to compile.
`simple.c` writes assignments normally and defines `CG` and `TA` as empty macros.

7. Formulas in place of tables

The shape of the helix comes from two small math formulas. `(1+7*q-q*q)/3` turns the
row number into the bond width: integer division rounds it into 0, 2, 3, 4, 4, 3, 2,
0. `r=c*15-c*c-36` is a curve that goes up then down, and `r/6+!q+4` turns that into
the indent, where `!q` adds one on crossing rows. `simple.c` replaces both with the
arrays `bond_widths` and `row_indents`, so the shape can be read directly.

8. All the work inside a for loop header

The generator is one `for` loop with almost everything in its condition. The reading
of input, row counting, and indent printing are joined by commas, and only the last
part, `q||x`, decides whether to continue. The body is a single `?:` choice between
printing a base pair and printing a crossing, with function calls joined by commas.
In the macro, `x=x?p(1,x),0:x` does the same thing: it prints a character and resets
`x` in an expression. `simple.c` uses `if` statements and a loop with one clear exit.

9. Disguised constants

Literal values are written in ways that hide what they mean. In the string `\40` is
a space and `\101` is the letter A, which makes `AT` hard to search for. `+0` is added
to the string for no effect. `8*8` is the starting place value 64, `32` is a space,
`126` is `~`, `10` is a newline, and `001` is just the number 1 written in octal.
`simple.c` uses plain strings, character literals, and named constants.

10. Short names reused for different jobs

All variables are single letters and most are globals of type `int`, shared by both
modes. `d` is the place value when generating a helix but a throwaway value when
decoding one. `x` is the input byte in one and the output byte in the other, and `c`
is the row in one and the letter count in the other. The parameter `c` of `p` also
hides the global `c`. `simple.c` gives each job its own named variable, and those
used by only one mode are local to it.

11. A redundant strand

Every base is written twice, as a letter and its partner, so the helix follows real
DNA pairing. Only the first one holds data. The decoder has `c++%2<1` to quietly skip
every second letter, which makes the data look twice as long as it is. `simple.c`
states this directly with `on_first_strand`.
