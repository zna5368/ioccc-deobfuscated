# 2012/vik

Difficulty: Terrifying

1. Recursive main as a function table

`main` is the only function, and it calls itself for every job. The value of the
first parameter `c` picks the job, the way a function name normally would. For
example `main(-24,v)` loads a file, `main(-23,v)` compresses data, `main(-7,v)`
loads a PNG and `main(-16,v)` runs brainfuck. To understand a call, you have to
work out which part of the huge `return` statement that number reaches.

2. Reusing argc and argv for everything

Because every job is `main`, every job must take the same two parameters. `c`
stops being an argument count and becomes a job number. `v` stops being the
command line and becomes a pointer to the global array `A`, used as a bundle of
arguments. `A[0]` is the current read position, `A[1]` the source, `A[2]` and
`A[3]` the ends of the compressed data, `A[4]` the output and `A[5]` an end
marker. A line such as `(A[5]=t+n,*A=t,A[1]=z,A)` is really a list of arguments
being filled in before a call.

3. The whole program in one return statement

Almost all of the logic is a single `return` with nested `?:` operators and comma
operators. The comma operator runs several expressions in order, so it replaces
a block of statements, and `?:` replaces `if`. The chain
`c+23?c+22?c+21?c+6?c+5?c+4?0:...` is a `switch` in disguise, because `c+23` is
false (zero) only when `c` is `-23`.

4. Hidden change to the selector

The test `-++c<4` adds one to `c` in the middle of the chain, so every comparison
after it is off by one. The branch guarded by `c+23` actually runs for
`main(-24,v)`, not `main(-23,v)`. Later code such as `main(c-7,v)` and
`(v[1]!=v[2])*--c` depends on this changed value.

5. Loops that run only for one job

There are four `for` loops at the top of `main`, and every call goes through all
of them. Each has `c` in its condition, for example `c==-11&&m<H`, so it does
nothing unless the job number matches. The first one is PNG unfiltering, the
second is the brainfuck interpreter, and the other two move bits. The loop
`for(m=0;...` also resets the global `m` on every call, which is why the last
loop can start with an empty `for(;m<n*2;m+=2)`.

6. Recursion in place of loops

Simple repeated work is done by calling `main` again. Writing a 32 bit number
counts `c` from `-128` up by 32, storing a byte each time, so `main(-128,v)` runs
four times. Reading a number counts `c` from `-4` up to `0`, each level adding
`256*main(c,v)`. The chunk walker and the row copier also repeat through
recursion, and `(v[1]!=v[2])*--c` ends the row copier by multiplying the job
number by a false condition to make it zero, which is the job that returns.

7. Meaningless single letter globals

All state is in globals named `i`, `j`, `k`, `l`, `m`, `n`, `Q`, `W`, `H`, `B`,
`Z`, `Y`, `X` and `O`, and the buffers are `x`, `z`, `s`, `t` and `w`. Some have
more than one purpose: `l` is the filter type in one place and a chunk length in
another, and `Z` holds a pixel and then a distance. `W` is the row width in
bytes, `H` is the total image size rather than the height, and `B` is the color
type.

8. Numbers in place of characters and strings

Character constants are written as numbers, so the brainfuck commands appear as
`n-62`, `n-60`, `n-91`, `n-93`, `n-43`, `n-44`, `n-45` and `n-46` instead of `>`,
`<`, `[`, `]`, `+`, `,`, `-` and `.`. Subtracting gives zero (false) on a match.
The mode letters are `101` for `e` and `98` for `b`. The name of the PNG data
chunk is the number `0x49444154`, which spells `IDAT` when written out byte by
byte, and the PNG signature is the number `0x89504e47`.

9. Reversed array indexing

In C `a[b]` means `*(a+b)`, so the two names can be swapped. The code writes
`1[v]` for `v[1]`, `5[*v]` for `(*v)[5]`, `n[*v]` for `(*v)[n]` and `(-c)[*V]` for
`(*V)[-c]`, which makes array accesses harder to recognize.

10. Two views of the same pointer array

`V` and `v` are the same pointer with different types. `V` treats the data as
`unsigned char` and `v` as `char`. Using `*V[4]` in one place and `*v` in another
looks like two variables, but they share the same memory, and moving one also
moves the other.

11. One buffer with several jobs

The same memory holds different things over time. `t` holds pixels, then the
compressed data, and its top half is the brainfuck tape. `w` holds the extracted
data, and its top half collects compressed PNG data. `s` holds both the embedded
file and the decompressed base image, so a short text leaves old image data
behind it, and the first four bytes of `s` are a marker that is also the chunk
name used for output.

12. Editing the file in place

Rather than build a new PNG, the program changes the one it loaded. The color
type byte is overwritten and its checksum rewritten, `memmove` deletes each `IDAT`
chunk, and a new chunk is inserted before the end chunk. The expression
`c=A[6]-x-12` finds that end chunk by stepping back 12 bytes from the end of the
last chunk, and `c` is reused to hold the offset.

13. Single letter check for the chunk type

`5[*v]-68` looks at one letter of the chunk name, the `D` of `IDAT`, instead of
comparing the whole name. The value `n>8` ends the chunk loop because the eight
signature bytes at the start of the file are never subtracted from the file size.

14. Bit tricks with operator precedence

`**V|~3&*V[1]++` works only because `&` happens before `|`, and it keeps the top
six bits of one byte and the bottom two of another. `*V[1]++<<6|56` moves the low
two bits to the top and fills in the rest. The tokens in `*(*V)+++(l?...` are
easily misread: it means `*(*V)++ + (...)`, a read that moves a pointer, followed
by an addition.

15. Shaped source code

The layout of the code is a picture, so line breaks and spaces fall in the middle
of expressions and tell you nothing about the structure of the program.
