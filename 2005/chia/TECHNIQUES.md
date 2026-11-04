# 2005/chia

Difficulty: Hard

1. Disguising C as another language

The body of the file is a Java program, complete with `import`, `class`,
`public static void main(String[] args) throws IOException` and Javadoc
comments. Every Java word is a C macro. Words that mean nothing in C, such as
`String`, `new`, `throws` and `IOException`, are defined to be empty so they
vanish, and the rest expand into the real C code. The `import java.io.*;` lines
sit inside a `/* ... */` comment that is opened at the end of the `while`
macro, so the compiler never sees them.

2. Macros that expand into other macros

The real program is spread across chained macros. `class` starts the global
variables and the `struct`, `public` finishes the struct and defines the
direction table `F`, `static` begins the drawing function `f`, `void` continues
it, `this` and `byte` finish it, and `main(a)` defines the output helpers `b`
and `a` before opening the real `main()`. You have to expand them in the right
order to see one function, and a single function body is split across four
different macros.

3. Stringification of non-C tokens

`if(x)` is redefined as `b(#x)`, which turns its argument into a string. So
`if(C)` means "print the escape sequence ESC[C" (move cursor right), and
`if(7;21H)` and `if(2J)` produce cursor positioning and clear screen codes that
are not valid C tokens at all. In the same way `do(c)` becomes `a(#c "\b")`,
so `do(:)`, `do(.)` and `` do(`) `` print a single character followed by a
backspace. Using the keywords `if` and `do` as macro names makes the code look
like control flow when it is really printing.

4. Hidden busy wait delay

`try` is defined as `for(;--c.c;)` and is written four times in a row in the
output function. `c.c` is a `short` that starts at 0, so each loop counts down
through every value until it wraps back to 0, wasting time so the snowflake is
drawn as an animation. The struct `c` and its member `c` have the same name,
and `aSet`, `err`, `out`, `line` and `System` all expand to `c`, so the Java
statements turn into useless expressions like `c.c,c.c;`.

5. Encoded number tables

The string `"JF>:>FB;;BII"` holds two tables of six steps each. Subtracting 66
from each character gives the column steps `8,4,-4,-8,-4,4` and the row steps
`0,-7,-7,0,7,7` for the six directions a turtle can face (every 60 degrees).
They are read with the reversed indexing `N[F]` and `N[6+F]`, which is the same
as `F[N]` and `F[6+N]`.

6. Fixed point coordinates with different scales

The position `O` and `l` is stored multiplied by 16. Dividing by 16 gives the
column and the half-row, and dividing `l` by 32 gives the terminal row because
each row is split into a top and bottom half. The code also overwrites `O`,
`l`, `I` and `_` with their divided values in the middle of an expression, so
the same variable holds a scaled and an unscaled number at different moments.

7. Ternary chains in place of if statements

The whole drawing function is one expression made of `?:` and commas.
`y--?f(0),f(1),f(4),f(1):...` chooses between recursing (with the Koch pattern
straight, left 60, right 120, left 60) and drawing a step. The values `1`, `2`
and `8` in the unused branches are placeholders that are thrown away. The
condition `++Y[..][..]` is always true after the increment, so the `:8` branch
never runs.

8. Recursion depth counter in a global

`y` starts at 4 and is decremented by `y--` in the test and restored by `++y`
at the end of `f`, so it acts as the remaining recursion depth without being a
parameter.

9. Meaningless names and dead code

Variables are named `N`, `_`, `O`, `l`, `y`, `Y` and `I`, letters that look
like digits or each other. The Java leftovers produce code that does nothing:
an unused variable `reader`, a declaration `int d(int[])`, and a function `q`
that is never called.
