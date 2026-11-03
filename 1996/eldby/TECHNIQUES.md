# 1996/eldby

Difficulty: Medium

1. Encoded strings

Every character that is printed is stored 5 higher than its real value and
`putchar(t[i++]-5)` subtracts 5 when printing. So `" `MYmtw%FF..."` is really
`ESC [ H` (move the cursor to the top left corner) followed by
`Thor AAge Eldby [eldby@online.no]`, `'\n'+5` is a newline, `37` is a space
(`'%'`) and the shading string `"3tjlq9TX"` is really `".oegl4OS"`. Because
nothing in the source looks like readable text, it is hard to tell what will be
printed.

2. One buffer used for the escape code, the name and the picture

The array `t` holds the cursor escape sequence, then the author's name, then
the 24 x 81 screen of characters (80 columns plus a newline). A second pointer
`p=t+3` points just past the escape sequence so that `p[c]` is cell `c` of the
screen. The rendering loop starts at cell 33 so that it never overwrites the
name, and the newlines are written once at the start and then skipped with
`c+=!((c+1)%81)`, which adds 1 to `c` only when it is in the last column.

3. Single letter global variables reused for everything

All variables are one letter globals: `x`, `y`, `z`, `r` are the centre and
radius of 5 spheres, `j` and `h` are the ray direction, `a`, `b`, `d`, `e` are
parts of the ray-sphere intersection and `l` is a random range. The global `i`
is used as a loop counter in `main()` but is also the hidden argument of `m()`,
which uses it to choose which sphere gets a new random position.

4. Magic numbers instead of named constants

The screen size 1944 (24 rows x 81 columns) is stored in the float `s`, and
other values are computed from it: `l=s*20` is the spread of random positions,
`l/3` is the distance between spheres, `l*2` is where a sphere respawns and
`s/2` is how far the spheres move each frame. The random number generator
`f=(f*6478+1)%65346` and the shift `g()>>4` for the radius are also unnamed.

5. Ray tracing math packed into one expression

The test `(b=(a=h*x[i]+j*y[i]+z[i])*a-(d=1+j*j+h*h)*(...))>0` assigns three
variables in the middle of a comparison. It is the discriminant of the
quadratic equation for where the ray `(h, j, 1)` hits a sphere: `a` is the dot
product of the ray and the centre, `d` is the squared length of the ray and if
`b` is positive the ray hits the sphere.

6. Hand written square root in an empty for loop

`for(e=b;e*e>b*1.01||e*e<b*.99;e-=.5*(e*e-b)/e);` has no body. All the work is
in the update part, which is Newton's method for finding the square root of `b`
until it is within 1%. This avoids needing `sqrt()` and the math library.

7. Using the return value of a function that returns nothing

`m()` has no `return` statement, but it is used as `+!m()` so that it can be
called in the middle of an expression that sets `z[i]`. The value it returns is
undefined. The compiler treats it as 0, so `!m()` just adds 1. In `simple.c`
the sphere is placed first and the `+ 1` is written out.

8. Loops written backwards and with side effects in the header

`for(i=4;i+1;i--)` counts down until `i` is -1 (`i+1` becomes 0, which is
false). The respawn loop `for(i=4;i+1;z[i]-=s/2,i--)` moves the sphere in the
update part of the loop, before decrementing. It also leaves `i` at -1, so the
following `while(i<s)putchar(t[i++]-5)` starts one byte before the buffer and
prints a leftover byte from the stack at the start of every frame.

9. Reading past the end of a string

The shading index `(int)(8*e/d/r[i])` is usually 0 to 7, but a ray through the
exact middle of a sphere gives 8. That reads the `'\0'` at the end of
`"3tjlq9TX"`, which prints as byte 0xfb after subtracting 5. `simple.c` adds
this byte to the end of its shading string so the output is the same.
