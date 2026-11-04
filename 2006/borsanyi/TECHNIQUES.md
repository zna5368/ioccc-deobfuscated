# 2006/borsanyi

Difficulty: Hard

1. File that includes itself

The last lines say `#ifndef T` / `#include __FILE__`. `__FILE__` is the name of
the file being compiled, so the source pulls itself in again. It is read three
times. The `#ifndef C` / `#elif ! defined J` / `#else` block at the top picks a
different set of macros on each pass, and the `#undef D`, `#undef E` and
`#undef U` lines clear the old ones first. Pass 1 defines `C`, pass 2 defines
`J` and pass 3 defines `T`, which stops the include. So the same text in the
bottom half becomes three different pieces of code.

2. One list of names used three ways

The 25 `D(...)` lines are the same on every pass, but `D` changes. On pass 1
`D(N)` makes a prototype `void*N(void*);`. On pass 2 it makes `N,`, so the
lines become the initializer of the function pointer table `fn[25]` (opened by
`E` and closed by `U`). On pass 3 it makes the whole body of function `N`.
`E` and `U` also change each time: `E` is the `ak()` function, then the header
array and `fn` table, then the pixel buffer `B`. `U` is `int`, then `};`, then
`main()`.

3. The font is hidden in the function names

There is no font table. Each of the 25 functions stands for one pixel of a 5x5
glyph, and its name lists the characters that have that pixel lit. For example
`bdefhklmnprtuvwxyz57` is the top left pixel. Inside the function `# N` turns
the name into a string, and `strchr(# N, *x)` checks if the current character
is in that list. Since names can only use letters, digits and `_`, the symbols
`@`, `.` and `-` are first changed to `K`, `P` and `M`.

4. A chain of threads

Each function starts the next one in its own thread. `r=tn` gives the function
its pixel number and `C(&t,NULL,fn[++tn],y)` (`C` is `pthread_create`) starts
function number `r+1`. The chain stops at 24, so 25 threads run at once. Each
one waits for the thread it started with `pthread_join`. `r` and the character
counter `n` are `static`, so each function keeps its own values between calls,
and `!n` is only true on the very first call.

5. Loops written as recursion

The pixel functions call themselves with `N(x+1)` to step through the string.
`ak()` also walks the string by calling itself. It rewrites each character as
it goes, adds 6 to the width counter `l` with a comma operator hidden inside
the `?:` test, and returns `ak(u+1)-1` so the pointer moves back to the start
of the string when the calls return.

6. Macros that split statements and braces

`J` writes the header and then ends with `K`, which opens a `{` block and starts
the `for` loops. `I` declares `int n,r;`, so `static I char *x=y;` is really
`static int n,r; char *x=y;`, and only `n` and `r` are static. The `U` for
`main` closes `K`'s brace with `;}` and then puts `L`, the final `return`, before
its own `}`. None of these macros is valid C on its own.

7. GIF data packed as escaped strings

The header `H` is a string of octal escapes and odd characters like `x`, `\b`,
`\200` and `,`. These are bytes of the GIF format: `x` (120) and `\b` (8) are
the default width and height, `\200` turns on a 2 colour palette, `\377` is
white and `,` starts the image. `h[6]` and `h[24]` are overwritten with the
real width. The trailer `"\1\t\0;"` is a 1 byte block holding the end code 9
(`\t`), an empty block and the `;` that ends the file.

8. LZW compression without compressing

Each pixel byte is `B[r][n]|8`. GIF data is LZW compressed with 4 bit codes
here, so one byte holds two codes. The low 4 bits are always 8, the "clear"
code that resets the decoder, and the high 4 bits are the colour (`16` in `B`
means colour 1). Because the decoder is reset before every pixel, its code
size never grows and the data can be written out raw. This is also why the
width `l` is used as the length byte of each row's block, which caps the text
at 42 characters (6*42+3 = 255).
