# 2014/maffiodo2

Difficulty: Easy

1. Meaningless identifiers

Every name is a single letter: `d`, `e`, `j`, `g`, `a` and `b`. Nothing says that
`d` is the output width, `e` counts pixels or `j` adds up colour values, so you
have to read each use to find out.

2. All the code on one line

The whole program is one line with no spaces, and nothing separates one statement
from the next except a `;`. Without indentation you cannot see at a glance where
the loop starts and ends or which code belongs to which `if`.

3. Implicit int and old style functions

`d=80,e,j;` declares three global variables with no type at all, and C assumes
`int`. `g()` and `main(a,b)` leave out their return type too, and `char**b;`
gives the type of a parameter after the bracket, in the old style of C. Without
the type words, a declaration looks like an ordinary expression.

4. Missing headers

There is no `#include` line in the program. `getchar`, `putchar` and `puts` are
used with no declaration, so the compiler guesses how they are called. `strlen`
needs a real declaration to work properly, and it comes from `-include string.h`
in the Makefile, which is outside the source code.

5. Nested call used as a repeat

`g(g())` looks like one function working on the result of another. `g` returns
nothing and takes no parameters, so the value passed along means nothing. The
inner call runs first and then the outer call runs, which is a short way of
writing `g();g();` to run `g` twice.

6. Hidden result in a global variable

`g(){j+=getchar();}` does not hand back the byte it reads. It adds the byte to the
global `j`, and the code that uses the total has to reset it with `j=0` before
each pixel. The reader cannot tell that `g()` changes `j` without reading the
body of `g`.

7. Several calculations packed into one expression

`b[1][j/3*strlen(b[1])>>8]` does all of its work inside an array index. It takes
the average of three values, multiplies by the length of a string and divides by
256. You also need to know that `*` happens before `>>` and that `/` and `*` work
from left to right. The values are not named at any step.

8. Shift used in place of division

`>>8` moves the bits of a number eight places to the right, which is the same as
dividing by 256. This hides why the number is there: a colour value has 256
possible values, so the result is a position inside the string.

9. Magic numbers

`80`, `3` and `8` are plain numbers with no explanation. The width at least sits
in the variable `d`, but that name says nothing about it, and `d` is never changed
after it is set.

10. End of input hidden in a running total

`j=0;g();if(j<0)break;` finds the end of the input without any mention of `EOF`.
It works because `getchar` gives -1 at the end of the input and no real byte is
negative. Only the first of the three bytes is tested. If the input stops part way
through a pixel, the two calls to `g` add -1 to the total, and the program prints
a character for that pixel anyway.

11. Side effects inside a condition

`if(!(++e%d))` adds one to the counter, finds the remainder and turns the answer
around with `!`, all in one test. It reads as "if not something", but it really
means "if the count has just reached a multiple of the width".

12. Unusual way to print a newline

`puts("")` prints an empty string. It only produces a line break because `puts`
always adds a newline after its text, so the line appears to print nothing.

13. Unnamed and unchecked command line argument

The text given on the command line is used directly as `b[1]`, twice, and is not
given a name. The argument count `a` is never looked at, so if the argument is
missing, `b[1]` is a null pointer and the program crashes as soon as it has a
pixel to draw.
