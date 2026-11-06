# 2018/poikola

Difficulty: Hard

1. Self-checksumming source

The program opens its own source with `fopen(__FILE__, "r")`, reads the whole
file with `getdelim(..., '\0', W)` and runs a Fletcher-16 checksum over it
(`a2=(a2+a2_[a3])%255, a5=(a5+a2)%t`). If the result `a5<<8|a2` is not equal to
`s=41456`, a `goto a4_` jumps over the whole animation. Changing even a single
character of prog.c breaks the program, which makes it very hard to tidy up or
experiment with the code. simple.c checksums `prog.c` by name for this reason.

2. Macros that hide keywords and punctuation

`#define p return `, `#define X typedef`, `#define E {` and `#define B }`
replace common keywords and braces (the author calls `E`/`B` Pascal's
`begin`/`end`). `#define $ printf("\x1b"` makes every `$"[40m"` an ANSI escape
print, `K` is `printf(`, `G(y)` sleeps and flushes, and `O` is `__DATE__+7`
(the year part of the compile date).

3. Typedefs with one-letter names

`_` is `int`, `H` is `char`, `P` is `unsigned short`, `w` is `double` and `z`
is an enum whose values `T` and `F` are 0 and 1. Declarations like
`_ vir(H __[])` or `w Z(w D,_ Y)` look like nonsense until these are replaced.

4. Code hidden inside comments (and comments inside code)

The `/*` at the end of line 22 starts a comment that contains what looks like
code (`c_ [9]; main`). The real code starts right after `*/` with
`_ a1, a3, a4=024; P a2, a5;` which declares the global variables. The text
`//END\` at the end uses backslash line continuation to turn several lines of
ASCII art into one long comment.

5. Source shaped as ASCII art

The code is laid out as the Big Dipper with the stars of Ursa Minor written in
comments. Statements are broken at random places, so a single expression like
`a1_=a7*V(a5_,...)-a1*V(90-a5_,a7)+66` is spread over several lines.

6. XOR-encoded strings

`l[]` holds the star names XORed with `-~__LINE__` (the line number plus one,
49), so moving that line changes the key. `I[]` is XORed with `'-'-'#'` (10)
to give "Lets find some Easter eggs". Both are decoded at run time in loops.

7. Strings stored as floating point numbers

The `float and[]` array has three floats whose bytes in memory spell
`"25th IOCCC!"`. It is printed with `printf("%.11s\n",(char*)and)`, but only
on little-endian machines, which is checked with `*(H *)(_[])E X_OK B`
(the first byte of the integer 1). Using `and` as a variable name also looks
like a C++ keyword.

8. Predefined macros used as constants

Numbers are replaced with system macros: `W_OK` (2), `F_OK` (0), `R_OK` (4),
`X_OK` (1), `F_TEST` (3), `F_TLOCK` (2), `SEEK_END` (2), `__ATOMIC_SEQ_CST`
(5), `__ATOMIC_RELEASE` (3), `__DBL_DIG__` (15), `__CHAR_BIT__` (8),
`__FLT_RADIX__` (2), `_POSIX_TRACE` (-1) and `unix` (1). `aI_` is
`~-(__FLT_MAX_EXP__<<1)` which is 255.

9. Obscured number literals

Values are written in odd forms: `024` is octal 20, `0550` is octal 360,
`-~9` is 10, `~-' '` is 31, `'d'` is 100, `'_'` is 95, `')'` is 41 (so
`N*')'` is 451), and hexadecimal floats like `0x1.1df46a226e211p-6` (about
pi/180) and `0x1.db6db6db6a92ap+0` (about 13/7).

10. Hand-written math functions

Instead of `<math.h>`, `Z` is a recursive power function, `u` is factorial,
`vir` is an `atoi` and `V` computes cosine in degrees with a Taylor series.
`V(90-x)` is used as sine. A rotation of each star by an angle is hidden in
these calls.

11. Misleading and recycled variable names

The globals `o,b,a,f,g,k,m` (the "Oh Be A Fine Girl Kiss Me" spectral class
mnemonic) are used for the Easter date calculation instead of star colours.
`a4` is first 20, then 1900, then the year, then the century, then the sleep
multiplier and finally the exit code. `a5_` is a string index, then the
rotation angle; `k` goes from century to the base colour `0xA5B9FF`; `main`'s
`argc` parameter `a6` is reused as the star loop counter.

12. Packed data tables

The `A2[]` array holds three numbers per star: an x offset plus 66, a row that
is scaled by 13/7 and has 14 subtracted, and a colour that is added to the
base colour `k`. The real coordinates and the RGB value only appear after
this arithmetic is done.

13. Inlined date algorithm

The date of Easter is computed with a chain of comma expressions mixed into
unrelated statements (`k=A6/'d',b=k/A8,a=k%4,...`). The final test
`(D^(R.tm_mon+1))==(Y-=R.tm_mday)` compares the Easter month and day with
today's date using XOR and subtraction instead of plain equality checks.

14. Pointer arithmetic and index swapping

`a5_[l]`, `e[__]`, `a1[I]` and `a6[A2]` use the rule that `a[i]` is the same as
`i[a]` in C. The star name is chosen with `l+((a6-F_TLOCK)/__ATOMIC_RELEASE)*6`
and printed with `%.*s` and a width of 6.
