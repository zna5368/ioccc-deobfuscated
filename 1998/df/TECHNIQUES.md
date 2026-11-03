# 1998/df

Difficulty: Terrifying

1. The source file includes itself

`#include __FILE__` appears three times. `__FILE__` is the name of the file
being compiled (`df.c`), so the file is read by the preprocessor several times
in a row. On each pass a different set of macros is defined (`int`, `while`,
`signed`), so `#ifndef int`, `#ifdef while` and `#elif defined(signed)` pick a
different part of the file each time. The real program is pieced together from
fragments scattered across the passes: for example the `?:` expression that
decodes words starts on one pass and ends on the next (`(p _)=(i-'a')[d]:...`).
Running only the preprocessor and reading its output is the way to see the
actual code.

2. Keywords redefined as macros

`int`, `signed` and `while` are turned into macros. `while(int)` does not loop:
it expands to the declarations in `main` and `fopen(__##int##__,"r")`, where
`int` is the argument `FILE`, so it pastes into `__FILE__` and opens the
program's own source. `signed` expands to the whole guessing function `l()`,
and `int` expands to `typedef (*define)();` plus the function table `O`.
`#define y define` lets `#y` look like a directive, but it is only used inside
comments and dead code. `#define _ ++` hides every increment as `i _`.

3. Data hidden in the program's own source text

There are no words in the program. At run time it opens `df.c`, and for seven
lines it skips to a tab, then a space, then decodes every character up to the
end of the line. The "junk" text such as
`c[a]+s,p[c],r[m]+u[i+4*o|f]...`, the variable names in `n()` and `X()`
(`pb`, `Md`, `iev`, `jmqhu`, ...), the comment `/*3-m*'c',bs='g';` and the
string `"gxdgbtgxsxpcctvpixktedhiedcte"` are the encoded word list. Even the `<g>`
after `"Demonic Smiley"` and the `cut` in `| cut here` are encoded letters, so
deleting comments or reformatting the file breaks the game. The clean version
just stores the 24 decoded words in an array.

4. Sliding dictionary compression

The hidden text is decoded with a 25-letter dictionary `d` that starts as
`NOPQRSTUVWXYZABCDEFGHIJKL`. A capital letter is a new letter: it is output and
pushed to the front of the dictionary (`memmove(d+1,d,24)`). A lowercase letter
`a`..`y` is an index into the dictionary (`a` = most recent letter), `z` means
a space, and every other character is ignored. This turns the readable words
into what looks like random code.

5. Calling functions through a table with reversed indexes

`O[6]={getc,putchar,memmove,printf,n,l}` is a table of function pointers.
`0[O](f)` is `getc(f)`, `(3&8|2)[O](d+1,d,24L)` is `memmove` (because
`3&8|2` is 2) and `5[O]("Demonic Smiley")` calls the game `l()` (the string is
ignored). In C, `a[b]` is the same as `b[a]`, so `0[O]`, `O[0]` and `3[O]`
all just pick table entries.

6. Variables stored in a shared array, written differently each time

The game function keeps its loop counter, its input character and its
uppercase guess in `d[1]`, `d[3]` and `d[8]`, the same array used as the
dictionary. Each use is spelled differently: `2[d-1]`, `3[d-2]` and `d[1]` are
all `d[1]`; `2[d+__STDC__]` and `2[d+!NULL]` are `d[3]`; `3[d+5]` and
`4[d+4]` are `d[8]`. The clean version uses named locals `i`, `typed` and
`guess`.

7. Hidden word and mask stored in the same buffer

All decoded words are packed into `s[234]` with no separators. The chosen word
`p` is cut off by writing `*(p+8)=0` (over the next word), and the row of
underscores shown to the player is written at `p+10`, inside the following
word. The clean version uses a separate `shown` array.

8. Bit tricks and odd initial values

`&223` clears bit 5 to turn a lowercase letter into uppercase. The counter of
hidden letters `m` starts as `(char)p+1`, the low byte of a pointer plus one,
which only serves to be non-zero. The two halves of
`(p[i]-d[8]||(p[i+10]=d[8]))+(p[i+10]-q||m _)` use `||` as an `if`: the
first reveals a matching letter, the second counts remaining underscores.

9. Labels and goto instead of loops

`main:` / `goto main` repeats the line decoder seven times (`c _<6` is true
for c = 0..6). The game uses `hell:`, `froze:` and `stoned:` with `goto` for
its print, read and exit steps. These become `for` and `do`/`while` loops.

10. Decoy code and misleading names

`n()` and `X()` are never called and only exist to hold encoded letters in
their variable names. `0&&3[O]("-->%s<--", ...)` never runs because `0&&`
short-circuits; it is only there to carry an encoded string.
`main(i,love_unix)` never uses `love_unix`, `while(FILE)` is not a loop,
and three copies of `int main(...)` sit inside a comment that starts and ends
in the middle of the lines.
