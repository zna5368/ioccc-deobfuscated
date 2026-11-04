# 2011/dlowe

Difficulty: Medium

1. Themed meaningless identifiers

Every variable and function has a zombie name (`BRAINS`, `CORPSES`, `CRUSH`,
`BITE`, `CHEW`, `GNAW`, `EAT`, `PUS`, `OOZE`, `UGH`...). They hint at the
theme but say nothing about what each one holds. Even `argc`/`argv` are
renamed to `GRR`/`UGH`. `CRUSH` is a dot product, `BITE` builds a byte-pair
histogram, `CHEW` runs the network forward, `GNAW` does one backpropagation
step and `EAT` loads a directory of files.

2. Type hidden behind a macro

`#define _ float` makes the underscore a type name, so declarations look like
`_ GORE;` or `_**EAT(...)`. `_*BRAIN` is just `float *BRAIN`.

3. Magic numbers written in different ways

The same values are spelled several ways so they are hard to recognise:
`1<<16`, `2<<15`, `4<<14`, `8<<13` and `256*256` are all 65536 (the number of
inputs), `1<<17` is twice that, `7<<1` is 14, and the characters `'-'` and `'.'`
are written as `45` and `46`.

4. Reusing spare space in the weight array

The network is `float BRAINS[7][65536]`. Rows 0 to 5 are the hidden neurons'
weights, but the output neuron only needs 6 weights, so the rest of row 6 is
used as a pile of loose variables: `BRAINS[6][13]` is the error,
`[14]` the output gradient, `[34]` a hidden gradient, `[50..55]` the hidden
derivatives, `[81]` the output, `[82]` its derivative and `[97]` the total
error. Readers see array accesses where they expect named variables. The
same trick is used for the input arrays: `BITE` allocates `1<<17` floats so
`CHEW` can store the hidden neuron outputs right after the 65536 inputs.

5. Reversed array indexing

`ARM[LEG]` and `BRAIN ++ [BODY]` use the fact that `a[b]` is the same as
`*(a+b)`, so `b[a]` works too. They mean `LEG[ARM]` and `BODY[BRAIN++]`.

6. Hidden reciprocal

`powf(1+expf(-x), -1)` raises a number to the power -1, which is just
`1/(1+expf(-x))`, the logistic (sigmoid) function. The output neuron writes
the same formula the normal way, so the two look unrelated.

7. Index arithmetic that hides a constant

`LEG[6][UG+(1<<7)-14]` reads slot `114+UG`. It looks like it should be the
hidden derivatives stored at slot `50+UG` (`(1<<6)-14`), but it is not; those
slots are never written and only hold whatever was loaded from the network
file. simple.c keeps this behaviour under the name `HIDDEN_DERIV_USED`.

8. Reusing stdin as the only file handle

Instead of `fopen`, every file (each training file and each file to classify)
is opened with `freopen(name, "r", stdin)`, so `BITE` can always just call
`getc(stdin)`. The network itself is read from the original stdin first with
`fread`.

9. Comma operators and odd loop headers

`return BODY=realloc(...),BODY[BRAIN]=0,BODY;` and
`return BONE[6][82]=..., BONE[6][81];` do several assignments in one return
statement. `for(GORE=0,--ARM; ARM>=0; --ARM)` hides the setup in the loop
header, and `for(srand(time(0)),PUS=0; ...)` seeds the random number
generator inside a `for`. `while(BONE?(TOOTH=readdir(BONE)):0)` combines a
NULL check with the directory read.

10. Shaped source layout

The code is formatted into a picture, with statements
broken across lines at random places, which hides where functions and
loops begin and end.
