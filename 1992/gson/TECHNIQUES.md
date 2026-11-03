# 1992/gson

Difficulty: hard

1. Source code shaped into a picture

The code is spread out over the page with tabs and spaces so that it forms the
letters "AG". Statements and even expressions are cut in the middle, for
example `j.j=` / `ftell(` / `stdin)` sit on three different lines. The
compiler does not care about the layout, but a person cannot read it. Running
it through a code formatter is the first step to understanding it.

2. One-letter names that look like the alphabet

Every global, function, struct member and parameter is a single letter from
`a` to `z`. The same letter means different things in different places: `g` is
the input sentence (a global struct) and also the signature member of the
struct (`j->g`), `h` is the line buffer and also the hash member, `i` is a
loop counter, a parameter and the count array member, `j` is the file offset
member and a local variable. `simple.c` gives each one a descriptive name such
as `sentence`, `signature`, `hash_table` and `subtract`.

3. Counting letters with bitwise operations

Instead of an array like `count['a']`, the letter counts are kept
"bit-sliced" in four `long`s: bit n of the first `long` is the 1s bit of
letter n's count, bit n of the second is the 2s bit, and so on. The function
`n()` subtracts two such vectors for all letters at once with `^`, `&`, `|`
and `~`, like an electronic subtractor circuit, and returns the borrow bits
that show which letters went below zero. There is no add function: `t()` adds
a letter by computing `0 - letter` and then subtracting that. `n(x,x,x)` is
used just to clear a vector to zero. `simple.c` names these `subtract()` and
`add_letter()` and clears the vector with plain assignments.

4. Whole loops written in the for statement header

Almost every loop has an empty body and does all of its work in the condition
and increment parts of `for(;;)`, joined with the comma operator. The
subtractor in `n()` does its work in the increment part, and the printing loop
in `v()` is `for(...;i=getchar(),putchar(...),i-'\n';);`. `simple.c` moves
the work into the loop bodies and uses `while` and `do`/`while` loops.

5. Conditional and logical operators used instead of if statements

`a || b` is used to mean "if not a then b" and `a && b` to mean "if a then b",
for example `q.g||(q.j=j->j,v(&q,'\n'))` and the chain
`!x||(j->g&~o->g)||n(...)||(u(&q),...)`. The bucket choice at the start of
`w()` is one large `?:` expression whose result is thrown away, ending in
`:0`. `simple.c` uses ordinary `if`/`else` statements.

6. Tricks that hide control flow

`&0` forces a value to zero so that a function call can be put inside a loop
condition: `(m=gets(m))||w(...)&0` reads a line, and at end of file runs the
whole anagram search and then ends the loop. In `main()`,
`(k=-atoi(m))&0` reads the `-N` option and stops the loop in one expression.
In `w()`, `?2:(x=0)` sets the depth to zero to make two nested loops stop at
once. The global `m` is reused: in `main()` it walks the arguments, and in
`y()` it is both the line buffer pointer and the "not at end of file" flag.
`simple.c` uses `return` and `break` and separate variables.

7. Recursion used to allocate memory

There is no `malloc()`. Each dictionary word that might be part of an anagram
is stored in the local variable of a new call to `y()`, and `y()` calls itself
to read the next word. The anagram search `w()` is started from the deepest
call, at end of file, while all those stack frames still exist. In the search
the local struct `q` of each call links back to the caller's struct, which
makes a linked list of the words found so far that `v()` prints from oldest to
newest. `simple.c` keeps this design but explains it in comments.

8. Disguised numbers

The magic numbers are written as calculations or in unusual bases:
`315*13` is 4095 (the hash mask), `64*64` is 4096 (the bit just above the
hash), `0400` is octal for 256 and `l[4095]` is the last hash bucket.
`simple.c` uses `HASH_MASK`, `HASH_SIZE` and `PLANES` instead.

9. Bit tricks to walk the hash table

The hash table index is the low 12 bits of a word's letter set, and the
search only visits buckets whose index is a subset of the remaining letters.
`r=(r&i)-1&i` steps to the next lower subset, and the `s|=s>>1, s|=s>>2, ...`
sequence spreads the highest stray bit down to all lower bits so that the
search can jump past buckets that use letters no longer available. A further
loop `p>>=1` finds the highest letter bit that still has to be used. These are
kept in `simple.c` but with named variables and comments.

10. Old style K&R function definitions and implicit int

Functions are written like `n(o,p,q)long*o,*p,*q;{...}` with the parameter
types after the parameter list, and most functions have no return type, so
they default to `int` and return nothing. `simple.c` uses prototype style
definitions with `void` and `long` return types.
