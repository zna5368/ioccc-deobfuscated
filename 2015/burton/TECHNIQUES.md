# 2015/burton

Difficulty: Hard

1. No digits anywhere in the source

`prog.c` has no number or character constants at all. Every number starts from
a global variable, which C sets to zero. In `q()`, `b=!b` makes 1, `f=-~b` makes
2, `u=f|b` makes 3, `s=b<<u` makes 8, `c=s|f` makes 10, `a=s<<f` makes 32 and
`e=a<<u` makes 256. Other numbers are built from these wherever they are needed:
`u<<t` is 48 (the character `0`), `c*c+b` is 101 (the character `e`) and `a*t-s`
is 120 (the character `x`). `simple.c` writes `'0'`, `'e'` and `'x'` directly.

2. Arithmetic written with bitwise identities

`-~x` means `x+1` and `~-x` means `x-1`, because `~x` equals `-x-1`. So `~-d` is
-1 (the value of `EOF`), `~-a/f` is 15 (the mask for one hex digit) and `y-~x` is
`y+x+1`. The two forms look almost the same but go in opposite directions, so
each one has to be worked out by hand.

3. Text hidden in one encoded string

`S[]="syntax_error=M@K~|JOEF\\^~_NHIA"` holds both the error message and the
operator table. `q()` repairs it at run time. `q[~s]=a` changes `_` to a space,
and `*q=!c` replaces `=` with a zero byte to end the message. The rest of the
string is the operator characters with 32 added, so `M` is really `-` and `@` is
really a space. `|` and `~` are too large to store with 32 added, so `q--[f]+=a`
and `q--[c]+=a` add it at run time. `simple.c` lists the operators in plain text.

4. Numbers in place of characters and operators

Operators are never compared with characters. A table turns each input character
into a small number, for example `-` is 1, `+` is 3 and `(` is 15, and the parser
only sees those numbers. Worse, the code subtracts from the number as it goes:
`z()` does `v-=u<<f`, then `v-=f`, then `v-=t`, and tests the leftover value each
time, so the meaning of `v` keeps changing inside one expression. `simple.c` uses
an `enum` and a `switch`.

5. Memory taken from below the stack

There are no arrays and no `malloc`. `B=(L*)N+~e*e` takes the address of `argv`
and moves 65792 bytes down, into stack space that nothing is using yet. The line
buffer, token table, hex digits and registers are then just offsets from that
address (`A=B+e/f`, `x=B+e`, `o=(V*)(x+a)`). `simple.c` uses a normal static
block with named offsets, and keeps the order because a very long command line
overflows from one area into the next.

6. Meaningless one letter names and reused variables

Nearly everything is a single letter global, and letters are reused as locals
that hide the globals. `a` is 32, but it is also a parameter of `r()`, `g()` and
`h()`, and a local in `y()`. `e` is 256, but `M()` has a local `e` that is 4.
`j` holds the last character read, then a letter, then the number being parsed,
then the number being printed. `simple.c` gives each job its own name and makes
most of them locals.

7. Types and keywords renamed

`typedef` and `#define` rename the language: `V` is `long long`, `Z` is `unsigned
long long`, `L` is `char`, `P` is `void`, `R` is `return` and `W(x)` is
`for(;x;)`. Unlike a `#define`, a `typedef` is not removed by the preprocessor,
so the names are still there after running `cpp`. The names also clash with
functions and variables that use other letters, so you need a table to read any
line.

8. Only `for` and `return` for control flow

There is no `if`, `else`, `while` or `switch`. Decisions are long chains of `?:`,
such as the long choice of factors in `z()`, or use `&&` and `||` as an `if`, as in
`m()&&G()`. Loops are `W(...)`, and the `W` in `z()` is really an `if` because its
body always returns. The chain in `w()` that handles `$`, newline, `#` and `;` is
one single statement. `simple.c` uses `if`, `else`, `while` and `switch`.

9. Functions that hide inside expressions

To be allowed in `?:` and `&&`, functions that return nothing are declared with a
return type and simply never return a value, such as `C p(L*T)` and `O G(P)`.
Also, `setjmp` is called inside the argument of `_i(!setjmp(l)?...:(L*)S)`, so one
line of `main` holds the parsing, the printing and the error handling.
`simple.c` turns this into an `if (!setjmp(...))` with two branches.

10. Hidden addition and subtraction

There is no visible `-` for subtraction in the parser. `F()` handles both `+` and
`-` with `n+=(v-f)*y()`. Plus is token 3 and minus is token 1, and the token 2
between them is the space, so `v-f` turns them into +1 and -1. `z()` does the
same for a leading sign with `(v-f)*z()`.

11. Hand written number conversion

`printf` and `strtol` are replaced by custom code. `_c()` makes decimal digits
backwards, `M()` makes hex digits backwards and shifts them into place, and
`_d()` builds a number from text. The test for a hex prefix is
`(*++T|a)>>f==a-f`, which uses `|` to ignore case and `>>` to check four
characters at once. The value of a letter digit comes from
`b+(v&a)+a*f-c`, which is 55 for capitals and 87 for small letters.

12. A fake `getchar`

`k` is a function pointer that holds `getchar` or `_k`. The command line arguments
are copied into the upper half of the input buffer, and `_k` hands them back one
at a time as if they were typed. The single reader then works for both, but the
source never says that the arguments are being read.

13. Hidden side effects through globals

Functions pass information by leaving it in globals. `M()` changes `E`, which is
the end pointer used by the reading loop in `w()`, so `$` shortens the line being
read. `M()` also leaves `j` at zero, and `w()` reads that as end of input. The
flag for a silent assignment is stored in `o[0]`, the unused slot before the
registers, so writing it can also overwrite an over-long command line. `simple.c`
keeps these behaviours but explains them in comments.

14. Source drawn as a picture

The source is drawn in the shape of the word `calc`, which breaks up every
statement and makes the line structure useless for reading. Functions are split
only where a `for` loop needs more than one statement, because curly braces are
avoided, so the split does not follow the logic of the program.
