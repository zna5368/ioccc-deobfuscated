# 2025/yang2

Difficulty: Hard

1. Picture-shaped layout

The whole source is laid out as a picture of Fern, so statements are cut off
in odd places, like `return+a?a%2+b(a/2)` running over a line break and
`m]=I[(X+i)& m]+2` being split by spaces. The compiler ignores this spacing,
but a reader can no longer see where a statement starts or ends.

2. Comments used as spacers and decoys

Empty comments `/**/` stand in for spaces, as in `static/**/int/**/f`, so the
words of the code can sit right next to each other. Other comments, like
`/*4665726e*/` (the CRC that spells `Fern`), `/*-=Y*/`, `/*-N*/` and `/*qW*/`,
are there to fill the picture and look like code, but they mean nothing.

3. Single letter global variables

Every variable is one letter (`f,e,r,n,X,x,Y,y,Z,z,p,q,l,I,i,j`), and `i`,
`z` and `p` are each used for several unrelated jobs. For example, `z` is a
match length in one place, a buffer slot in another, and the bits to print in
a third. In simple.c they become names like `encoded`, `read_count`,
`instruction_at` and `best_length`.

4. Strings compressed with a letter macro table

The output program is stored in `h[2]` in a shortened form. The function `B()`
prints a string, but when it sees a capital letter from `M` to `Z` it prints
entry `h[letter-77]` of the table instead (`M` gives `,`, `P` gives a space,
`Q` gives a newline, `R` gives `o,longjmp`, `S` gives `setjmp`, `U` gives
`#include`, `V` gives `#define` and so on). Some entries contain more of these
letters, so `B()` calls itself to expand them. A `~` means "print the next
character as it is", which is how spaces and newlines are written, because
`B()` skips every character that is a space or smaller. This is why the
string can be filled with extra spaces to fit the picture. simple.c just
prints the decoded text with `fputs`/`printf`.

5. Stringizing macro for table entries

`#define D(d)#d,` turns plain code such as `D(setjmp)` into the string
`"setjmp",`. Since the text does not need quote marks, it blends in with the
real C code around it. The `#define` is itself hidden in the middle of the
picture after the comment `/*-N*/`.

6. Hidden C/C++ switch with `sizeof('z')`

`h[1][2]*=sizeof('z')>1;` changes the magic word. In C, `'z'` is an `int`, so
`sizeof('z')` is more than 1 and the line multiplies by 1 (no change). In C++,
`'z'` is a `char` of size 1, so the line multiplies by 0, putting an end of
string after the `z` and shortening `"Pzoltraak"` to `"Pz"`. The same file
writes `zoltraak` when built as C and `z` when built as C++.

7. Off-by-one string tricks

`puts(h[z&1]+1)` picks the line to print from a bit. `h[0]` is `","` and
`h[1]` is `"Pzoltraak"`, and skipping the first character gives either an
empty string or `zoltraak`. The extra first character is only there so the
same entries also work in the macro table from technique 4.

8. Magic numbers and odd number formats

`m=131071` is the ring buffer mask `(1<<17)-1`, `0x0` and `016` (octal 14)
hide plain numbers, and `^+288` means `(byte ^ 0x20) | 0x100`, which flips
bit 5 and adds the end bit for a literal. `Z+((n-Y-1)<<9)+m-2` builds a repeat
code: `m-2` is `(1<<17)-3`, so it subtracts the minimum match length of 3 and
adds the end bit at the same time.

9. Side effects packed into conditions and loop headers

Work is hidden inside tests and `for` headers. `(i=getchar())-EOF` reads and
tests in one go. `for(i=0;i<Z;l[z]=i++>0?-1:n){I[z=e++&m]=...}` records which
instruction starts at each byte in the loop's increment part. The match loop
`for(z=0;...;z++){}` has an empty body because the whole comparison, including
an assignment to `p` and `q`, sits in the condition.

10. Recursive one line helper

`int b(int(a)){return+a?a%2+b(a/2):0;}` counts the 1 bits of a number using
recursion and a `?:` instead of a loop. The extra parentheses in `int(a)`
and the `+` in `return+a` are only there to look strange.

11. Self-including generated program

The printed prelude is its own puzzle. It defines the magic word as
`case __LINE__:a(__LINE__);`, wraps everything in `switch(setjmp(j))`, and
uses `#include __FILE__` so the generated file reads itself a second time to
get the code lines. Repeat instructions are run by `longjmp` back to the line
number of earlier output, with a small stack `k[999]` remembering where to
come back to. This is why the encoder limits the nesting depth to below 998.
