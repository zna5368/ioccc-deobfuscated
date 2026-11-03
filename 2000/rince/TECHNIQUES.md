# 2000/rince

Difficulty: Terrifying

1. Floating point constants encoded as ASCII

The program needs 54 floating point constants (orbital periods, radii, angles in radians). None of them appear as numbers in the source. They sit in the first 216 characters of the long string assigned to `*A` (that is, `argv[0]`), four characters per number. Each frame, the loop `for(j=*A; *s++ = pow(t,(j[2]*'_'-3028+3[j])/t)*pow(6e-4+1,j[sizeof*X(++j)]+95**j-3072.)-1;) j+=4;` turns each group of four characters into a number. The first two characters choose a power of 1.0006 (a coarse logarithmic step), and the last two add a very small correction through `pow(t, x/t)` with `t = 300650000`. One is then subtracted. For example `"zS  "` decodes to 172.99999943 (the speed of light in AU per day) and `"8EC}"` decodes to 3.01488 (172.74 degrees in radians). The group `"  \037S"` decodes to exactly 0, which ends the loop.

2. A hidden bytecode interpreter ("J-code")

Everything after the constants in the string is a small program in a made-up stack language. The function `X()` steps through it one character at a time and uses a `switch` to decide what each character means. `' '` multiplies, `'}'` divides, `'I'` adds, `'='` subtracts, `'{'` is `sin`, `'/'` is `cos`, `'^'` is `asin`, `'>'` is `sqrt`, `'-'` reduces an angle with `fmod(x, 2*pi)`, `'!'` swaps the top two values, `'0'` tests `cos(x) < 0`, `'V'` and `'<'` form a loop that runs four times, and `'O'` draws a moon and its label. Lower case letters (and `[`, `` ` ``) store the top of the stack into a variable, and upper case letters (and `;`, `@`) push a variable back onto the stack. The real calculation is written in reverse Polish notation inside this string. For example `T I{k` means `k = sin(c52 + c53 * T)`. The deobfuscated version writes these formulas as ordinary C expressions.

3. Opcodes chosen to look like C punctuation and whitespace

The J-code characters were chosen to be `;`, `{`, `}`, `[`, `]` and spaces wherever possible, so that the string looks like broken C code or padding instead of a program. The space character is the multiply instruction, and `;` is not a statement end but "load variable number 59". The author's question "What does the `;` command do in J-code?" refers to this.

4. The constants are the bottom of the stack

There is no separate table of constants. After decoding, the stack pointer `s` is left pointing at the last constant, so the constants are simply the values already sitting on the stack, in reverse order. Each J-code instruction that needs a constant just uses whatever is on top of the stack, so the code never names a constant, and you have to count stack positions to find out which number is used where.

5. Subroutines in short strings, and a global that they reset

`+` runs the J-code `"K I@I-"` (compute `fmod(a + b*k + c, 2*pi)` from two constants `a` and `b`, the time variable `k` and the angle variable `` ` ``, used for the four moon angles) and `]` runs `"/ ="` (compute `a - b*cos(c)`). They are run by calling `X()` again, which recurses. Each call to `X()` sets the global loop counter `r` back to 0, so the four-step loop must not call a subroutine, and the variables loaded with characters below `'I'` are indexed by `r` (`I[*i + (*i<73)*r]`). This is how one loop body reaches the four moons' radii and angles as `A`/`E` plus `r`.

6. Unreachable code inside a switch (red herring)

`struct {...} Z; K z=j++;` is written at the top of the `switch` block, before the first `case`. Because `switch` jumps straight to a label, the `j++` is never run. Only the declarations matter: `z` is assigned later when a subroutine is called, and the draw instruction uses `strlen(z)` (3, the length of `"/ ="`) to pop three values off the stack. `case 1:` in front of `default:` is also never used because the string contains no byte with value 1.

7. `sizeof` used to hide that an expression is never evaluated

`j[sizeof*X(++j)]` looks like it calls `X()` and moves `j`, but the operand of `sizeof` is never evaluated. It is just `sizeof(char)`, which is 1, so this is `j[1]`.

8. Integer and character constants written as each other

Many numbers are disguised. `*"'    "` is the character `'\''`, which is 39, the y pixel of Jupiter's centre. `'_'` is 95, the base of the character encoding. `l=45<<9` is 23040, which is both the X11 full circle (360*64) and, together with `3.75`, the number of seconds in a day (3.75*23040 = 86400). `*(Y*)&g=6` sets the first member of `XGCValues`, which is `function`, to 6 (`GXxor`). The `case` labels mix decimal (`73`), octal (`076`, `\117`) and character (`'}'`) forms.

9. Variables reused for unrelated jobs

The loop `for(; v[V]=XCreateGC(S,d,4+v,&g), !--v; );` uses the counter `v` both as the array index (`v[V]` is `V[v]`) and as the GC mask: it creates the XOR GC with mask `5` (function and foreground) and then the normal GC with mask `4`. In the J-code, variables like `k`, `` ` `` and `[` are reused for several different quantities (sin V, the light-time-corrected day, J, K, psi, R, lambda, G) one after the other. The deobfuscated version gives each quantity its own name.

10. Labels taken from inside the program string

The moon labels `I`, `II`, `III` and `IV` are not separate strings. The drawing instruction points into the J-code string itself at `i-127+r/3*2`, where the four characters `IIIV` happen to be, and uses `r%3+r/3+1` (1, 2, 3, 2) as the length. A structure `Z` with the same layout as `XTextItem` is passed to `XDrawText` without a cast.

11. Macros and typedefs to hide names

`O` expands to `;break;case`, so each `O 61:` is really the end of one case and the start of the next. `W(f)` pastes `Window(S,` onto a word, so `W(XMap)` is `XMapWindow(S,`. `_` is `*s` (the top of the stack). `K` is `char *` and `Y` is `int`, so declarations like `K X(K i)` hide that `X` takes and returns a string.

12. Source shaped as a picture

The code is laid out in the shape of the X Window System logo, which breaks statements across odd line breaks and spacing.
