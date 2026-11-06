# 2024/cable1

Difficulty: Terrifying

1. Token ids hidden in a UTF-32 string literal

`I*_=U" 炾ોİ䃃璱ᝓ၎瓓甧染ɐఛ瓁"` looks like a string of random Chinese and
Indian characters. The `U` prefix makes every character a 32 bit number, and
each number is really a token id of the model (`28862, 2763, 304, ...`). The
main loop steps through them with `$=*++_` and prints the matching vocabulary
words, so this is the greeting shown at start up. The first character (a space)
is skipped by the pre-increment `++_` and the zero at the end of the string
tells the loop that the greeting is over.

2. Constants computed from other constants

The model sizes are hidden in arithmetic. `W=64`, `E=2*W` (128),
`D=Z*E` (4096), `H=86*E` (11008), and the vocabulary size 32000 is written as
the multi-character constant `'}\0'`, which is `'}'*256 + 0`. The `g()` macro
gets its loop counts from the size it is given: `s%11%5` is 1, 4 or 3 (the
number of matrices) and `s/6&33` is 1 or 32 (the number of layers) for the
sizes 32000, 4096 and 11008. The magic number `1088*s` is the size of one
matrix in floats (the int8 values plus one float scale per 64 values).

3. Macros that hide loops and names

`#define f(u,n)for(I u=0;u<(n);u++)` is a loop, and `x`, `N` and `u` are
shorter forms of it with the loop variable `i`, `k` or `h` already filled in.
`u` also adds `_Pragma("omp parallel for")` so the loop runs on many CPU cores
when compiled with OpenMP. `a(_)` glues `typedef` and `_t` together, so
`a(int8_)C` is `typedef int8_t C` and `a(floa)F` is `typedef float F`.
`_(s)` and `_()` paste a word in front of `printf` to make `sprintf` and
`printf`.

4. One letter and symbol names

Every variable is one character: `r` is the residual stream, `L`, `a`, `K`
and `J` are scratch buffers, `k` holds the logits, `G[0]`/`G[1]` are the key
and value caches, `$` is the current token, `s` is the position and `_` is the
greeting pointer. The same letters are reused for different things: inside
the attention loop `A`, `T` and `w` are new local variables that hide the
globals of the same name, and inside `j()` a local `_` is an integer sum and
`t` (a type name elsewhere) is an index.

5. Reversed array indexing

C lets you write `a[b]` as `b[a]`, so `i[A+e*D]`, `k[u[i]]`, `i[G]`,
`f[P][z]` and `1[...]` are all ordinary array reads and writes written
backwards. `1[w=cos(w),T=i+++(...)]` even does two assignments inside the
brackets before reading `T[1]`.

6. One function doing four jobs

`j(e,o,p,v,X)` does an optional RMS normalization, quantizes the input to
8 bits, multiplies it by a quantized matrix and optionally adds the result to
the residual stream. Which parts run depends on `e`: when `e` is the vocabulary
size there is no normalization (`c=0`) and the last loop `x D-c)` adds the
result into `r`; otherwise `c=D`, the normalization runs and the add loop runs
zero times. The number of 64 value groups is passed secretly through the global
`$`, set in the middle of argument lists such as `d[$=H/$,2]`.

7. Whole program in a for loop header

`main` is one `for(C*o;;...)` loop. The increment part does the printing of
each new token and resets the context when it is full, the condition part is
empty, and the body is guarded by the greeting test `!(*_?$=*++_:0)`. Input
reading, the chat template and the whole tokenizer live inside the header of
another `for` loop: `for(printf(...),scanf(...)?...:exit(0),p=sprintf(...);
z=p-=z;U++[o+=z,B]=f)`.

8. Ternary operators and comma lists instead of if statements

`~f?i[G]=malloc(3e9):exit(puts(...))` checks that `open()` did not return -1,
`p<U?$=B[p++]:fflush(0)` chooses between feeding a prompt token and flushing
the output, and `p<U||printf(...)` uses `||` so that printing only happens
when the prompt is done. Long chains joined with commas replace blocks of
statements.

9. Tricks with zero and one

`!!i*v[h]` is 0 for the first group and `v[h]` for the rest, so it starts a
sum without a separate reset. `i?w:0` starts a running maximum at zero.
`T[i]/=k?1:w` divides the attention weight by the total only once (when
`k==0`) and by one the other times. `z-=!f` makes the tokenizer try a shorter
piece only when nothing was found, and `~-E` is `E-1` (127, the largest int8).

10. Hidden math approximations

The rotary position embedding needs `10000^(i/128)`, which is written as
`exp(i%E/14.)` since `ln(10000)/128` is close to `1/14`. The attention scores
are divided by `11` instead of `sqrt(128)`, and SiLU is written as
`a/(exp(-a)+1)`. The softmax does not subtract the maximum first and the old
score is not cleared before adding the new one, which only works because the
values stay small.

11. Implicit declarations and missing return

`open()` is declared by hand as `I open()` instead of including `<fcntl.h>`,
and its flags argument is the global `f` which happens to be 0 (`O_RDONLY`).
The `mmap` flags are the plain numbers `1` (`PROT_READ`) and `2`
(`MAP_PRIVATE`). `j()` is declared to return an `int` but has no `return`, and
the last call `w=j(...)` uses that missing value only as a starting point for
the search for the largest logit.

12. Data layout read by pointer bumping

The model file is mapped into memory as one big float array `A`. The vocabulary
is read with `i[P]=(C*)A+4,A+=(I)*A`, where the first float of each entry is its
own length, and every matrix is cut off the front of `A` by the `g()` macro.
Whatever is left at the end is the RMS normalization weights, read as
`A+e*D` with `e` being the layer, the layer plus 32, or 64 for the final norm.
