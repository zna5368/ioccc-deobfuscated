# 2019/mills

Difficulty: Horrific

1. Picture layout and decoy comments

The source is shaped like a picture, with large runs of spaces inside
expressions and statements broken across lines, even in the middle of names
(`rewi\` / `nd(f )` uses a backslash line continuation to split `rewind`). The
comments are decoys made of C keywords (`/*void*/`, `/*#pragma _Atomic int*/`,
`//struct volatile union {`, `/* typedef volatile long double not ... */`) that
look like code but are ignored by the compiler.

2. Program defined on the compiler command line

The network itself is not in `prog.c`. `NW` and `BK` are macros given with
`-D` in the `Makefile`, and they expand to variable declarations such as
`V hp = I(HS), t1 = L(HS, x), ...` inside `MD()` and `main()`. All the
hyperparameters (`LR`, `N`, `DI`, `PF`, ...) are also `-D` macros, so the
source has no numbers telling you the batch size, learning rate or file name
format.

3. Macros that hide control flow

`O` expands to `; { } } if(! n --) { { { ; } }`. It closes the code for one
kind of operation and opens the next one, so the text between the `O`s in
`J()` and `D()` is really a `switch` on the operation type: `n` is set to the
type and each `if(!n--)` tests the next case number. `OO` and `P(v,i)` hide
the setup of the pointers `e`, `p`, `q`, `a`, `b`, `r` and a backwards
`while(--q,p-->e)` loop over the elements of a vector.

4. A tiny virtual machine instead of direct code

The layer functions `I`, `L`, `A`, `M`, `S`, `T`, `C`, `CM`, `OG` do not
compute anything. They only reserve space and append a record
`(R){type, out, in, param}` to the list `B`. The real math is done later by
`J()` (forward pass) and `D()` (backward pass) which walk that list, so you
have to know the meaning of the numbers 0 to 7 to see what the network does.

5. Single letter, reused and shadowed names

Almost every name is one letter. `t` is first a set of "seen" characters and
then the input buffer. `h` counts characters, then batches, then validation
batches. `j` is the last character, but is also set to 0 or 1 by the progress
report. Worse, `main()` declares local arrays `float R[d+d],B[4*w],q[d*2]`
that hide the type `R`, the operation list `B` and the global pointer `q`, and
a later `char p[999]` hides the global pointer `p`. Inside `D()` a local array
is named `V`, which is also the name of the vector type.

6. Inverted array indexing

C allows `a[b]` to be written as `b[a]`, so the code uses things like
`n[t]`, `c[e]++`, `d[p]=0`, `w[r]`, `1[E]`, `2[E]`, `n[++q]--` and
`j["TV"]`. With the integer first, it is hard to tell which part is the array.

7. One big buffer addressed with offsets

All the vectors of a time step live in one array of `2*d` floats. The first
half holds the values and the second half the gradients, so `q=p+d` turns a
value pointer into its gradient pointer. The parameters live in one array of
`4*w` floats split into weights, gradients, Adam variance and Adam mean, which
the code reaches with `p[w]`, `p[b]` and `p[a]` after setting `a=3*w` and
`b=2*w`. Vectors are just `{offset,size}` pairs, so nothing in the code says
which offset belongs to which layer.

8. Hidden recursion and stack frames

`D()` trains through time by calling itself once per character. Each call
puts a new frame `float V[2*d]` on the stack and stores the distance to it in
the global `u`. The carry operation writes `p[a+u]` into the next frame and
the backward pass reads the next frame's gradient with `q[a+u]`, so the link
between time steps is hidden in a pointer difference between two unrelated
arrays.

9. Tricky expressions and conditions

`while(w>-(n=getc(f)))` is a "read until EOF or NUL" loop because `w` is still
0. `1>F%DI` means `F%DI==0`. `l*=F!=i` clears the loss only when the training
part ends. `printf(&j["\n%c..."], j["TV"], ...)` skips the leading newline and
picks `T` or `V` by indexing a string with a 0/1 value. `l-=log(n[++q]--)/N`
adds the loss and turns the probability into the softmax gradient in one
step.

10. Misleading loop bounds

Loops that run a fixed number of times are written in terms of unrelated
pointers, for example `z=SL+W; while(z-->B)` prints `SL` characters because
the local `B` is the same array as `W`, and `for(t[1+N]=m;Q;fclose(o))` uses
the float `m` (still 0) to put the end marker in the buffer and `fclose` as
the loop step.
