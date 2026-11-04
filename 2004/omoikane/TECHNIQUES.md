# 2004/omoikane

Difficulty: Hard

1. ASCII-art layout with decoy comments

The code is drawn in the shape of the character Rinia, so statements are split
across lines at random places, keywords are spread far apart (for example
`fopen` and its arguments sit on separate lines) and comments such as
`/*kero  Q'        ,KSS  */` or `/*b9213272*/` are only there to fill in the
picture. The comments look like data (the second one even looks like a
checksum) but the compiler throws them away.

2. Meaningless one-letter names and typedefs

`typedef char p;` and `typedef int _;` hide the types `char` and `int`, so a
line like `_ x(_ K,_ r,_ q)` is really `int x(int K, int r, int q)`. All the
globals (`u`, `w`, `v`, `R`, `i`, `N`, `I`, `A`, `m`, `o`, `e`, `a`, `k`,
`n`) are single letters, and `A`/`a`, `I`/`i`, `N`/`n` look alike while
meaning completely different things.

3. Tiny wrapper functions and a return macro

`#define c(C) return (C);` turns `c(...)` into a return statement, and the
helpers `E`, `B`, `D` and `C` are just `fopen`, `fseek(f,0,...)`, `fclose`
and `-puts`. Reading the code requires remembering what each letter does.
Because the macro ends in its own `;`, code like `c(C("...")) u=0;` puts a
second statement right after the `return` on the same line.

4. Reusing variables and arrays for unrelated jobs

The array `a` first counts how often each character appears in the layout,
then is overwritten with the file offsets of the patch bytes. `n` first holds
where each character starts a run of 8, then becomes the CRC-32 lookup table.
`N` is the previous character, then the lowest frequency, then the position
of the checksum slot. `R` is a character from the file, then a table entry,
then the random target CRC value.

5. Control flow written as nested `?:` expressions

Almost every `if` is replaced by the conditional operator. Opening and
reading the input file is one long chain
`E(...)?B(2)?0:(((o=ftell(f))>=8)?(u=malloc(o))?B(0)?0:!fread(...):0:0)?0:D():0`,
and the layout scan is `++a[R]?(R==N)?(++I>7)?(n[N]+1)?0:(n[N]=i-7):0:(N=R)|(I=1):0`.
Parts like `++a[R]?` and `((*w[I]=u[i])?1:(*w[I]=46))?` are always true and
only exist to sneak an assignment into the condition.

6. Work hidden inside `for` headers

The CRC loop `for(;r<q;K=...^n[255&(K^u[r++])]);` has an empty body, with all
the work in the increment part. The main search loop starts with
`for(*k=x(~0,i=0,*a);i>-1;)`, and the odometer step
`for(i=I-1;i>-1?!w[i][++e[i]]:0;)for(A=i--;A<I;e[A++]=0);` advances a
counter, detects the carry and resets the lower digits in one line. The
first loop that writes the checksum even starts with the `sprintf` call:
`for(sprintf(*w+1,"%08x",x(R=time(i=0),m,o)^~0);i<8;++i)`.

7. Encoded and split string literals

Strings are broken into pieces and use octal escapes, e.g.
`"<in" "file>" "\40<l" "a" "yout> " "<outfile>"` where `\40` is a space, and
`"  bad\40la" "yout  "`. Characters are written as numbers such as `46` for
`'.'`, `33`/`127` for the printable range, and `47`/`92` for `/` and `\`.

8. Hidden side effect of `sprintf`

The checksum string is printed into `w[0]+1`, which is the candidate list of
the first patch byte. This both supplies the eight hex digits that get copied
into the file and quietly cuts that patch byte's choices down to its original
character and those eight digits.

9. Exit status from the length of a message

`C` returns `0-puts(q)`, and every exit goes through `c(C("..."))`, so the
program's exit code is the negated return value of `puts`. The success case
`D()|C("OK.")` closes the file and prints the message in one expression.
