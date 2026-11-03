# 1992/imc

Difficulty: Hard

1. All code inside the `for` loop header

The whole program is `for(;; ...);` with an empty body. Everything happens in
the third part of the `for`, the part that normally holds `i++`. That part
can only hold an expression, so the program uses no `if`, `while` or `{}`
blocks. It uses the `?:` operator for choices, `&&` and `||` for short tests
such as `(J=malloc(q))||(perror("malloc"),r)`, and the comma operator to run
several steps one after another.

2. A state machine instead of nested loops

Because loops cannot be written inside an expression, the variable `A` records
where the program is up to. `A==0` sets up the picture, `A==1` starts the next
row, `A==2` starts the next pixel in a row, and `A==3` does one Mandelbrot
step. Each pass of the endless `for` does one small step and changes `A` to
choose what happens on the next pass. In simple.c these became two ordinary
`for` loops (rows and columns) around a `while` loop.

3. Recursive `main` to walk through the arguments

Each option is handled and then the program calls
`main(c-q,v+q)`, passing an argument list that starts at the last argument used.
The new call starts again at `q=1`, so it reads the next option. When there are
no options left, the drawing code runs and calls `exit`, so `main` never
returns. simple.c uses a normal `for` loop over `argv`.

4. Option letters found by arithmetic

The option letter `_` is never compared with `'c'`, `'s'` and so on. Bad
letters are rejected with sums such as `_/2=='2'` (only `d` and `e`),
`(_-1)/3=='\"'` (`g`, `h` and `i`) and `_/05*2==','` (`n` to `r`). The
characters `'2'`, `'"'` and `','` are used only as the numbers 50, 34 and 44,
so the comma has nothing to do with commas. Valid letters are then told apart
with tests like `_>0152` (octal for 106), `_/4>27` and `_&1` (odd or even
letter code). The numbers are written in decimal, octal, hex and character
form to make them harder to compare. simple.c uses a `switch` with the
letters.

5. Macros that hide code and have unmatched brackets

Short macros hide common names: `C` is `double`, `o` is `char`, `l` is `long`,
`L` is `sscanf`, `i` is `stderr`, `e` is `stdout`, `E` is `putc` and `r` is
`exit(1)`. The macros `F`, `d` and `O` leave brackets open or close brackets
they did not open, so they only make sense when placed together. The macro
names and parameters are also arranged so that they spell out "Ian Colliers
Obfuscated C Code" down the left side.

6. `exit` redefined to give a value

`#define exit(a) (exit(a),0)` turns a call to `exit` into an expression with
the value `0`. This lets it be used with `||`, as in `K||(fputs(n,i),r)`,
where a function that returns nothing would not be allowed.

7. Shared argument reading macros

The `s` and `F` macros read one or two numbers for an option. They try the text
after the option letter first, then the next argument, and use `++q` and `--q`
to take an argument and give it back if it was not a number. The increments
are hidden inside `&&` chains, so whether `q` changes depends on which tests
run. In simple.c these are the functions `read_one` and `read_two`.

8. Meaningless single letter names

All variables have one letter names, and one is just `_`. Several are reused
for different jobs: `J` is first the current argument and later the picture
memory, `q` is first the argument index and later the picture size, and `G`
and `R` are either the Julia constant or the pixel position depending on the
mode.

9. Hidden strings and numbers

The output strings are hidden behind the macros `I` (the grey shade
characters), `a` (`"%s?\n"`) and `n` (`"0?\n"`). Characters are written as
numbers, for example `E(46,i)` prints a `.` on stderr. The Sun raster header is
a plain array of numbers `h[]` whose meaning is not given.

10. Fake code inside a comment

The end of the program has `/*/ (m=128,u--),A==6?exit(1):B<u . e=3,l=2*c*/`.
`/*/` starts a comment instead of ending one, so this code, which looks like it
does something, is never compiled.

11. Shape of the source

The code is laid out as a picture of the Mandelbrot set, so lines are split in
odd places and spacing does not show the structure of the program.
