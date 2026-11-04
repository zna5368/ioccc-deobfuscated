# 2012/blakely

Difficulty: Hard

1. Single-letter names and type aliases

`typedef double o;` and `typedef char z;` hide the types behind letters. Nearly
every capital letter `A` to `Y` is a global `double`, and each one has a single
job, for example `S` and `C` hold the sine and cosine of the camera angle and
`R`, `D`, `U` hold the ray direction. Several names are also reused for
unrelated things: `o` is both a type and a macro parameter, `f` is a global
`double` and also a local `int` in `main()`, `e` is the key string and also a
local `int` in `s()`, and `Z` is a macro name, a macro parameter and the half
width variable in `main()`.

2. Macros that hide the operator tests

`Z(z)` expands to `if(*j==z)`, and `c(z,p)` builds a whole RPN operator from an
ASCII code and a C operator using token pasting (`p##=`). So `c(45,-)` is the
code for `-` and `c(43,+)` for `+`. The characters are written as numbers
(`42` is `*`, `100` is `d`, `116` is `t`, `121` is `y`, ...) so you cannot
search the source for them. `w(a,m,f)` does the same for `sin` and `cos`.

3. Token pasting to update three stacks at once

`o(a,b,c,d)` writes one statement for `bf` and the same statement for `bg`
by pasting `f` and `g` onto each argument, then pops the stack with `q--`. The
call is written so the pieces look meaningless, as in
`o(b,[q-1]=r*b,[q-1] + v*b,[q])`, which becomes
`bf[q-1]=w[q]*bf[q-1] + w[q-1]*bf[q]`. The value stack `w` and the derivative
stacks `bf` and `bg` are reached through the short macros `r` (top) and `v`
(below top). In `simple.c` these are written out as `value_stack`, `dx_stack`
and `dy_stack`.

4. Reversed array indexing

`a[b]` is the same as `b[a]` in C, so the code writes `q[bf]`, `(q-1)[bg]`,
`3[w]`, `4[w]`, `f[e]` and `(f%2)[b]` to make array accesses look odd.

5. Delayed digit push

A digit is stored one slot above the top of the stack (`q++ ... (q--)[bg]=0`)
and the flag `t` is set. Every later operator starts with the `y` macro,
`q+=t;t=0`, which finally counts the digit as pushed. The `,` operator commits
the digit by hand. In `simple.c` this flag is called `pending`.

6. Stack array reused as scratch space

`main()` keeps the ray length in `w[3]` and the normal length in `w[4]`,
slots that belong to the RPN value stack. The `sin`/`cos` code reads
`w[q+1]`, the slot just above the stack top, so these leftover values can
affect the result. `simple.c` keeps the same slots so the output stays the
same.

7. GIF bytes made from strange expressions

The GIF file bytes are never written plainly:

* `"GIF89a"` comes from the first six characters of `e` XORed with 1 to 6
  (`f[e]^f+1`).
* `"NETSCAPE2.0"` comes from `e[6]` to `e[16]` XORed with 3 to 13
  (`e[f]^f-3`). The string `e` is 17 characters long without a NUL byte.
* `putchar()` returns the byte it wrote, so `_(_((_(33)<<3)-9)/23)` writes
  `0x21`, then `(33<<3)-9 = 0xFF`, then `255/23 = 11`.
* `_((_(e[10]/2-1)<<3)-18)` writes `'D'/2-1 = 0x21` and then `0xF6`.
* The graphic control bytes `04 00 0A 00 00 00` come from the polynomial
  `-2*c*c+3*c+9` that is only used on every second loop step.
* The loop count bytes `03 01 80 80` are made by `f>15 ? 2*f-31 : c`, where
  `c` is 384 and `putchar` keeps only the low byte, `0x80`.
* The image descriptor picks bytes from `b` with `c&4 ? (1-c%2)[b] : 0`.
* `_(_(129)-129)` writes the end code `129` and then the terminator `0`.
* The 128 color palette is the expression `c/3%2 ? c/6<<2 : 0`, which makes
  even entries black and odd entries a gray ramp.

`simple.c` writes each of these bytes as a plain constant.

8. Loop counters reused for later constants

After the `for` and `while` loops finish, their counters are used again with
known values. `f` is 1 after `while(--f>1)`, so `f+=5` starts the NETSCAPE
loop at 6, and `c` is 384 after the palette loop, so `c/22` (17) and `c/29`
(13) are the loop limits that follow.

9. Sine and cosine without calling them

The camera angle is turned by `J = 3.1415/Z` each frame. Instead of
`sin()`/`cos()`, the code uses the first terms of their Taylor series,
`E_ = 1-J*J/2` and `T = J*(1-J*J/6)`, and applies a rotation to `S` and `C`.
Double angle values (`S*C*2` and `S*S-C*C`) then set up the camera axes.

10. Comma and conditional operators in place of `if` statements

The pixel shading is a single `?:` expression that holds a comma list
`(4[w]=..., H/=-4[w], K/=-4[w], fabs(...)*exp(...))`. The clear code that
splits each row in two is written as `O ? l : (_(128), _(...))`, where `l` is
only there to give the unused branch a value. `e/=-1` is a way of writing
`e = -e`.

11. Dead code

`0*b,[0]` and `0*b,[q]` add zero times another stack value, and `M`, `argc`
and part of `b` are never really used. `b[0] = Z>>9` is always 0, because `Z`
is a `char` that cannot be larger than 127. In `simple.c` the `0 * ...` terms
are kept because they can turn an infinity into NaN.

12. Layout

The code is shaped into the letters `X Y Z` with spaces, so statements are
broken across lines in odd places, such as the line break inside
`2*f-` / `31`.
