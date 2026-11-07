# 2024/kurdyukov2

Difficulty: Insane

1. Macros that are pieces of code, not whole statements

`D(o,s)`, `N(A,B,C)`, `A(j,n,v,w)`, `B(o,n)`, `C(j,v)` and `L` are not
functions. They are pieces of text with missing parts in the middle.
`L` opens two `for` loops and their `{` braces, but the matching `}` braces
appear much later in the code outside the macro. `A` takes an argument like
`I[v+m+s` that is missing its closing `]`, and the macro adds `*z+g]` after
it. You only see real C code after the preprocessor has run.

2. One macro body used for two different things

The whole 8x8 inverse DCT is written once as `N(A,B,C)`. `D` calls `N` two
times with different arguments. The first time `A` is `=c[z+8*` so it reads
the columns of the coefficients and stores into `T[...]` with `>>11`. The second
time `A` is `=T[z*8+` so it reads the rows and stores clamped pixels into the
output. The arguments `B` and `C` are text like `+1024; T[` that get pasted
between the eight output expressions, so the end of each line is built from
three different places.

3. Token pasting

`J(x)` is `jpeg_##x;` and `W(x)` uses it twice, so `W(create)` becomes
`jpeg_create_compress(&d);jpeg_create_decompress(&S);`. The libjpeg function
names never appear in full, which makes them hard to search for.

4. Magic numbers

The numbers `4433`, `15137`, `6270`, `7373`, `9633`, `16069`, `3196`,
`20995`, `2446`, `25172`, `16819` and `12299` are cosine values multiplied by
2^13 (8192), taken from libjpeg's integer IDCT. `>>11` and `>>18` are the
fixed point scale factors, `-(~H<<17)` is `128` plus rounding (shifted by 18),
and `71` is a starting value that makes the coefficient order formula begin at
coefficient 63.

5. Bit tricks for clamping

`(f>>=18)&-H?~(f>>31|-H):f` clamps a value to 0..255. `f&-H` is nonzero if
`f` is outside 0..255. `f>>31` is `-1` for negative values, so `~(-1|-H)` is
`0`. For values that are too big it is `~(0|-H)`, which is `255`. In the
same way `p*=t*=t<0?0:t` squares `t` and sets it to 0 if it was negative,
then multiplies `p` by the result, all in one line.

6. A formula that replaces a loop over coefficients

`for(f=k=71; (i=k<8?k*8-8:k%8-7?k-7:k/8+55); )` visits the 63 AC
coefficients one diagonal at a time, starting with the highest frequency
(63, 62, 55, 61, 54, 47, ...). The loop has no step. Each new index is
worked out from the last one (`k=i` is hidden inside a later assignment), and
the loop stops when the formula gives 0. Later, `k-i-7` is used to find out
if the current coefficient is the first one of a new diagonal.

7. Variables reused for many things

`q` is the float 512 used to sample the DCT basis functions, then the
difference between two pixels, then the ratio `Y/X` used to scale up the
coefficients. `f` is a work variable in the IDCT, but `D` also sets it to 0 at
the end, so the main loop uses it as a flag saying "coefficients changed
since the last IDCT". `a`, `b`, `g` and `z` are loop counters, IDCT values,
and quantization limits at different times. The variables `d`, `e`, `h`, `p`,
`q`, `t`, `u`, `v` and `T` declared inside `D` hide the outer ones with the same
names, but `a`, `b`, `g`, `z` and `f` inside `D` are still the outer
variables.

8. Chained assignments and comma expressions

Lines like `f|=b^=c[k=i]=a=z>r?r:z<a?a:z;` do five things at once: clamp the
new value between the lower limit `a` and upper limit `r`, store it in `a` and
in `c[i]`, set `k` to `i` for the next loop step, XOR it with the old value
`b`, and add the result to the "changed" flag `f`. The padding loops such as
`F(i,s)*I=I[s],I[h*s+s]=I[h*s],I++;` use commas to fit three statements and a
pointer move into one loop body. The pointer is moved back at the end with
`I-=s*y+s`.

9. A loop counter used as control flow

`F(E,2)` looks like it runs two times, but `Y>X?q=Y/X:E++;` adds one more
to `E` when no scaling is needed, so the loop stops after one time. Also
`F(i,64&&!K)` becomes `i<64&&!K`, so the dequantization loop only runs when
`K` is 0.

10. Quantization limits written as arithmetic

`g=(b+(b<0?-r:r)/2)/r*r` rounds the coefficient to the nearest multiple of the
quantization step `r`. `a=((g<1)-r)/2+g` and `r-=g?1-a:r+a` then find the
lowest and highest values that would round to the same multiple. The step `r`
is replaced by the upper limit, and `(g<1)` is used as the number 0 or 1 to
handle rounding of negative values.

11. Hidden types and an oversized buffer

`void*v,**R` is used for the `jvirt_barray_ptr*` array returned by
`jpeg_read_coefficients`, and `v=&S` is used to pass the decompress struct as
a `j_common_ptr`. Each row of the image buffer is `s=w*8+2` samples long,
but only `w+2` samples of each row are used. This makes the index math
harder to follow.

12. Short meaningless names

All variables are one letter. `S` and `d` are the decompress and compress
structs, `T` is the table of basis functions, `P` is the decoded block,
`I` is the decoded image with a one pixel border, `V` is the quantization
table, `c` is the current block of coefficients, and `X`/`Y` are dot products.

13. Windows main replacement

When `_WIN32` is defined, `main` is a macro that makes a new `main()`. It
sets stdin and stdout to binary mode and then calls the real code, which was
renamed to `M`.
