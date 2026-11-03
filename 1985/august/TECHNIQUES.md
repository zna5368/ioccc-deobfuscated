# 1985/august

Difficulty: Hard

1. Macros that hide structure names and member chains

`#define p struct c` and `#define q struct b` turn single letters into struct
types, and `#define h a->a`, `#define i a->b` and `#define e i->c` hide pointer
chains such as `a->b->c` behind one letter. The single letters look like
ordinary variables, so a line like `h=d;i=z(p);i->a=u;` is really
`a->a = d; a->b = malloc(sizeof(struct c)); a->b->a = u;`.

2. Macros that hide whole function headers and endings

`#define n (d,b)p*b;{q*a;p*c;` holds the parameter list, the old-style
parameter declaration, the opening brace and two local variables. Writing
`q*u n ...` therefore defines a whole function `struct b *u(d, b) struct c *b;
{ struct b *a; struct c *c; ...`. `#define s return a;}q*` ends one function
and starts the return type of the next one, so function boundaries are
invisible in the source.

3. Reused, meaningless names

Every struct member is called `a`, `b` or `c`, and the same letters are also
used for the two struct tags, the parameters and the local variables. In
`a->b->a` the first `a` is a variable, `b` is a member and the last `a` is a
member of a different struct. The reader has to track the type at every step
to know what each letter means.

4. Closures and lazy infinite streams built by hand

`struct c` holds a function pointer, an integer and a pointer to another
`struct c`; it acts as a closure (a function together with its saved
arguments). The macro `o`, `a=(*b->a)(b->b,b->c)`, calls the closure to get the
next cell (`struct b`) of an infinite list. `u` counts upward from 2, `v` drops
multiples of a number, and `w` is the sieve of Eratosthenes that stacks a new
`v` filter for every prime it finds. This functional programming style is very
unusual in C and hard to follow without drawing the pointers.

5. Recycling memory to hide the data flow

`v` and `w` do not allocate a new closure for the rest of the stream; they
overwrite the closure they were given (`i=c;i->a=v;...` and `i=b;i->a=w;...`).
The same block of memory changes meaning while the function runs, so it is
hard to see which stream a pointer belongs to.

6. Recursive `main` for printing in binary

`main` is used both as the program entry point (`b` is `argc`, which is
positive) and as a binary printer when called with a negative number
`main(-h,0)`. `putchar(b?main(b/2,0),-b%2+'0':10)` first recurses on half the
number to print the higher bits, then prints the lowest bit. When the number
reaches 0 it prints a newline (character `10`), so each number starts with a
newline instead of ending with one. Passing the value as a negative number is
just a way to tell the two uses of `main` apart.

7. Comma operator and ternary to squeeze statements together

Statements are joined with commas so they fit in a loop or `if` without braces,
for example `if(b>0)a=z(p),h=w,a->c=z(p),...,t(0,a);` and
`for(;;)o,main(-h,0),b=i;`. Inside `putchar` the ternary operator chooses
between a comma expression (recursive call then digit) and the newline.

8. Unused parameters and a function that never returns

Every function takes the same `(d, b)` parameters so they can all be stored in
the same closure type, even though `u` ignores `b` and `w` and `t` ignore `d`.
`t` is declared to return a pointer but loops forever, so `main` never reaches
its `putchar` on the first call.

9. Implicit declarations

`malloc` and `putchar` are used without including any header, relying on old C
rules that assume an undeclared function returns `int`. The `z(t)` macro casts
the result of `malloc` back to a pointer to hide this.
