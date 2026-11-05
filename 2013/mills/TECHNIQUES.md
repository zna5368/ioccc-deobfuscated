# 2013/mills

Difficulty: Hard

1. Font hidden in whitespace

The string at the very end of `mills.c` looks empty, but it is 506 bytes of tabs,
spaces and form feeds. Each digit is 5 columns of 9 characters followed by a form
feed, so `G` jumps to a digit with `z=p*46+f`. A tab means "no dot" (`9-*z++`) and
a form feed ends the digit (`12^*z`). The rows are also stored bottom to top.
`simple.c` draws the digits as ASCII art in the `font` table.

2. Data declared early, defined last

`char*z,*f;` near the top only announces `f`. The real `char*f=` comes after
`main`, so the strange string does not sit next to the code that reads it.

3. One macro writes six functions

`S(x,y,z)` builds a function that repeats the expression `y` three times, once for
each of x, y and z. `S(B,*d++=*s++,*)` copies a vector, `S(o,...)` adds, `S(a,...)`
subtracts, `S(H,...)` is a dot product, `S(X,...)` scales and `S(v,...)` adds a
number. The third argument is `*` or nothing, which decides if the second
parameter is a pointer or a plain number. All of them return a sum because the dot
product needs one, even though the other five results mean nothing. `simple.c`
writes them out as `vector_copy`, `vector_add` and so on.

4. Reversed strings

`u` prints a string by calling itself first and writing the character on the way
back: `*s&&w((u(1+s),*s))`. That prints it backwards, so every string in the source
is stored in reverse. `"TADI"` is `IDAT`, `"RDHI"` is `IHDR`, `"DNEI"` is `IEND`
and `"...0.1/PTTH"` is the start of the HTTP reply. A search for those words finds
nothing. `simple.c` uses a normal loop in `put_string` and normal strings.

5. Swapped array subscripts

`2[E]`, `1[c]`, `9[z]` and `12[g]` are the same as `E[2]`, `c[1]`, `z[9]` and
`g[12]`, because C defines `a[b]` as `*(a+b)` and addition works in either order.
It looks like a mistake, and it mixes in with `*I`, which is another way to write
`I[0]`.

6. One-letter names that are reused

Nearly every name is one letter, and the same letter means different things. `h`,
`i`, `p`, `c` and `b` are globals, but `Y`, `J`, `G`, `T` and `main` declare their
own versions with other types: `b` is a `float` global and a `struct tm*` in
`main`. Globals change jobs too. `D` is a distance in `d`, a row counter in `G` and
a scale in `main`. `x` holds the seconds, then later the pixel column. `Q` is the
ray start, then scratch space in `Y`. `simple.c` gives each job its own name.

7. Function inputs and outputs hidden in globals

`d()` has no parameters, yet it reads `E` and `Q`, rewrites `E`, and hands back a
distance in `D`. `E` even changes meaning inside `d`: it comes in as a point to
aim at and leaves as a direction. `Y` returns nothing and leaves its answer in `c`.
You must read the whole function to learn what it touches. `simple.c` names these
`ray_origin`, `ray_direction`, `hit_distance` and `colour`.

8. Short-circuit logic used instead of `if`

`2[E]&&0>(t=(1+2[Q])/2[E])&&(D=-t,h=P);` is three nested `if` statements: `&&` skips
the rest as soon as a test fails. `d()-i||v(...)` runs `v` only when `d()` equals
`i`. Subtraction and xor stand in for `!=`, as in `if(h-P)`, `9-*z++` and `12^*z`.

9. Assignments buried inside other expressions

Work happens in places a reader does not look. `socket(R.sin_family=AF_INET,...)`
fills in a struct inside a function argument, `X(U,D=W(I)/1e3)` sets `D` while
passing it, and `*c=1[c]=(2[c]=2[E]/2)/2` sets three colours at once.
`12[g]=~(10[g]=j>>8)` writes two bytes in one statement.

10. Pointers used as a code

The pointer returned by `d` holds four kinds of answer. `0` is the sky, a value
below `L` is a light, the value `P` is the floor, and anything else is a sphere.
`P` really is the end of the sphere list, so nothing about it says "floor".
`simple.c` returns a sphere index or `HIT_FLOOR` or `HIT_NOTHING`.

11. Numbers written so they cannot be recognised

Well known values appear in decimal. `3988292384` is the CRC polynomial
`0xEDB88320`, `65521` is the Adler-32 modulus, and `2130706433` is the address
127.0.0.1. `r(33554433)` is `0x02000001`, which is four separate PNG header
fields (RGB, deflate, adaptive filter, Adam7) packed into one call. `w(120); w(1);`
is the zlib header.

12. Table packed into letters

`"xxxdtrb!  d r y "` looks like noise, but it is the Adam7 interlacing table. Only
the low bits of each character matter: `14&*z` and `15&1[z]` give the row and
column steps, `7&8[z]` and `7&9[z]` give the start positions. Each pass moves `z`
along by one, so neighbouring passes share letters. `simple.c` has a plain
`adam7_passes` table.

13. Holes filled in later

`Z` does `p+=4` to skip the chunk length and `J` goes back to fill it. The row loop
does `p+=4` for the block size, then `12[g]=~(10[g]=j>>8); 11[g]=~(9[g]=j);` fills
it in, at offsets that are never explained. `J` also uses `r(e-p-8)`, moving `p`
back and forward so that the same pointer does the writing and the CRC reading.
`simple.c` names these offsets and moves the row work to `begin_row` and `end_row`.

14. Program parts kept in the Makefile

Some of the source lives on the compile command line. `-Dl=10` turns
`listen(s,l)` into a backlog of 10, and `l` looks just like the digit `1`.
`-Dwin_port=...` turns `R.win_port=8224` into two assignments, one of which sets the
address. `LT`, `NL` and `EY` supply the lights and the eye position.

15. Relying on a palindrome port number

`R.win_port=8224` skips `htons`, which should convert the port to network byte
order. It works only because 8224 is `0x2020`, which reads the same with its
bytes swapped. `simple.c` calls `htons` so the code does not depend on this.

16. A loop that mostly does nothing

`for(i=M; L>i; ++i)` looks like it visits 9 lights, but there are 3 lights of 3
floats each. Six of the nine steps land in the middle of a light and cast a shadow
ray at a meaningless point. The `d()-i` test can never pass for those, so their
work is silently thrown away. `simple.c` loops over the 3 lights only.

17. Clearing by multiplying

`X(c,0); v(c,1);` sets a colour to white by scaling it to zero and adding one, and
`X(n,0); ++2[n];` builds the vector (0, 0, 1) the same way. This reuses the short
helpers instead of plain assignments, so it is not obvious that the old value does
not matter.

18. Formula that looks like a textbook one but is not

`t=b<t?t+b:b-t` sits where a ray tracer normally has `b-sqrt(t)`, so a reader
"recognises" it and moves on. There is no square root, and a reader who assumes
one will misjudge the shapes that get drawn. `simple.c` keeps the formula and says
so in a comment.

19. Source drawn as a shape

The code is laid out as a circle using tabs and spaces, so statements are cut at
odd points, even between a function name and its arguments, as in `write` / `(k,g,p-g)`.
Reading line by line tells you nothing about where a function starts or ends.
