# 2012/omoikane

Difficulty: Insane

1. Source code drawn as a picture

The whole program is shaped like the character it is named after. To make the
shape work, strings are cut into tiny pieces such as `"KfW"` `""` `"Ww|"`, which
the compiler glues back together, and empty comments like `/**/` and
`/*^^*/` fill gaps. Comments such as `'UU!` and `NYA!` only add to the drawing.
Spaces and line breaks appear in the middle of expressions, so the layout tells
you nothing about the structure of the code.

2. Preprocessor directives hidden inside a declaration

`#define i(...)` sits between `_ d,b,` and `I[256]`, and `#define q(g)` sits
inside the brackets of `K[ ... 256]`. The preprocessor removes these lines
before the compiler sees them, so the declaration carries on as if they were
never there. Nearly all of the top of the picture is the body of `q`, joined
into one long line with backslashes at the line ends.

3. An if statement macro with its arguments out of order

`#define i(I1,Il,lI)if(Il){lI;}else{I1;}` puts the else branch first, the
condition second and the then branch last. Nested calls such as
`i(i(...),r-1,i(...))` must be read inside out, and `;` is passed when a branch
has nothing to do. The parameter names `I1`, `Il` and `lI` also look alike.

4. Function source turned into a string by a macro

`q(_*x(){...}_*X(){...})` passes two whole functions to the macro `q`. Inside
the macro, `g` pastes them in as real code, and `#g` turns the same text into a
string. The functions are compiled into this program and also printed into the
program it generates, so one piece of code plays two parts.

5. Confusing variable names

Almost every name is one letter, picked to look alike: `I`, `l`, `L`, `O`, `o`
and `0` are easily mistaken for each other. The type `unsigned int` is renamed
to `_`, so `_ *x()` and `sizeof(_)` look like punctuation. The letters `n`,
`y`, `a`, `r`, `u`, `k`, `o` also spell out the name of the program, which is
the only reason for their order.

6. Variables and buffers used for several jobs

`O` is a loop counter, then the value of a base 85 digit, then the length of
the block. `d` is both a "print this line" flag and the 32-bit number being
encoded. `p` starts as `argc` and becomes a position in a text. `K` holds the
seed and later the key. `c` holds the hint message and is then overwritten with
each block of input. `S` holds some text and is also the buffer for each output
line, so the first text is copied onto itself. A reader cannot learn what a
variable means from any single use of it.

7. Whole program written as one state machine

The output has several very different parts, but they are all made by the one
loop `for(C=Q=S;r-8;)`. `r` says which part is being printed, `Q` points to its
text and `p` is the position in it. Each pass handles one character, and tests
such as `r-4&&r-5` choose what to do for the current part. Normal code would
just do these steps one after another. Here the order has to be worked out by
following `r` through every pass.

8. Subtraction used as a not equal test

`r-8` is non-zero, which means true, whenever `r` is not 8. The same trick is
used in `z-63`, `C-S` and `b-12`. Together with the reversed `i` macro,
this means most conditions read as the opposite of what they do.

9. Numbers and characters written in disguise

Characters are given as numbers: `34` is a double quote, `92` a backslash, `63`
a question mark and `32` a space. Inside strings, a space is written as `\40`
or `\x20`. Plain numbers are also dressed up, as in `01`, `02`, `04`, `+86`,
`(+  1)` and `0x0`.

10. Pointer changed to create an empty string

`*s=C` looks like it stores one value, but it replaces the first entry of the
table of texts `s` with a pointer to the end of what was just written. The next
part of the program therefore reads an empty text, and that is what makes it
print the key instead of copying any characters. The first text is `S` and the
next is `s[0]`, so the entry that seems to hold the hint is never used for it.

11. Functions that return pointers for no reason

`x()` makes a random number, but it returns a pointer to it, so each use is
written `*x()`. `X()` returns `&O`, a pointer that nothing ever looks at. This
makes simple function calls look like pointer tricks.

12. Work hidden in for loop headers

`for(p=b=d=O=0;O<256;K[O++]=0)*K=+86;` clears the array in the header and keeps
setting the first item to 86 in the body, so the result is that `K[0]` is 86 and
the rest is 0. In the same way, `for(O=1;12>O;K[O++]=*x());` has an empty body,
and the digits in the base 85 loop are stored in the header with `c[--p]=...`.

13. Generated program written for three languages at once

The output is valid bash, perl and C. `#ifdef/*` is a comment to bash and perl,
but to C it starts a block that is skipped, and it also opens a C comment.
`'true' or q{` runs a harmless command in bash, while `q{...}` makes perl treat
the bash lines as a quoted string. `exec head -8 $0` makes bash print the first
8 lines and stop, and the perl code prints everything from the first blank
line onward. `__DATA__` tells perl to stop reading the code there.

14. Program that includes itself

The generated code has `_ K[]={ #include __FILE__ }`. The file includes itself
to fill in the array. On the second pass, `q` is defined, so the `#ifndef q`
part is skipped and only the numbers of the key are read. Each half is guarded
this way, which is why the halves can be cut apart and joined in either order.

15. Hidden message with a key trick

The string `c` looks like noise. It is a message encrypted with a key of all
zeroes, and it stays at the front of the data of each generated program.
`S=s+*K` starts decoding at an offset taken from the key. The real key's first
number is 86, which skips the message. If the key is removed, `K` is just `{0}`,
so decoding begins at the message and prints it instead of the real data.

16. Encryption and base 85 text encoding

The ISAAC random number generator hides in `x()`, with `?:` choosing the shift
amount where the published version lists four separate steps. Each 4 bytes are
mixed with a random number by `^=`, then turned into 5 digits by `%85` and `/85`.
`O<32?O+95:O+6` maps those digits to printable characters and avoids the quote
and backslash, and `(83+*S++)%89` reverses that mapping with no `if` at all.

17. End of data stored in a single character

`b?b-1?b-2?36:37:33:35` is a chain of `?:` that picks a character for the number
of bytes in the last block. In the decoder, `O-*S%5` uses the remainder of that
character to work out how many bytes to drop, so the character is both the end
of data mark and the length. These values look unrelated until they are worked
out by hand.

18. Unexplained line positions

`C>S+76`, `C<S+78`, `C<S+79` and `C!=S+6` control the layout of the output. Line
width is 79 and a string must close at column 78. The value 6 pads the last data
line with random characters so that the fixed decoder text after it ends exactly
at the end of the line. The numbers themselves give no hint of this.
